#include "ConstPropagation.hpp"
#include "logging.hpp"

#include <cmath>
#include "LoopSearch.hpp"

// 给出了返回整形值的常数折叠实现，大家可以参考，在此基础上拓展
// 当然如果同学们有更好的方式，不强求使用下面这种方式
ConstantInt *ConstFolder::compute(
    Instruction::OpID op,
    ConstantInt *value1,
    ConstantInt *value2)
{

    int c_value1 = value1->get_value();
    int c_value2 = value2->get_value();
    switch (op)
    {
    case Instruction::add:
        return ConstantInt::get(c_value1 + c_value2, module_);

        break;
    case Instruction::sub:
        return ConstantInt::get(c_value1 - c_value2, module_);
        break;
    case Instruction::mul:
        return ConstantInt::get(c_value1 * c_value2, module_);
        break;
    case Instruction::sdiv:
        return ConstantInt::get((int)(c_value1 / c_value2), module_);
        break;
    default:
        return nullptr;
        break;
    }
}

ConstantFP* ConstFolder::compute_fp(
    Instruction::OpID op,
    ConstantFP* value1,
    ConstantFP* value2
)
{
    float c_value1 = value1->get_value();
    float c_value2 = value2->get_value();
    switch (op)
    {
        case Instruction::fadd:
            return ConstantFP::get(c_value1 + c_value2, module_);
        case Instruction::fsub:
            return ConstantFP::get(c_value1 - c_value2, module_);
        case Instruction::fmul:
            return ConstantFP::get(c_value1 * c_value2, module_);
        case Instruction::fdiv:
            // 没有处理除数为0的情况
            return ConstantFP::get(c_value1 / c_value2, module_);
        default:
            return nullptr;
    }
    return nullptr;
}

ConstantInt* ConstFolder::compute_comp(
    CmpInst::CmpOp op,
    ConstantInt* value1,
    ConstantInt* value2
)
{
    int c_value1 = value1->get_value();
    int c_value2 = value2->get_value();
    switch (op)
    {
        case CmpInst::EQ:
            return ConstantInt::get(c_value1 == c_value2, module_);
        case CmpInst::NE:
            return ConstantInt::get(c_value1 != c_value2, module_);
        case CmpInst::LT:
            return ConstantInt::get(c_value1 < c_value2, module_);
        case CmpInst::LE:
            return ConstantInt::get(c_value1 <= c_value2, module_);
        case CmpInst::GT:
            return ConstantInt::get(c_value1 > c_value2, module_);
        case CmpInst::GE:
            return ConstantInt::get(c_value1 >= c_value2, module_);
        default:
            return nullptr;
    }
    return nullptr;
}

/**
 * 浮点数比较的精度设置为1.0e-6
 * 认为a等于b需满足|a-b| < 1.0e-6
 * 认为a大于b需满足a-b >= 1.0e-6
 * 其余以此类推
 */
ConstantInt* ConstFolder::compute_fcomp(
    CmpInst::CmpOp op,
    ConstantFP* value1,
    ConstantFP* value2
)
{
    float c_value1 = value1->get_value();
    float c_value2 = value2->get_value();
    switch (op)
    {
        case CmpInst::EQ:
            return ConstantInt::get(fabs(c_value1 - c_value2) < 1.0e-6, module_);
        case CmpInst::NE:
            return ConstantInt::get(fabs(c_value1 - c_value2) >= 1.0e-6, module_);
        case CmpInst::LT:
            return ConstantInt::get(c_value2 - c_value1 >= 1.0e-6, module_);
        case CmpInst::LE:
            return ConstantInt::get(c_value1 - c_value2 < 1.0e-6, module_);
        case CmpInst::GT:
            return ConstantInt::get(c_value1 - c_value2 >= 1.0e-6, module_);
        case CmpInst::GE:
            return ConstantInt::get(c_value2 - c_value1 < 1.0e-6, module_);
        default:
            return nullptr;
    }
    return nullptr;
}

