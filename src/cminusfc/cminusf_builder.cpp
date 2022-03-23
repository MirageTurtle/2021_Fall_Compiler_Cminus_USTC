#include "cminusf_builder.hpp"

// use these macros to get constant value
#define CONST_FP(num) \
    ConstantFP::get((float)num, module.get())
#define CONST_ZERO(type) \
    ConstantZero::get(type, module.get())
#define CONST_INT(num) \
    ConstantInt::get((int)num, module.get())


// You can define global variables here
// to store state
Function* nowfunc; //当时处于的函数
//需要维护nowfunc并且把定义中出现的变量加入到scope中，把已经完成的函数加入到scope中，在函数的参数方面有类型转化。
// Value* curr_num = nullptr;
bool inFunc = false;
Value* curr_arg = nullptr;  // pass arg value
Value* val; //表达式的值
LoadInst* ldaddr; //普通变量的地址
GetElementPtrInst* gepaddr; //数组变量的地址
int flag=0;

/*
 * use CMinusfBuilder::Scope to construct scopes
 * scope.enter: enter a new scope
 * scope.exit: exit current scope
 * scope.push: add a new binding to current scope
 * scope.find: find and return the value bound to the name
 */
void CminusfBuilder::visit(ASTProgram &node)  // program -> declaration-list
{
    for(auto decl: node.declarations)
    {
        decl->accept(*this);
    }
}

void CminusfBuilder::visit(ASTNum &node)
{
    switch (node.type)
    {
        case TYPE_INT:
            val = CONST_INT(node.i_val);
            break;
        case TYPE_FLOAT:
            val = CONST_FP(node.f_val);
            break;
        default:
            std::cout << "TypeError!" << std::endl;
            exit(0);
    }
    
}

void CminusfBuilder::visit(ASTVarDeclaration &node)  // var-declaration -> type-specifier ID ; | type-specifier ID [ INTEGER ] ;
{
    // type-specifier part
    Type* varType = nullptr;
    switch (node.type)
    {
        case TYPE_INT:
            varType = Type::get_int32_type(module.get());
            break;
        case TYPE_FLOAT:
            varType = Type::get_float_type(module.get());
            break;
        default:
            std::cout << "TypeError!" << std::endl;
            exit(0);
    }

    if(node.num == nullptr)  // if it is not an array variable
    {
        // global variable or not
        if(scope.in_global())
        {
            auto zero = CONST_ZERO(varType);
            auto gvar = GlobalVariable::create(node.id, module.get(), varType, false, zero);
            scope.push(node.id, gvar);
        }
        else
        {
            auto var = builder->create_alloca(varType);
            scope.push(node.id, var);
        }
    }
    else  // if it is an array var
    {
        auto* arrayType = ArrayType::get(varType, node.num->i_val);  // the attribute "num" is pointer to the size of the array
        if(scope.in_global())
        {
            auto zero = CONST_ZERO(arrayType);
            auto gvar = GlobalVariable::create(node.id, module.get(), arrayType, false, zero);  //? is const or not? e.g. int a[2]; a++?
            scope.push(node.id, gvar);
        }
        else
        {
            auto var = builder->create_alloca(arrayType);
            scope.push(node.id, var);
        }
    }
}

