#include "BasicBlock.h"
#include "Constant.h"
#include "Function.h"
#include "IRBuilder.h"
#include "Module.h"
#include "Type.h"

#include <iostream>
#include <memory>

#define CONST_INT(num) ConstantInt::get(num, module)
#define CONST_FP(num) ConstantFP::get(num, module)

int main()
{
    auto module = new Module("fun");
    auto builder = new IRBuilder(nullptr, module);


    // callee function
    Type* Int32Type = Type::get_int32_type(module);
    std::vector<Type*> Ints(1, Int32Type);
    auto calleeFuncType = FunctionType::get(Int32Type, Ints);
    auto calleeFunc = Function::create(calleeFuncType, "callee", module);
    auto bb = BasicBlock::create(module, "callee", calleeFunc);
    builder->set_insert_point(bb);
    
    std::vector<Value*> args;
    for(auto arg = calleeFunc->arg_begin(); arg != calleeFunc->arg_end(); arg++)
    {
        args.push_back(*arg);
    }
    auto aAlloca = builder->create_alloca(Int32Type);
    builder->create_store(args[0], aAlloca);
    auto aLoad = builder->create_load(aAlloca);
    auto mul = builder->create_imul(CONST_INT(2), aLoad);
    builder->create_ret(mul);


    // main function
    auto mainFunc = Function::create(FunctionType::get(Int32Type, {}), "main", module);
    bb = BasicBlock::create(module, "main", mainFunc);
    builder->set_insert_point(bb);
    auto retAlloca = builder->create_alloca(Int32Type);
    builder->create_store(CONST_INT(0), retAlloca);
    auto call = builder->create_call(calleeFunc, {CONST_INT(110)});
    builder->create_ret(call);


    std::cout << module->print();
    delete module;
    return 0;
}