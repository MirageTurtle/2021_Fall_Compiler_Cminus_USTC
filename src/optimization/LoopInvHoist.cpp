#include <algorithm>
#include <iostream>
#include "logging.hpp"
#include "LoopSearch.hpp"
#include "LoopInvHoist.hpp"


void LoopInvHoist::run()
{
    LoopSearch loop_searcher(m_, false);
    loop_searcher.run();
    auto functions = m_->get_functions();
    for (auto func : functions)
    {
        if (!func->get_basic_blocks().size())
        {
            continue;
        }
        else
        {
            //检查当前函数中各循环是否为内层循环，信息保存至is_inner_loop中
            std::unordered_map<BBset_t *, bool> is_inner_loop;
            for (auto loop : loop_searcher.get_loops_in_func(func))
            {
               if(is_inner_loop.find(loop) == is_inner_loop.end())
	       		{
		       		is_inner_loop.insert({loop, true});
	       		}
	       		auto parent_loop = loop_searcher.get_parent_loop(loop);
	       		if(parent_loop)
	      		{
		       		if(is_inner_loop.find(parent_loop) == is_inner_loop.end())
		       		{
			       		is_inner_loop.insert({parent_loop, false});
		       		}
		       		else
		       		{
			       		is_inner_loop[parent_loop] = false;
		       		}
	       		}
	    	}
            //to_deal标志当前函数中是否仍有需处理的循环
            bool to_deal = false;
            for(auto loop : loop_searcher.get_loops_in_func(func))
            {
                if(is_inner_loop[loop])
                {
                    to_deal = true;
                    break;
                }
            }
            //开始寻找循环不变式
            while (to_deal)
            {
                BBset_t *cur_inner_loop = nullptr;
                //找到一个需处理的内层循环
                for(auto loop : loop_searcher.get_loops_in_func(func))
                {
                    if (is_inner_loop[loop])
                    {
                        cur_inner_loop = loop;
                        break;
                    }
                }
                while (cur_inner_loop)
                {
                    //def_list保存各当前循环中指令
                    std::unordered_set<Value *> def_list;
                    //move_list保存寻找的可外提的指令
                    std::vector<Instruction *> move_list;
                    //ins_bb保存各指令对应的基本块
                    std::unordered_map<Instruction *, BasicBlock *> ins_bb;

                    //先遍历各基本块的各条指令，保存基本信息
                    for (auto bb : *cur_inner_loop)
                    {
                        for (auto ins : bb->get_instructions())
                        {
                            def_list.insert(ins);
                            ins_bb.insert({ins, bb}); 
                        }
                    }
                    //flag标志当前是否仍需查找当前循环内的循环不变式
                    bool flag = false;
                    do
                    {
                        flag = false;
                        for (auto bb = cur_inner_loop->begin(); bb != cur_inner_loop->end(); bb++)
                        {
                            for (auto ins : (*bb)->get_instructions())
                            {
                                if(def_list.find(ins)==def_list.end()){
                                    continue;
                                }
                                else
                                {
                                    //is_to_move标志当前指令是否是需外提的循环不变式
                                    bool is_to_move = true;
                                    if (ins->is_load() || ins->is_store() || ins->is_br() || ins->is_phi() || ins->is_call()||ins->is_ret())
                                    { 
                                        is_to_move = false;
                                        continue;
                                    }
                                    else
                                    {
                                        for (auto operand : ins->get_operands())
                                        {
                                            if (def_list.find(operand) != def_list.end())
                                            {
                                                is_to_move = false;
                                                break;
                                            }
                                        }
                                    }
                                    //若是需外提的循环不变式，将指令加入到move_list并从def_list中删除
                                    if (is_to_move)
                                    {
                                        flag = true;
                                        move_list.push_back(ins);
                                        def_list.erase(ins);
                                    }
                                }
                            }
                        }        
                    } while (flag);
                    //开始对各循环不变式的外提操作
                    //先将循环不变式从各种所在的基本块中删除
                    for(auto ins : move_list)
                    {
                        ins_bb[ins]->delete_instr(ins);
                    }
                    //在所在基本块的前驱中插入循环不变式
                    for (auto pre_block : loop_searcher.get_loop_base(cur_inner_loop)->get_pre_basic_blocks())
                    {
                        if (cur_inner_loop->find(pre_block) == cur_inner_loop->end())
                        {
                            
                            auto term = pre_block->get_terminator();
                            pre_block->delete_instr(term);
                            for(auto ins : move_list)
                            {
                                pre_block->add_instruction(ins);
                                ins->set_parent(pre_block);
                            }
                            pre_block->add_instruction(term);
                            break;
                        }
                    }
                    //更新cur_inner_loop，采用深度优先
                    is_inner_loop[cur_inner_loop] = false;
                    auto get_parent_loop = loop_searcher.get_parent_loop(cur_inner_loop);
                    if(!get_parent_loop)
                    {
                        is_inner_loop[get_parent_loop] = true;
                    }
                    cur_inner_loop = get_parent_loop;
                }
                //更新to_deal
                to_deal = false;
                for(auto loop : loop_searcher.get_loops_in_func(func))
                {
                    if(is_inner_loop[loop])
                    {
                        to_deal = true;
                        break;
                    }
                }
            }
        }
    }
}
