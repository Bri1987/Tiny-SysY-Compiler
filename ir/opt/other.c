//
// Created by Valerian on 2023/5/25.
//


#include "other.h"


//for integer-divide-optimization only
//we should analyze algorithm identical

bool checkValid(Value *lhs, Value *rhs){
    //



}


bool AlgorithmIdentical(BasicBlock *block){
    //backwards

    //if two operand is produced by the same Opcode and the same right Operand
    //we can first
    InstNode *tailNode = block->tail_node;
    InstNode *headNode = block->head_node;

    while(tailNode != headNode){
        if(isCalculationOperator(tailNode)){
            //we only consider add, div, mod, sub, mul
            Value *lhs = ins_get_lhs(tailNode->inst);
            Value *rhs = ins_get_rhs(tailNode->inst);

            //OK let's find their instructions
            Instruction *lhsIns = (Instruction *)lhs;
            Instruction *rhsIns = (Instruction *)rhs;

            //we need to make sure that all the instructions all in the same block!!


        }
        tailNode = get_prev_inst(tailNode);
    }
}