void CminusfBuilder::visit(ASTFunDeclaration &node)  // fun-declaration -> type-specifier ID ( params ) compound-stmt
{
    FunctionType* funcType = nullptr;
    Type* retType = nullptr;
    std::vector<Type*> paramsType;  // for get funtion type
    // type-specifier part
    switch (node.type)
    {
        case TYPE_INT:
            retType = Type::get_int32_type(module.get());
            break;
        case TYPE_FLOAT:
            retType = Type::get_float_type(module.get());
            break;
        case TYPE_VOID:
            retType = Type::get_void_type(module.get());
            break;
        default:
            std::cout << "TypeError!" << std::endl;
            exit(0);
    }

    // params
    Type* Int32Type = Type::get_int32_type(module.get());
    Type* FloatType = Type::get_float_type(module.get());
    Type* Int32PtrType = Type::get_int32_ptr_type(module.get());
    Type* FloatPtrType = Type::get_float_ptr_type(module.get());
    for(auto param: node.params)
    {
        // assume the type is correct
        if(param->type == TYPE_INT)
        {
            if(!param->isarray)
            {
                paramsType.push_back(Int32Type);
            }
            else
            {
                paramsType.push_back(Int32PtrType);
            }
        }
        else
        {
            if(!param->isarray)
            {
                paramsType.push_back(FloatType);
            }
            else
            {
                paramsType.push_back(FloatPtrType);
            }
        }
    }
    funcType = FunctionType::get(retType, paramsType);
    // create function
    auto func = Function::create(funcType, node.id, module.get());
    scope.push(node.id, func);
    auto funcBB = BasicBlock::create(module.get(), node.id, func);

    nowfunc = func;
    builder->set_insert_point(funcBB);
    inFunc = true;
    scope.enter();
    // 分配函数参数的空间
    std::vector<Value*> args;
    for(auto arg = func->arg_begin(); arg != func->arg_end(); arg++)
    {
        args.push_back(*arg);
    }
    int i = 0;
    for(auto param: node.params)
    {
        curr_arg = args[i++];
        param->accept(*this);
    }
    node.compound_stmt->accept(*this);
    
    // error return part(没有终止指令)
    if(!builder->get_insert_block()->get_terminator())
    {
        if(nowfunc->get_return_type()->is_void_type())
        {
            builder->create_void_ret();
        }
        else if(nowfunc->get_return_type()->is_integer_type())
        {
            builder->create_ret(CONST_INT(0));
        }
        else if(nowfunc->get_return_type()->is_float_type())
        {
            builder->create_ret(CONST_FP(0));
        }
        else if(nowfunc->get_return_type()->is_pointer_type())
        {
            builder->create_ret(nullptr);
        }
        else  // 不知道考虑全了没
        {
            std::cout << "ReturnTypeError!" << std::endl;
            exit(0);
        }
    }
    scope.exit();
    inFunc = false;
}

void CminusfBuilder::visit(ASTParam &node)  // param -> type-specifier ID | type-specifier ID [ ]
{
    // here must be in a function scope
    if(!node.isarray)  // it is not an array param(e.g. a number)
    {
        Type* paramType = nullptr;
        switch (node.type)
        {
            case TYPE_INT:
                paramType = Type::get_int32_type(module.get());
                break;
            case TYPE_FLOAT:
                paramType = Type::get_float_type(module.get());
                break;
            default:
                std::cout << "Parameter TypeError!" << std::endl;
                exit(0);
        }
        auto paramAlloca = builder->create_alloca(paramType);
        builder->create_store(curr_arg, paramAlloca);
        scope.push(node.id, paramAlloca);
    }
    else  // an array param
    {
        Type* paramsType = nullptr;
        switch (node.type)
        {
            case TYPE_INT:
                paramsType = Type::get_int32_ptr_type(module.get());
                break;
            case TYPE_FLOAT:
                paramsType = Type::get_float_ptr_type(module.get());
                break;
            default:
                std::cout << "Parameter TypeError!" << std::endl;
                exit(0);
        }
        auto paramsAlloca = builder->create_alloca(paramsType);
        builder->create_store(curr_arg, paramsAlloca);
        scope.push(node.id, paramsAlloca);
    }
}


void CminusfBuilder::visit(ASTCompoundStmt &node)   //compound-stmt -> { local-declarations statement-list }
//function:  
{
	bool isToExit = !inFunc;
	if(!inFunc)
	{
		scope.enter();
	}
    inFunc = false;
    for (auto decl: node.local_declarations) 
    {
        decl -> accept(*this);
    }
    for (auto stmt: node.statement_list) 
    {
        stmt -> accept(*this);
    }
    if(isToExit)
    	scope.exit();
}

void CminusfBuilder::visit(ASTExpressionStmt &node) //expression-stmt -> expression; | ; 
{ 
    if(node.expression != nullptr)
        node.expression -> accept(*this);
}