// 用来判断value是否为ConstantFP，如果不是则会返回nullptr
ConstantFP *cast_constantfp(Value *value)
{
    auto constant_fp_ptr = dynamic_cast<ConstantFP *>(value);
    if (constant_fp_ptr)
    {
        return constant_fp_ptr;
    }
    else
    {
        return nullptr;
    }
}
ConstantInt *cast_constantint(Value *value)
{
    auto constant_int_ptr = dynamic_cast<ConstantInt *>(value);
    if (constant_int_ptr)
    {
        return constant_int_ptr;
    }
    else
    {
        return nullptr;
    }
}

std::map<Value*, Value*> const_val_map;

void ConstPropagation::run()
{
    // 从这里开始吧！
    auto const_folder = ConstFolder(m_);
    // get loop information
    LoopSearch loop_searcher(m_, false);
    loop_searcher.run();

    auto func_list = m_->get_functions();
    for(auto func : func_list)
    {
        if(func->get_basic_blocks().size() == 0)
        {
            continue;
        }
        else
        {
            std::set<BasicBlock*> delete_block_set;
            bool changed = false;
            bool delete_bb = false;
            do
            {
                changed = false;
                delete_bb = false;
                auto bb_list = func->get_basic_blocks();
                for(auto bb : bb_list)
                {
                    // if(bb->get_name() != ((std::string)"label39"))
                    // {
                    //     continue;
                    // }
                    // else
                    // {
                    //     LOG(ERROR) << bb->get_pre_basic_blocks().size();
                    //     exit(-1);
                    // }
                    // if bb in delete_black_set

                    // for useless basic block
                    
                    if(delete_block_set.find(bb) != delete_block_set.end())
                    {
                        continue;
                    }
                    
                    // one pre bb
                    if(bb->get_pre_basic_blocks().size() == 1)
                    {
                        // judge whether this pre bb is in loop
                        auto loop = loop_searcher.get_inner_loop(bb);
                        auto base = loop_searcher.get_loop_base(loop);
                        // this basic block is the entry of the loop
                        if(base == bb)
                        {
                            LOG(DEBUG) << base->get_name();
                            // need to delete this loop(add it to the set)
                            for(auto it = loop->begin(); it != loop->end(); it++)
                            {
                                delete_block_set.insert(*it);
                                auto succ_bb_list = (*it)->get_succ_basic_blocks();
                                for(auto succ_bb : succ_bb_list)
                                {
                                    succ_bb->remove_pre_basic_block(*it);
                                }
                            }
                            continue;
                        }
                    }  // if(bb->get_pre_basic_blocks().size() == 1)

                    // if bb don't have pre bb
                    if(bb->get_pre_basic_blocks().size() == 0 && bb->get_name() != ((std::string)"label_entry"))
                    {
                        changed = true;
                        delete_bb = true;
                        LOG(DEBUG) << bb->get_name();
                        delete_block_set.insert(bb);
                        auto succ_bb_list = bb->get_succ_basic_blocks();
                        for(auto succ_bb : succ_bb_list)
                        {
                            succ_bb->remove_pre_basic_block(bb);
                        }
                        continue;
                    }
                    

                    // change instructions
                    std::map<Value*, Value*> global_constvar_map;
                    std::vector<Instruction*> delete_insts;
                    auto inst_list = bb->get_instructions();
                    for(auto inst : inst_list)
                    {
                        LOG(DEBUG) << inst->get_name() << " " << inst->get_instr_op_name();
                        if(inst->get_instr_op_name() == ((std::string)"ret"))
                        {
                            LOG(WARNING) << inst->is_store();
                            LOG(WARNING) << inst->is_load();
                            LOG(WARNING) << inst->is_call();
                            LOG(WARNING) << inst->is_phi();
                            LOG(WARNING) << inst->isBinary();
                            LOG(WARNING) << (inst->is_zext() || inst->is_fp2si() || inst->is_si2fp());
                            LOG(WARNING) << (inst->is_cmp() || inst->is_fcmp());
                        }
                        // instruction type
                        // store inst
                        if(inst->is_store())
                        {
                            auto l_val = ((StoreInst*)inst)->get_lval();
                            // if dest(l_val in expression) is global variable
                            if(((GlobalVariable*)l_val))
                            {
                                auto r_val = ((StoreInst*)inst)->get_rval();
                                if(global_constvar_map.find(l_val) == global_constvar_map.end())
                                {
                                    global_constvar_map.insert(std::pair<Value*, Value*>(l_val, r_val));
                                }
                                else  // else can't be omitted. else part will update the value of global variales
                                {
                                    global_constvar_map[l_val] = r_val;
                                }
                            }
                            continue;
                        }
                        // load inst
                        if(inst->is_load())
                        {
                            // LOG(DEBUG) << "load instruction: " << inst->get_operand(0);
                            auto source = inst->get_operand(0);  // where the value load from
                            if(global_constvar_map.count(source))
                            // ? try if(find() != end())
                            {
                                auto value = global_constvar_map[source];
                                // we can't use replace_all_use_with function
                                // we need to ensure the use's parent is this bb
                                auto use_list = inst->get_use_list();
                                for(auto use : use_list)
                                {
                                    if(((Instruction*)(use.val_))->get_parent() == bb)
                                    {
                                        ((User*)(use.val_))->set_operand(use.arg_no_, value);
                                    }
                                }
                                delete_insts.push_back(inst);
                            }
                            continue;
                        }
                        // call function
                        if(inst->is_call())
                        {
                            // ? how about call params are const
                            continue;
                        }
                        // phi function
                        if(inst->is_phi())
                        {
                            if(inst->get_num_operand() == 2)
                            {
                                // phi function has only 1 param
                                // we need to replace it
                                auto value = inst->get_operand(0);
                                inst->replace_all_use_with(value);
                                delete_insts.push_back(inst);
                                changed = true;  // we replace phi with const
                            }
                            else
                            {
                                auto case_num = inst->get_num_operand() / 2;
                                for(int i = 0; i < case_num; i++)
                                {
                                    auto case_bb = (BasicBlock*)(inst->get_operand(2*i + 1));
                                    if(delete_block_set.find(case_bb) != delete_block_set.end())
                                    {
                                        // the bb for this case will be deleted
                                        // we need to remove this case
                                        inst->remove_operands(2*i, 2*i+1);
                                        changed = true;  // we remove some case from phi function
                                    }
                                }
                            }
                            continue;
                        }
                        // two-operand instruction
                        if(inst->isBinary())
                        {
                            // operands part
                            bool flag_delete = false;
                            auto operand_a = ((BinaryInst*)inst)->get_operand(0);
                            auto operand_b = ((BinaryInst*)inst)->get_operand(1);
                            if(const_val_map.count(operand_a))
                            {
                                operand_a = const_val_map[operand_a];
                            }
                            if(const_val_map.count(operand_b))
                            {
                                operand_b = const_val_map[operand_b];
                            }
                            // operation type(int or float)
                            if(inst->is_add() || (inst->is_sub()) || (inst->is_mul()) || (inst->is_div()))
                            {
                                auto operanda_constint = cast_constantint(operand_a);
                                auto operandb_constint = cast_constantint(operand_b);
                                if(operanda_constint && operandb_constint)
                                {
                                    ConstantInt* const_result = const_folder.compute(inst->get_instr_type(), operanda_constint, operandb_constint);
                                    const_val_map.insert(std::pair<Value*, Value*>(inst, const_result));
                                    inst->replace_all_use_with(const_result);
                                    flag_delete = true;
                                }
                            }
                            else
                            {
                                auto operanda_constfp = cast_constantfp(operand_a);
                                auto operandb_constfp = cast_constantfp(operand_b);
                                if(operanda_constfp && operandb_constfp)
                                {
                                    ConstantFP* const_result = const_folder.compute_fp(inst->get_instr_type(), operanda_constfp, operandb_constfp);
                                    const_val_map.insert(std::pair<Value*, Value*>(inst,const_result));
                                    inst->replace_all_use_with(const_result);
                                    flag_delete = true;
                                }
                            }
                            if(flag_delete)
                            {
                                delete_insts.push_back(inst);
                                changed = true; // const propagation
                            }
                            continue;
                        }
                        // conversion instruction
                        if(inst->is_zext() || inst->is_fp2si() || inst->is_si2fp())
                        {
                            bool flag_delete = false;
                            if(inst->is_fp2si())
                            {
                                auto fp_val = cast_constantfp(inst->get_operand(0));
                                if(fp_val)
                                {
                                    auto int_val = ConstantInt::get((int)(fp_val->get_value()), m_);
                                    const_val_map.insert(std::pair<Value*, Value*>(inst, int_val));
                                    inst->replace_all_use_with(int_val);
                                    flag_delete = true;
                                }
                            }
                            else if(inst->is_si2fp())
                            {
                                auto int_val = cast_constantint(inst->get_operand(0));
                                if(int_val)
                                {
                                    auto fp_val = ConstantFP::get((float)(int_val->get_value()), m_);
                                    const_val_map.insert(std::pair<Value*, Value*>(inst, fp_val));
                                    inst->replace_all_use_with(fp_val);
                                    flag_delete = true;
                                }
                            }
                            else  // inst->is_zext()
                            {
                                // zext() is often with comparison instruction
                                LOG(INFO) << "zext instruction: " << inst->get_name();
                                auto bool_val = cast_constantint(inst->get_operand(0));
                                if(bool_val)
                                {
                                    LOG(DEBUG) << "bool_val is const.";
                                    auto int_val = ConstantInt::get((int)(bool_val->get_value()), m_);
                                    const_val_map.insert(std::pair<Value*, Value*>(inst, int_val));
                                    inst->replace_all_use_with(int_val);
                                    flag_delete = true;
                                }
                            }
                            if(flag_delete)
                            {
                                delete_insts.push_back(inst);
                                changed = true;
                            }
                            continue;
                        }
                        // comparison instruction
                        if(inst->is_cmp() || inst->is_fcmp())
                        {
                            bool flag_delete = false;
                            LOG(INFO) << "cmp instruction: " << inst->get_name();
                            auto operand_a = inst->get_operand(0);
                            auto operand_b = inst->get_operand(1);
                            ConstantInt* cmp_result;
                            auto op = ((CmpInst*)inst)->get_cmp_op();
                            if(inst->is_cmp())
                            {
                                auto operanda_constint = cast_constantint(operand_a);
                                // debug for testcase1 i < 100...
                                LOG(DEBUG) << "operanda_constint";
                                if(operanda_constint)
                                {
                                    LOG(DEBUG) << operanda_constint->get_value(operanda_constint);
                                }
                                else
                                {
                                    LOG(DEBUG) << "operanda_constint is nullptr";
                                }
                                auto operandb_constint = cast_constantint(operand_b);
                                if(operanda_constint && operandb_constint)
                                {
                                    cmp_result = const_folder.compute_comp(op, operanda_constint, operandb_constint);
                                    flag_delete = true;
                                }
                            }
                            else
                            {
                                auto operanda_constfp = cast_constantfp(operand_a);
                                auto operandb_constfp = cast_constantfp(operand_b);
                                if(operanda_constfp && operandb_constfp)
                                {
                                    cmp_result = const_folder.compute_fcomp(op, operanda_constfp, operandb_constfp);
                                    flag_delete = true;
                                }
                            }
                            if(flag_delete)
                            {
                                const_val_map.insert(std::pair<Value*, Value*>(inst, cmp_result));
                                inst->replace_all_use_with(cmp_result);
                                delete_insts.push_back(inst);
                                changed = true;
                            }
                            continue;
                        }  // end instruction type
                    }  // for(auto inst : inst_list)

                    // delete instructions
                    for(auto inst : delete_insts)
                    {
                        bb->delete_instr(inst);
                    }

                    // terminator part
                    auto terminator = bb->get_terminator();
                    if(terminator->is_br() && terminator->get_num_operand() == 3)
                    {
                        // just this case
                        LOG(DEBUG) << terminator->get_parent()->get_name();
                        auto condition = terminator->get_operand(0);
                        auto condition_constint = cast_constantint(condition);
                        if(condition_constint)
                        {
                            auto first_bb = (BasicBlock*)(terminator->get_operand(1));
                            auto second_bb = (BasicBlock*)(terminator->get_operand(2));
                            auto condition_value = condition_constint->get_value();
                            // if condition_value>0, we need to remove second_bb
                            if(condition_value > 0)
                            {
                                auto pre_num = second_bb->get_pre_basic_blocks().size();
                                auto loop = loop_searcher.get_inner_loop(second_bb);
                                auto base = loop_searcher.get_loop_base(loop);
                                bb->remove_succ_basic_block(second_bb);
                                second_bb->remove_pre_basic_block(bb);
                                if(second_bb->get_name() == ((std::string)"label39"))
                                {
                                    LOG(ERROR) << terminator->get_parent()->get_name();
                                    LOG(ERROR) << pre_num;
                                    if(pre_num == 1)
                                    {
                                        // return;
                                    }
                                }
                                if(pre_num == 1 || (pre_num == 2 && base == second_bb))
                                {
                                    // pre_num==1 means second_bb will never be there. It will be orphan.
                                    // pre_num==2 && base==second_bb means second_bb is the base of its loop. The loop will never be there.
                                    if(second_bb->get_name() == ((std::string)"label39"))
                                    {
                                        LOG(ERROR) << "ERROR! Will delete label39!";
                                        LOG(ERROR) << terminator->get_parent()->get_name();
                                        LOG(ERROR) << (pre_num==1) << " " << (pre_num==2 && base==second_bb);
                                        if(pre_num == 2 && base == second_bb)
                                        {
                                            LOG(ERROR) << base->get_name();
                                        }
                                        else
                                        {
                                            LOG(ERROR) << (*(second_bb->get_pre_basic_blocks().begin()))->get_name();
                                        }
                                    }
                                    delete_bb = true;
                                    delete_block_set.insert(second_bb);
                                    // delete the succ
                                    auto succ_list = second_bb->get_succ_basic_blocks();
                                    for(auto succ : succ_list)
                                    {
                                        succ->remove_pre_basic_block(second_bb);
                                    }
                                }
                                // change the br instruction
                                // 这个bug找了两三个小时，因为写在上面那个if里面了
                                ((BranchInst*)terminator)->create_br(first_bb, bb);
                                bb->delete_instr(terminator);
                            }
                            else
                            {
                                delete_bb = true;
                                delete_block_set.insert(first_bb);
                                bb->remove_succ_basic_block(first_bb);
                                first_bb->remove_pre_basic_block(bb);
                                auto succ_list = second_bb->get_succ_basic_blocks();
                                for(auto succ : succ_list)
                                {
                                    succ->remove_pre_basic_block(first_bb);
                                }
                                ((BranchInst*)terminator)->create_br(second_bb, bb);
                                bb->delete_instr(terminator);
                            }  // if-else condition
                        }
                        continue;
                    }  // if(terminator->is_br())
                }  // for(auto bb : bb_list)
            }while(changed || delete_bb);  // do-while

            // delete bb
            for(auto bb : delete_block_set)
            {
                if(bb->get_name() == ((std::string)"label39"))
                {
                    LOG(ERROR) << "Delete label39, Error!";
                }
                func->remove(bb);
            }
        }
    }
}
