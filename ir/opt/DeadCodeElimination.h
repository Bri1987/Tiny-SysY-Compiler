//
// Created by Valerian on 2023/3/31.
//

#ifndef C22V1_DEADCODEELIMINATION_H
#define C22V1_DEADCODEELIMINATION_H
#include "function.h"
#include "utility.h"
bool DeadCodeElimination(Function *currentFunction);

void Clean(Function *currentFunction);

void OnePass(Function *currentFunction);

//phi生成后跑一次 phi函数消除之后
bool DeleteUselessBasicBlocks(Function *currentFunction);
#endif //C22V1_DEADCODEELIMINATION_H