void CminusfBuilder::visit(ASTSelectionStmt &node)  //selection-stmt -> if(expression) statement | if(expression) statement else statement
{ 
    node.expression -> accept(*this);
    auto e_val = val;
    auto TyInt32 = Type::get_int32_type(module.get());
    auto TyFloat = Type::get_float_type(module.get());
    Value* cmp = nullptr;
    if (e_val->get_type()->is_integer_type() == true)
        cmp = builder->create_icmp_ne(e_val,CONST_ZERO(TyInt32));
    else
        cmp = builder->create_fcmp_ne(e_val,CONST_ZERO(TyFloat));
    auto trueBB = BasicBlock::create(module.get(), "", nowfunc);
    auto falseBB = BasicBlock::create(module.get(), "", nowfunc);
    auto nextBB = BasicBlock::create(module.get(), "",nowfunc);
    builder->create_cond_br(cmp, trueBB, falseBB);
    //trueBB
    builder -> set_insert_point(trueBB);
    node.if_statement -> accept(*this);
    if(!builder->get_insert_block()->get_terminator())
        builder->create_br(nextBB);
    //falseBB
    builder -> set_insert_point(falseBB);
    if(node.else_statement != nullptr)
    {
        //builder -> set_insert_point(falseBB);
        node.else_statement -> accept(*this);
        if(!builder->get_insert_block()->get_terminator())
            builder->create_br(nextBB);
    }
    else
       builder->create_br(nextBB);
    builder->set_insert_point(nextBB);
}


void CminusfBuilder::visit(ASTIterationStmt &node)  //iteration-stmt -> while(expression) statement
{ 
    auto TyInt32 = Type::get_int32_type(module.get());
    auto TyFloat = Type::get_float_type(module.get());
    auto judgeBB = BasicBlock::create(module.get(), "", nowfunc);
    auto loopBB = BasicBlock::create(module.get(), "", nowfunc);
    auto OutloopBB = BasicBlock::create(module.get(), "", nowfunc);
    builder->create_br(judgeBB);
    //judgeBB
    builder->set_insert_point(judgeBB);
    node.expression -> accept(*this);
    auto e_val = val;
    Value* cmp = nullptr;
    if (e_val->get_type()->is_integer_type() == true)
        cmp = builder->create_icmp_ne(e_val,CONST_ZERO(TyInt32));
    else
        cmp = builder->create_fcmp_ne(e_val,CONST_ZERO(TyFloat));
    builder->create_cond_br(cmp, loopBB, OutloopBB);
    //loopBB  
    builder->set_insert_point(loopBB);
    node.statement -> accept(*this);
    if(!builder->get_insert_block()->get_terminator())
        builder->create_br(judgeBB);  
    //OutloopBB
    builder->set_insert_point(OutloopBB);
}




void CminusfBuilder::visit(ASTReturnStmt &node)     //return-stmt -> return ; | return expression ;
{   
    auto TyInt32 = Type::get_int32_type(module.get());
    auto TyFloat = Type::get_float_type(module.get());
    if(node.expression == nullptr)
        builder->create_void_ret();
    else
    {
        Value* ret;
        auto funtype = nowfunc->get_function_type()->get_return_type();
        node.expression->accept(*this);
        auto e_type = val->get_type();
        if(e_type != funtype)
        {
            if(funtype->is_integer_type() == true)
                ret = builder->create_fptosi(val, TyInt32);
            else
                ret = builder->create_sitofp(val, TyFloat);
        }
        else
            ret = val;
        builder->create_ret(ret);
    }
}


