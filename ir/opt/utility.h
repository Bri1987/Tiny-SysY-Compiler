//
// Created by Valerian on 2023/3/19.
//

#ifndef C22V1_UTILITY_H
#define C22V1_UTILITY_H
#include "function.h"
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
bool isValidOperator(InstNode *insNode);
void clear_visited_flag(BasicBlock *block);
void correctType(Function *currentFunction);
float getOperandValue(Value *operand);
bool isCompareOperator(InstNode *insNode);
void renameVariables(Function *currentFunction);
void showInstructionInfo(InstNode *instruction_list);
void showBlockInfo(InstNode *instruction_list);
void HashSetClean(HashSet *set);
BasicBlock *newBlock(HashSet *prevBlocks,BasicBlock *block);
bool hasNoDestOperator(InstNode *insNode);
InstNode *findNode(BasicBlock *block,Instruction *inst);
bool isCriticalOperator(InstNode *insNode);
bool isCalculationOperator(InstNode *instNode);
bool HashSetDifferent(HashSet *lhs,HashSet *rhs);
void calculateNonLocals(Function *currentFunction);
void valueReplaceAll(Value *oldValue, Value *newValue, Function *currentFunction);
bool isParam(Value *val, int paramNum);
void HashSetCopy(HashSet *dest, HashSet *src);
unsigned long int hash_values(Vector *valueVector);
bool isSySYFunction(Value *function);
bool isInputFunction(Value *function);
bool isOutputFunction(Value *function);
bool isTimeFunction(Value *function);
bool JudgeXor(InstNode *insList);
void combineZext(InstNode *insList);
//true -> 会被其他用到
//false -> 不会被其他用到
bool JudgeIcmp(InstNode *icmp);
bool isSimpleOperator(InstNode *instNode);
#endif //C22V1_UTILITY_H
