#include "ActiveVars.hpp"
#include "set"
#include <map>
#include"vector"
#include"iostream"
#include <unordered_map>
#define DEFD(i) def.find(state->get_operand(i)) == def.end()
#define USED use.find(state) == use.end()
enum OpID
    {
        // Terminator Instructions
        ret,
        br,
        // Standard binary operators
        add,
        sub,
        mul,
        sdiv,
        // float binary operators
        fadd,
        fsub,
        fmul,
        fdiv,
        // Memory operators
        alloca1,
        load,
        store,
        // Other operators
        cmp,
        fcmp,
        phi,
        call,
        getelementptr, 
        zext, // zero extend
        fptosi,
        sitofp
        // float binary operators Logical operators

    };
OpID trans(std::string name)
{
    OpID type;
    if(name=="ret")
        type=ret;
    else if(name=="br")
        type=br;
    else if(name=="add")
        type=add;
    else if(name=="sub")
        type=sub;
    else if(name=="mul")
        type=mul;
    else if(name=="sdiv")
        type=sdiv;
    else if(name=="fadd")
        type=fadd;
    else if(name=="fsub")
        type=fsub;
    else if(name=="fmul")
        type=fmul;
    else if(name=="fdiv")
        type=fdiv;
    else if(name=="alloca")
        type=alloca1;
    else if(name=="load")
        type=load;
    else if(name=="store")
        type=store;
    else if(name=="cmp")
        type=cmp;
    else if(name=="fcmp")
        type=fcmp;
    else if(name=="phi")
        type=phi;
    else if(name=="call")
        type=call;
    else if(name=="getelementptr")
        type=getelementptr;
    else if(name=="zext")
        type=zext;
    else if(name=="fptosi")
        type=fptosi;
    else if(name=="sitofp")
        type=sitofp;
    return type;
}
std::set<Value*> set_and(std::set<Value *> set1,std::set<Value*> set2) //set1 and set2
{
    for(auto item:set2)
    {
        set1.insert(item);
    }
   return set1;
}
std::set<Value*> set_minus(std::set<Value *> set1, std::set<Value *> set2)//set1 - set2
{
    for(auto item : set2){
        if (set1.find(item) != set1.end())
        {
            set1.erase(item);
        }
    }
    return set1;
}
int  set_eq(std::set<Value*>set1,std::set<Value*>set2)// if set1==set2 return 1
{    
    if (set1.size() != set2.size())
        return 0;  
    for(auto term:set1)
    {
        if(set2.find(term)==set2.end())
            return 0;
    }
    for(auto term:set2)
    {
        if(set1.find(term)==set1.end())
            return 0;
    }
    return 1;
}
bool notconst(Value* vall)
{
    if(dynamic_cast<ConstantInt*>(vall)==nullptr &&
    dynamic_cast<ConstantFP*>(vall)==nullptr)
        return true;
    else 
        return false;
}
void ActiveVars::run()
{
    std::ofstream output_active_vars;
    output_active_vars.open("active_vars.json", std::ios::out);
    output_active_vars << "[";
    for (auto &func : this->m_->get_functions()) {
        if (func->get_basic_blocks().empty()) {
            continue;
        }
        else
        {
            func_ = func;

            func_->set_instr_name();
            live_in.clear();
            live_out.clear();
            // 在此分析 func_ 的每个bb块的活跃变量，并存储在 live_in live_out 结构内

            std::set<Value *> use;
            std::set<Value *> def;
            std::set<Value*> phiuse;
            std::map<Value *,BasicBlock *> phii;
            std::map<BasicBlock* ,std::set<Value*>> phi_bb;
            std::map<BasicBlock *, std::set<Value *>> usem; 
            std::map<BasicBlock *, std::set<Value *>> defm;  
            std::map<BasicBlock*,std::set<Value*>> phiu;
            std::map<BasicBlock *,std::map<BasicBlock* ,std::set<Value*>>> phib;
            for (auto bb : func_->get_basic_blocks())
            {
                //std::map<BasicBlock*,std::set<Value*>> phi_map;
                for (auto state: bb->get_instructions())//遍历所有指令
                {
                    auto name=state->get_instr_op_name();
                    OpID type;
                    type=trans(name);//将name转换成枚举型                   
                    switch(type)
                    {
                        case ret:
                        {
                            if (((ReturnInst *)state)->is_void_ret() == false &&
                            DEFD(0)&& notconst(state->get_operand(0)))
                                use.insert(state->get_operand(0));
                            break;
                        
                        } 
                        case br:
                        {
                            if (notconst(state->get_operand(0)) && DEFD(0) &&
                            ((BranchInst*)state)->is_cond_br())
                                use.insert(state->get_operand(0));
                            break;                        
                        } 
                        case add: 
                        case sub: 
                        case mul: 
                        case sdiv: 
                        case fadd: 
                        case fsub:
                        case fmul: 
                        case fdiv:
                        {
                            
                            if (notconst(state->get_operand(0)) && DEFD(0))
                                use.insert(state->get_operand(0));
                            if (notconst(state->get_operand(1)) && DEFD(1))
                                use.insert(state->get_operand(1));
                            if(USED)
                                def.insert(state);
                            break;
                        } 
                        case alloca1:
                        {
                            if(USED) 
                                def.insert(state);
                            break;
                        }
                        case load: 
                        {
                            if(DEFD(0))
                                use.insert(state->get_operand(0));
                            if(USED) 
                                def.insert(state);
                            break;
                        }
                        case store:
                        {
                            if (DEFD(0)&& notconst(state->get_operand(0)))               
                                use.insert(state->get_operand(0));
                            if (DEFD(1) && notconst(state->get_operand(1)))
                                use.insert(state->get_operand(1));
                            
                            break;
                        } 
                        case cmp: 
                        case fcmp: 
                        {
                            if (notconst(state->get_operand(0)) &&DEFD(0))
                                use.insert(state->get_operand(0));
                            if (notconst(state->get_operand(1)) &&DEFD(1))
                                use.insert(state->get_operand(1));
                            if(USED)
                                def.insert(state);
                            break;
                        }
                        case phi:
                        {
                            for(int i=0;i<state->get_num_operand();i=i+2)
                                if(DEFD(i))
                                {
                                    if(notconst(state->get_operand(i))) 
                                    {
                                        use.insert(state->get_operand(i));
                                        phii.insert({state->get_operand(i),dynamic_cast<BasicBlock *>(state->get_operand(i+1))});
                                    }
                                }
                            if(USED)
                                def.insert(state);
                            break;
                        } 
                        case call: 
                        {
                            for(int i=1;i<state->get_num_operand();i++)
                                if (notconst(state->get_operand(i)) && DEFD(i)) 
                                    use.insert(state->get_operand(i));
                            if(USED&&!state->is_void())
                                def.insert(state);
                            break;
                        }
                        case getelementptr: 
                        {
                            for(int i=0;i<state->get_num_operand();i++)
                                if (notconst(state->get_operand(i)) &&DEFD(i)) 
                                    use.insert(state->get_operand(i));
                            if(USED)
                                def.insert(state);
                            break;                            
                        } 
                        case zext: 
                        case fptosi: 
                        case sitofp: 
                        {
                            if(DEFD(0))
                                use.insert(state->get_operand(0));
                            if(USED)
                                def.insert(state);
                            break;
                        }        
                    }                   
                }
                
                for(auto term:phii)
                {
                    phi_bb[term.second].insert(term.first);
                    phiuse.insert(term.first);
                }
                //数据导入以及清空
                usem.insert(std::pair<BasicBlock *, std::set<Value *>>(bb, use));
                defm.insert(std::pair<BasicBlock *, std::set<Value *>>(bb, def));
                phib.insert(std::pair<BasicBlock *,std::map<BasicBlock* ,std::set<Value*>>>(bb,phi_bb));
                phiu.insert(std::pair<BasicBlock *, std::set<Value *>>(bb, phiuse));
                use.clear();
                def.clear(); 
                phii.clear();  
                phi_bb.clear();     
                phiuse.clear();
            }
 
            int flag = 1;
            for(auto bb:func->get_basic_blocks())//IN初始化为空集
            {
                std::set<Value *> empty_set;
                live_in.insert(std::pair<BasicBlock*, std::set<Value*>>(bb, empty_set));
            }
            while (flag)
            {
                flag = 0;
                for (auto bb : func->get_basic_blocks())
                {
                    std::set<Value *> in_set;
                                        
                    for (auto succ : bb->get_succ_basic_blocks())
                    {
                        auto livein = live_in.find(succ)->second;
                           
                       
                        std::set<Value*> in_without_phi;
                        if(phiu.find(succ)!=phiu.end())
                            in_without_phi=set_minus(livein,phiu.find(succ)->second);
                        for(auto term:in_without_phi)//IN[B]
                            in_set.insert(term);

                        if(phib.find(succ)!=phib.end())//PHI_IN[S,B]
                        {
                            auto map=phib.find(succ)->second;
                            if(map.find(bb)!=map.end())
                            {
                                auto vals=map.find(bb)->second;
                                for(auto term: vals)
                                    in_set.insert(term);
                            }
                        }

                       
                    }
                    if(live_out.find(bb) == live_out.end())//OUT=IN[S] and PHI_IN[S,B]
                        live_out.insert(std::pair<BasicBlock *, std::set<Value *>>(bb, in_set));
                    else 
                        live_out.find(bb)->second = in_set;
                    std::set<Value*> set1;                   
                    set1=set_minus(in_set,defm.find(bb)->second);
                    std::set<Value*> set2;
                    std::set<Value*> old_in;
                    old_in = live_in.find(bb)->second;
                    set2=set_and(usem.find(bb)->second,set1);
                    live_in.find(bb)->second=set2;//IN=USE and (OUT - def)
                    if (set_eq(set2,old_in) == 0)//IN不变后退出循环。
                        flag = 1;
                }
            }
            output_active_vars<< print();
            output_active_vars << ",";
        }
    }
    output_active_vars << "]";
    output_active_vars.close();
    return ;
}