void CminusfBuilder::visit(ASTVar &node) // ID|ID[simple_expression]
{
    int nowflag=flag;
    flag=0;
    auto addr=scope.find(node.id);//scope中获取定义的ID
    auto aint=addr->get_type()->get_pointer_element_type()->is_integer_type();
	auto afloat = addr->get_type()->get_pointer_element_type()->is_float_type();
	auto aptr = addr->get_type()->get_pointer_element_type()->is_pointer_type();
    if(node.expression == nullptr)
    {
        if(nowflag==1)
        {
            val=addr;
            //flag=0;
        }
        else
        {
        if(aint||afloat||aptr)
            val=builder->create_load(addr);
        else
            val=builder->create_gep(addr,{CONST_INT(0), CONST_INT(0)});
        }
    }
    else
    { 
        node.expression->accept(*this);
        auto e_val=val;        
        auto TyInt32 = Type::get_int32_type(module.get());       
        auto type=e_val->get_type();
        if(type->is_float_type())
        {
            e_val=builder->create_fptosi(e_val,TyInt32);
        }       
        
        auto trueBB=BasicBlock::create(module.get(), "", nowfunc);
        auto falseBB=BasicBlock::create(module.get(), "", nowfunc);
        auto icmp=builder->create_icmp_lt(e_val,CONST_INT(0));
        builder->create_cond_br(icmp, trueBB, falseBB);

        builder->set_insert_point(trueBB);
        auto neg=scope.find("neg_idx_except");
        builder->create_call(static_cast<Function *>(neg),{});
        if(nowfunc->get_return_type()->is_integer_type())
            builder->create_ret(CONST_INT(0));
        else if(nowfunc->get_return_type()->is_float_type())
            builder->create_ret(CONST_FP(0.));
        else
            builder->create_void_ret();
        
        builder->set_insert_point(falseBB);        
        Value *eptr;            
        if(aptr)
        {
            auto aload=builder->create_load(addr);
            eptr=builder->create_gep(aload,{e_val});
        }
        else if(aint||afloat)
            {eptr=builder->create_gep(addr,{e_val});}
        else
           { eptr=builder->create_gep(addr,{CONST_INT(0),e_val}); }      
        if(nowflag==1)
        {
            val=eptr;
            //flag=0;
            
        }
        else
           { val=builder->create_load(eptr);}
    }
    flag=0;
}

void CminusfBuilder::visit(ASTAssignExpression &node)   // var=expression
{
    flag=1;
    if(node.expression==nullptr)
    {
        node.var->accept(*this);
    }
    else
    {
        node.var->accept(*this);
        auto addr=val;
        auto TyInt32 = Type::get_int32_type(module.get());
        auto TyFloat = Type::get_float_type(module.get());
        node.expression->accept(*this);
        auto e_val=val;
        auto tyvar=addr->get_type()->get_pointer_element_type();
        auto tye=e_val->get_type();
        if(tyvar!=tye)
        {
            if(tyvar->is_integer_type())
                e_val=builder->create_fptosi(e_val,TyInt32);
            else
                e_val=builder->create_sitofp(e_val,TyFloat);
        }
        builder->create_store(e_val,addr);
        val=e_val;
            /*
        if(flag==1)
        {
            auto addr=ldaddr;
            node.expression->accept(*this);
            auto e_val=val;
            auto typeaddr=addr->get_load_type();
            auto etype=e_val->get_type();
            if(etype->get_type_id()!=typeaddr->get_type_id())
            {
                if(typeaddr->is_integer_type())
                    e_val=builder->create_sitofp(e_val,TyFloat);
                else
                    e_val=builder->create_fptosi(e_val,TyInt32);                         
            }
            builder->create_store(e_val,addr);
            val=e_val;
        }
        else if(flag==2)
        {
            auto addr=gepaddr;
            node.expression->accept(*this);
            auto e_val=val;
            auto typeaddr=addr->get_element_type();
            auto etype=e_val->get_type();
            if(etype->get_type_id()!=typeaddr->get_type_id())
            {
                if(typeaddr->is_integer_type())
                    e_val=builder->create_sitofp(e_val,TyFloat);                               
                else
                    e_val=builder->create_fptosi(e_val,TyInt32);    
            }
            builder->create_store(e_val,addr);
            val=e_val;
        }
        */
        
    }

}
void CminusfBuilder::visit(ASTSimpleExpression &node) // 比较
{
    if(node.additive_expression_r==nullptr)
    {
        node.additive_expression_l->accept(*this);
    }
    else
    {
        node.additive_expression_l->accept(*this);
        auto lval=val;
        node.additive_expression_r->accept(*this);
        auto rval=val;
        auto TyFloat = Type::get_float_type(module.get());
        Value *cval;
        if(lval->get_type()->is_integer_type()&&rval->get_type()->is_integer_type())
        {
            switch(node.op)
        {
            case OP_LE:
                cval=builder->create_icmp_le(lval,rval);
                break;
            case OP_LT:
                cval=builder->create_icmp_lt(lval,rval);
                break;
            case OP_GT:
                cval=builder->create_icmp_gt(lval,rval);
                break;
            case OP_GE:
                cval=builder->create_icmp_ge(lval,rval);
                break;
            case OP_EQ:
                cval=builder->create_icmp_eq(lval,rval);
                break;
            case OP_NEQ:
                cval=builder->create_icmp_ne(lval,rval);
                break;
        }        
        }
        else
        {
        if(lval->get_type()->is_integer_type())
            lval=builder->create_sitofp(lval,TyFloat);  
        if(rval->get_type()->is_integer_type())
            rval=builder->create_sitofp(rval,TyFloat);
        switch(node.op)
        {
            case OP_LE:
                cval=builder->create_fcmp_le(lval,rval);
                break;
            case OP_LT:
                cval=builder->create_fcmp_lt(lval,rval);
                break;
            case OP_GT:
                cval=builder->create_fcmp_gt(lval,rval);
                break;
            case OP_GE:
                cval=builder->create_fcmp_ge(lval,rval);
                break;
            case OP_EQ:
                cval=builder->create_fcmp_eq(lval,rval);
                break;
            case OP_NEQ:
                cval=builder->create_fcmp_ne(lval,rval);
                break;
        } 
        }
        auto TyInt=Type::get_int32_type(module.get());
        val=builder->create_zext(cval,TyInt);      
    }
}

