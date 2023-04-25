//
// Created by Valerian on 2023/3/31.
//

#ifndef C22V1_DEADCODEELIMINATION_H
#define C22V1_DEADCODEELIMINATION_H
#include "function.h"
#include "utility.h"
#include "dominance.h"
bool DeadCodeElimination(Function *currentFunction);
void Mark(Function *currentFunction);
void Sweep(Function *currentFunction);
void Clean(Function *currentFunction);
bool OnePass(Queue *postQueue);
#endif //C22V1_DEADCODEELIMINATION_H