std::string ActiveVars::print()
{
    std::string active_vars;
    active_vars +=  "{\n";
    active_vars +=  "\"function\": \"";
    active_vars +=  func_->get_name();
    active_vars +=  "\",\n";

    active_vars +=  "\"live_in\": {\n";
    for (auto &p : live_in) {
        if (p.second.size() == 0) {
            continue;
        } else {
            active_vars +=  "  \"";
            active_vars +=  p.first->get_name();
            active_vars +=  "\": [" ;
            for (auto &v : p.second) {
                active_vars +=  "\"%";
                active_vars +=  v->get_name();
                active_vars +=  "\",";
            }
            active_vars += "]" ;
            active_vars += ",\n";
        }
    }
    active_vars += "\n";
    active_vars +=  "    },\n";

    active_vars +=  "\"live_out\": {\n";
    for (auto &p : live_out) {
        if (p.second.size() == 0) {
            continue;
        } else {
            active_vars +=  "  \"";
            active_vars +=  p.first->get_name();
            active_vars +=  "\": [" ;
            for (auto &v : p.second) {
                active_vars +=  "\"%";
                active_vars +=  v->get_name();
                active_vars +=  "\",";
            }
            active_vars += "]";
            active_vars += ",\n";
        }
    }
    active_vars += "\n";
    active_vars += "    }\n";

    active_vars += "}\n";
    active_vars += "\n";
    return active_vars;
}