void CminusfBuilder::visit(ASTAdditiveExpression &node) // 加减
{ 
    if(node.additive_expression==nullptr)
        node.term->accept(*this);
    else
    {
        node.additive_expression->accept(*this);
        auto lval=val;
        node.term->accept(*this);
        auto rval=val;
        auto TyFloat = Type::get_float_type(module.get());
        if(lval->get_type()->is_integer_type() && rval->get_type()->is_integer_type())
        {
            switch (node.op) {
            case OP_PLUS:
                val = builder->create_iadd(lval, rval);
                break;
            case OP_MINUS:
                val = builder->create_isub(lval, rval);
                break;
            }
        }
        else
        {
            if(lval->get_type()->is_integer_type())
                lval=builder->create_sitofp(lval,TyFloat);  
            if(rval->get_type()->is_integer_type())
                rval=builder->create_sitofp(rval,TyFloat);
            switch (node.op) {
            case OP_PLUS:
                val = builder->create_fadd(lval, rval);
                break;
            case OP_MINUS:
                val = builder->create_fsub(lval, rval);
                break;
            }

        }
    }
}

void CminusfBuilder::visit(ASTTerm &node) //乘除
{ 
    if(node.term==nullptr)
        node.factor->accept(*this);
    else
    {
        node.term->accept(*this);
        auto lval=val;
        node.factor->accept(*this);
        auto rval=val;
        auto TyFloat = Type::get_float_type(module.get());
        if(lval->get_type()->is_integer_type()&&rval->get_type()->is_integer_type())
        {
            switch (node.op) {
            case OP_MUL:
                val = builder->create_imul(lval, rval);
                break;
            case OP_DIV:
                val = builder->create_isdiv(lval, rval);
                break;
            }
        }
        else
        {
            if(lval->get_type()->is_integer_type())
                lval=builder->create_sitofp(lval,TyFloat);  
            if(rval->get_type()->is_integer_type())
                rval=builder->create_sitofp(rval,TyFloat);
            switch (node.op) {
            case OP_MUL:
                val = builder->create_fmul(lval, rval);
                break;
            case OP_DIV:
                val = builder->create_fdiv(lval, rval);
                break;
            }

        }
    }
}

void CminusfBuilder::visit(ASTCall &node) //函数调用,未实现参数的类型转化
{ 
    auto fun = static_cast<Function *>(scope.find(node.id));
	std::vector<Value *> args;
	auto param_type = fun->get_function_type()->param_begin();
    auto TyFloat = Type::get_float_type(module.get());
    auto TyInt=Type::get_int32_type(module.get());    
	for (auto &arg : node.args)
	{              
		    arg->accept(*this);
		if (!val->get_type()->is_pointer_type() &&
			*param_type != val->get_type())
		{
			if (val->get_type()->is_integer_type())
				val = builder->create_sitofp(val, TyFloat);
			else
				val = builder->create_fptosi(val, TyInt);
		}
		args.push_back(val);
		param_type++;
       
	}    
	val = builder->create_call(fun, args);
  
}

