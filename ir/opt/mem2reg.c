//
// Created by Valerian on 2023/1/15.
//

#include "mem2reg.h"
HashSet *nonLocals = NULL;
HashSet *killed = NULL;
InstNode* new_phi(Value *val){
    Instruction *phiIns = ins_new(0);
    phiIns->Opcode = Phi;
    phiIns->user.value.pdata->pairSet = NULL;
    InstNode *phiNode = new_inst_node(phiIns);
    //做一个映射 记录现在对应的是哪个alloca
    phiNode->inst->user.value.alias = val;
    // 添加一个默认的名字
    phiIns->user.value.name = (char *)malloc(sizeof(char) * 7);
    strcpy(phiIns->user.value.name,"%phi");
    return phiNode;
}

pair *createPhiInfo(BasicBlock *prev, Value *val){
    pair *phiInfo = (pair*)malloc(sizeof(pair));
    phiInfo->define = val;
    phiInfo->from = prev;
    return phiInfo;
}

void insert_phi(BasicBlock *block,Value *val){
    //头指令
    InstNode *instNode = block->head_node;
    //插入空的phi函数
    InstNode *phiInstNode = new_phi(val);
    ins_insert_after(phiInstNode,instNode);
    // 让这个语句属于BasicBlock
    phiInstNode->inst->Parent = block;

    //设置当前phi函数的type
    Value *insValue = ins_get_value(phiInstNode->inst);
    insValue->VTy->ID = val->VTy->ID;
    printf("%d--------",val->VTy->ID);
    printf("set PhiNode success !\n");
}

void insertPhiInfo(InstNode *ins,pair *phiInfo){
    if(ins->inst->user.value.pdata->pairSet == nullptr){
        ins->inst->user.value.pdata->pairSet = HashSetInit();
    }
    assert(ins->inst->user.value.pdata->pairSet != NULL);
    assert(phiInfo != NULL);
    HashSetAdd(ins->inst->user.value.pdata->pairSet,phiInfo);
}

void mem2reg(Function *currentFunction){
    //对于现在的Function初始化
    currentFunction->loadSet = HashMapInit();
    currentFunction->storeSet = HashMapInit();

    BasicBlock *entry = currentFunction->entry;
    BasicBlock *end = currentFunction->tail;


    InstNode *head = entry->head_node;
    InstNode *tail = end->tail_node;

    InstNode *curNode = head;
    // 第一次循环去扫描一遍 记录loadSet 和 storeSet
    //(Value*)instruction -> HashSet
    while(curNode != get_next_inst(tail)) {
        BasicBlock *parent = curNode->inst->Parent;

        if (curNode->inst->Opcode == Load) {
            Value *loadValue = curNode->inst->user.use_list[0].Val;
            if(isLocalVar(loadValue)){
                if (HashMapContain(currentFunction->loadSet, loadValue)) {
                    //不是第一次加入
                    HashSet *loadSet = HashMapGet(currentFunction->loadSet, loadValue);
                    //那么就将现在的BasicBlock
                    if (!HashSetFind(loadSet, parent)) {
                        //如果存在那么就意味着存在两次load 我们目前仅保留一次 如果不存在我们再加入进去
                        HashSetAdd(loadSet, parent);
                    }
                } else {
                    HashSet *loadSet = HashSetInit();
                    HashSetAdd(loadSet, parent);
                    HashMapPut(currentFunction->loadSet, loadValue, loadSet);
                }
            }
        }

        if (curNode->inst->Opcode == Store) {
            //一定对应的是第二个
            Value *storeValue = curNode->inst->user.use_list[1].Val;
            if(isLocalVar(storeValue)){
                if (HashMapContain(currentFunction->storeSet, storeValue)) {
                    //如果存在的话那么之前的HashSet也一定存在
                    HashSet *storeSet = HashMapGet(currentFunction->storeSet, storeValue);
                    if (!HashSetFind(storeSet, parent)) {
                        HashSetAdd(storeSet, parent);
                    }
                } else {
                    HashSet *storeSet = HashSetInit();
                    HashSetAdd(storeSet, parent);
                    HashMapPut(currentFunction->storeSet, storeValue, storeSet);
                }
            }
        }
        curNode = get_next_inst(curNode);
    }

    // 打印一下现在HashSet
    HashMapFirst(currentFunction->loadSet);
    for(Pair *pair = HashMapNext(currentFunction->loadSet); pair != NULL; pair = HashMapNext(currentFunction->loadSet)){
        Value *val = pair->key;
        BasicBlock *block = pair->value;
        printf("value %s : load in block : b%d\n", val->name,block->id);
    }
    curNode = head;
    //做一些基本的优化
    // 删除无用的alloca
    while(curNode != get_next_inst(tail)){
        //如果alloca没有load就删除这个alloca
        //首先找到对应的value
        //不是alloca的数组并且对这个alloca没有load的话
        Value *insValue = ins_get_value(curNode->inst);
        if(curNode->inst->Opcode == Alloca && !isLocalArray(insValue) && !HashMapContain(currentFunction->loadSet,insValue)){
            //不存在loadSet之中不存在这个value
            //删除这个instruction
            InstNode *next = get_next_inst(curNode);
            delete_inst(curNode);
            curNode = next;
        }else if(curNode->inst->Opcode == Store) {
            Value *storeValue = ins_get_rhs(curNode->inst);
            if(isLocalVar(storeValue) && !HashMapContain(currentFunction->loadSet, storeValue)){
                InstNode *next = get_next_inst(curNode);
                delete_inst(curNode);
                curNode = next;
            }else{
                curNode = get_next_inst(curNode);
            }
        }else{
            curNode = get_next_inst(curNode);
        }
    }

    //TODO 做一些剪枝来保证phi函数插入更加简介

    //mem2reg的主要过程
    //Function里面去找
    HashMapFirst(currentFunction->storeSet);
    //对里面所有的variable进行查找
    for(Pair *pair = HashMapNext(currentFunction->storeSet); pair != NULL; pair = HashMapNext(currentFunction->storeSet)){
        //先看一下我们的这个是否是正确的
        Value *val = pair->key;
        HashSet *storeSet = pair->value;  //这个value对应的所有defBlocks

        // 只有non-Locals 我们才需要放置phi函数
        if(HashSetFind(nonLocals, val)){
            HashSetFirst(storeSet);
            printf("value : %s store(defBlocks) : ", val->name);
            for(BasicBlock *key = HashSetNext(storeSet); key != NULL; key = HashSetNext(storeSet)){
                printf("b%d ",key->id);
            }
            printf("\n");

            //place phi
            HashSet *phiBlocks = HashSetInit();
            while(HashSetSize(storeSet) != 0){
                HashSetFirst(storeSet);
                BasicBlock *block = HashSetNext(storeSet);
                HashSetRemove(storeSet,block);
                assert(block != nullptr);
                HashSet *df = block->df;
                HashSetFirst(df);
                for(BasicBlock *key = HashSetNext(df); key != nullptr; key = HashSetNext(df)) {
                    if (!HashSetFind(phiBlocks, key)) {
                        //在key上面放置phi函数
                        insert_phi(key, val);
                        HashSetAdd(phiBlocks, key);
                        if (!HashSetFind(storeSet, key)) {
                            HashSetAdd(storeSet, key);
                        }
                    }
                }
            }
            HashSetDeinit(phiBlocks);
        }
    }



    printf("after insert phi function!\n");
    //变量重新命名
    // DomTreeNode *root = currentFunction->root;
    assert(entry->domTreeNode == currentFunction->root);
    DomTreeNode *root = entry->domTreeNode;

    HashMap *IncomingVals = HashMapInit();

    curNode = entry->head_node;
    while(curNode != get_next_inst(entry->tail_node)){
        //printf("curNode id is : %d",curNode->inst->i);
        if(curNode->inst->Opcode == Alloca){
            //每一个alloca都对应一个stack
            stack *allocaStack = stackInit();
            assert(allocaStack != nullptr);
            HashMapPut(IncomingVals,&(curNode->inst->user.value),allocaStack);
            assert(HashMapContain(IncomingVals,&(curNode->inst->user.value)));
        }
        curNode = get_next_inst(curNode);
    }

    printf("in rename pass!\n");

    //变量重命名 如果都没有alloc那么就不需要了
    if(HashMapSize(IncomingVals) != 0){
        printf("---------------------------------------------fuck--------------------------------------- \n");
        dfsTravelDomTree(root,IncomingVals);
    }

    printf("after rename !\n");
    correctPhiNode(currentFunction);

    printf("after correct phiNode\n");

    // delete load 和 store
    deleteLoadStore(currentFunction);


    printf("after delete alloca load store\n");
    // 让LLVM IR符合标准
    renameVariabels(currentFunction);


    // OK 记得释放内存哦
    //先释放栈的内存再释放HashMap的内存
    HashMapFirst(IncomingVals);
    for(Pair *pair = HashMapNext(IncomingVals); pair != NULL; pair = HashMapNext(IncomingVals)){
        stack *allocStack = pair->value;
        stackDeinit(allocStack);
    }
    HashMapDeinit(IncomingVals);

    // Function中的HashSet也不需要了
    HashMapDeinit(currentFunction->storeSet);
    HashMapDeinit(currentFunction->loadSet);
}

// 针对的是需要拷贝回原来的
void insertCopies(BasicBlock *block,Value *dest,Value *src){
    InstNode *tailIns = block->tail_node;
    assert(tailIns != NULL);
    InstNode *copyIns = newCopyOperation(src);
    assert(copyIns != NULL);
    copyIns->inst->Parent = block;
    Value *insValue = ins_get_value(copyIns->inst);
    insValue->alias = dest;

    //应该不会超过10位数
    // 针对这个情况好像没什么用
    insValue->name = (char*)malloc(sizeof(char) * 10);
    strcpy(insValue->name,"%temp");
    ins_insert_before(copyIns,tailIns);

    // 由于是CopyOperation 所以phi的type我们已经是设置好了的
}


void dfsTravelDomTree(DomTreeNode *node,HashMap *IncomingVals){

    printf("in rename phrase : block : %d\n", node->block->id);
    assert(HashMapSize(IncomingVals) != 0);
    // 先根处理
    BasicBlock *block = node->block;
    InstNode *head = block->head_node;
    InstNode *tail = block->tail_node;

    InstNode *curr = head;


    //记录一下每一个alloc的压栈次数
    HashMap *countDefine = HashMapInit();
    HashMapFirst(IncomingVals);
    for(Pair *pair = HashMapNext(IncomingVals); pair != NULL; pair = HashMapNext(IncomingVals)){
        int *defineTimes = (int *)malloc(sizeof(int));
        //默认初始化为零
        *(defineTimes) = 0;
        Value *alloc = (Value*)pair->key;
        HashMapPut(countDefine,alloc,defineTimes);
    }

    printf("11111 \n");
    // 变量重命名
    while(curr != get_next_inst(tail)){
        switch(curr->inst->Opcode) {
            case Load: {
                Value *alloc = ins_get_lhs(curr->inst);
                if(isLocalVar(alloc)){
                    // 对应的alloc
                    printf("what name : %s\n",alloc->name);
                    // 需要被替换的
                    Value *value = ins_get_value(curr->inst);
                    Value *replace = nullptr;
                    //找到栈
                    stack *allocStack = HashMapGet(IncomingVals, alloc);
                    //如果是nullptr
                    assert(allocStack != nullptr);
                    //去里面找
                    stackTop(allocStack, (void *) &replace);

                    //replace 必然不能是nullptr否则会出现未定义错误
                    assert(replace != nullptr);
                    value_replaceAll(value, replace);
                }
                break;
            }
            case Store: {
                Value *store = ins_get_rhs(curr->inst); // 对应的allocas
                if(isLocalVar(store)){
                    //更新变量的使用
                    //对应的{}
                    Value *data = ins_get_lhs(curr->inst);
                    //可以直接这样更新
                    stack *allocStack = HashMapGet(IncomingVals, store);
                    assert(allocStack != nullptr);
                    printf("store %s\n",data->name);
                    stackPush(allocStack, data);
                    //记录define的次数
                    int *defineTime = HashMapGet(countDefine,store);
                    //if(count
                    assert(defineTime != NULL);
                    *(defineTime) = *(defineTime) + 1;
                }
                break;
            }
            case Phi: {
                // 找到对应的allocas
                Value *alloc = curr->inst->user.value.alias;
                // 如果是phi的话我们就需要将现在的栈顶设置为phi指令
                Value *phi = &curr->inst->user.value;
                stack *allocStack = HashMapGet(IncomingVals, alloc);
                assert(allocStack != nullptr);
                stackPush(allocStack, phi);
                //
                int *defineTime = HashMapGet(countDefine,alloc);
                assert(defineTime != NULL);
                *(defineTime) = *(defineTime) + 1;
                break;
            }
        }
        curr = get_next_inst(curr);
    }

    // 如果当前的末尾是funcEnd的话我们需要回退一个instNode
    if(tail->inst->Opcode == FunEnd){
        tail = get_prev_inst(tail);
    }

    // 维护该基本块所有的后继基本块中的phi指令 修改phi函数中的参数
    if(tail->inst->Opcode == br){
        //label的编号是
        int labelId = tail->inst->user.value.pdata->instruction_pdata.true_goto_location;
        printf("br %d\n",labelId);
        //从function的头节点开始向后寻找
        InstNode *funcHead = ins_get_funcHead(tail);
        BasicBlock *nextBlock = search_ins_label(funcHead,labelId)->inst->Parent;

        //维护这个block内的phiNode 跳过Label
        InstNode *nextBlockCurr = get_next_inst(nextBlock->head_node);

        while(nextBlockCurr->inst->Opcode == Phi){
            //对应的是哪个
            Value *alias = nextBlockCurr->inst->user.value.alias;
            //去找对应需要更新的
            stack *allocStack = HashMapGet(IncomingVals,alias);
            assert(allocStack != NULL);

            //从中去取目前到达的定义
            Value *pairValue = NULL;
            stackTop(allocStack,(void *)&pairValue);
            //填充信息

            // TODO 如果是空那么应该填充Undefine
            if(pairValue != NULL){
                pair *phiInfo = createPhiInfo(block,pairValue);
                insertPhiInfo(nextBlockCurr,phiInfo);
                printf("insert a phi info\n");
            }else{
                // 如果是NULL的话就是Undefine
                pair *phiInfo = createPhiInfo(block,pairValue);
                insertPhiInfo(nextBlockCurr,phiInfo);
            }
            nextBlockCurr = get_next_inst(nextBlockCurr);
        }
    }

    if(tail->inst->Opcode == br_i1){
        int labelId1 = tail->inst->user.value.pdata->instruction_pdata.true_goto_location;
        int labelId2 = tail->inst->user.value.pdata->instruction_pdata.false_goto_location;
        InstNode *funcHead = ins_get_funcHead(tail);

        BasicBlock *trueBlock = search_ins_label(funcHead,labelId1)->inst->Parent;
        BasicBlock *falseBlock = search_ins_label(funcHead,labelId2)->inst->Parent;


        //跳回第一个基本块没有意义 所以我们都去跳过Label
        InstNode *trueBlockCurr = get_next_inst(trueBlock->head_node);
        InstNode *falseBlockCurr = get_next_inst(falseBlock->head_node);

        //分别去维护信息
        while(trueBlockCurr->inst->Opcode == Phi){
            // 对应的是哪个alias
            Value *alias = trueBlockCurr->inst->user.value.alias;

            //取栈里面找那个对应的Value
            stack *allocStack = HashMapGet(IncomingVals,alias);
            assert(allocStack != NULL);

            Value *pairValue = NULL;
            stackTop(allocStack,(void *)&pairValue);

            // TODO 还是需要
            if(pairValue != NULL){
                pair *phiInfo = createPhiInfo(block,pairValue);
                insertPhiInfo(trueBlockCurr,phiInfo);
            }else{
                pair *phiInfo = createPhiInfo(block,pairValue);
                insertPhiInfo(trueBlockCurr,phiInfo);
            }
            trueBlockCurr = get_next_inst(trueBlockCurr);
        }

        while(falseBlockCurr->inst->Opcode == Phi){
            Value *alias = falseBlockCurr->inst->user.value.alias;

            stack *allocStack = HashMapGet(IncomingVals,alias);
            assert(allocStack != NULL);

            Value *pairValue = NULL;
            stackTop(allocStack,(void *)&pairValue);

            // TODO pairValue 是NULL的时候需要是Undefine
            if(pairValue != NULL){
                pair *phiInfo  = createPhiInfo(block,pairValue);
                insertPhiInfo(falseBlockCurr,phiInfo);
            }else{
                pair *phiInfo  = createPhiInfo(block,pairValue);
                insertPhiInfo(falseBlockCurr,phiInfo);
            }
            falseBlockCurr = get_next_inst(falseBlockCurr);
        }
    }

    printf("333\n");
    // 递归遍历
    HashSetFirst(node->children);
    for(DomTreeNode *key = HashSetNext(node->children); key != nullptr; key = HashSetNext(node->children)){
        dfsTravelDomTree(key,IncomingVals);
    }

    // 弹栈
    HashMapFirst((IncomingVals));
    for(Pair *pair = HashMapNext(IncomingVals); pair != NULL; pair = HashMapNext(IncomingVals)){
        Value *alloc = pair->key;

        stack *allocStack = HashMapGet(IncomingVals,alloc);
        assert(allocStack != NULL);
        if(allocStack != NULL){
            //去找压栈的次数
            assert(allocStack != NULL);
            int *defineTime = HashMapGet(countDefine,alloc);
            int actualTime = *(defineTime);
            printf("block : %d alloc : %s defineTimes : %d\n",block->id,alloc->name,actualTime);
            for(int i = 0; i < actualTime; i++){
                stackPop(allocStack);
            }
        }
    }

    //释放内存
    HashMapFirst(countDefine);
    for(Pair *pair = HashMapNext(countDefine); pair != NULL; pair = HashMapNext(countDefine)){
        int *countTime = pair->value;
        free(countTime);
    }
}

void deleteLoadStore(Function *currentFunction){
    // 删除这个函数内的load store

    BasicBlock *entry = currentFunction->entry;
    BasicBlock *end = currentFunction->tail;

    InstNode *head = entry->head_node;
    InstNode *tail = end->tail_node;

    InstNode *curr = head;
    while(curr != get_next_inst(tail)){
        switch(curr->inst->Opcode){
            case Alloca:{
                Value *insValue = ins_get_value(curr->inst);
                if(isLocalVar(insValue)){
                    InstNode *next = get_next_inst(curr);
                    delete_inst(curr);
                    curr = next;
                }else{
                    curr = get_next_inst(curr);
                }
                break;
            }
            case Load:{
                Value *loadValue = ins_get_lhs(curr->inst);
                if(isLocalVar(loadValue)){
                    InstNode *next = get_next_inst(curr);
                    delete_inst(curr);
                    curr = next;
                }else{
                    curr = get_next_inst(curr);
                }
                break;
            }
            case Store:{
                Value *storeValue = ins_get_rhs(curr->inst);
                if(isLocalVar(storeValue)){
                    InstNode *next = get_next_inst(curr);
                    delete_inst(curr);
                    curr = next;
                }else{
                    curr = get_next_inst(curr);
                }
                break;
            }
            default:{
                curr = get_next_inst(curr);
                break;
            }
        }
    }
}

void renameVariabels(Function *currentFunction) {
    bool haveParam = false;
    BasicBlock *entry = currentFunction->entry;
    BasicBlock *end = currentFunction->tail;

    InstNode *currNode = entry->head_node;

    //currNode的第一条是FunBegin,判断一下是否有参
    Value *funcValue = currNode->inst->user.use_list->Val;
    if (funcValue->pdata->symtab_func_pdata.param_num > 0)
        haveParam = true;

    //开始时候为1或__
    int countVariable = 0;
    if (haveParam){
        //更新第一个基本块
        countVariable += funcValue->pdata->symtab_func_pdata.param_num;
        currNode->inst->Parent->id = countVariable;
    }
    countVariable++;

    currNode = get_next_inst(currNode);
    while (currNode != get_next_inst(end->tail_node)) {
        if (currNode->inst->Opcode != br && currNode->inst->Opcode != br_i1) {

            if (currNode->inst->Opcode == Label) {
                //更新一下BasicBlock的ID 顺便就更新了phi
                BasicBlock *block = currNode->inst->Parent;
                block->id = countVariable;
                currNode->inst->user.value.pdata->instruction_pdata.true_goto_location = countVariable;
                countVariable++;
            } else {
                // 普通的instruction语句
                char *insName = currNode->inst->user.value.name;

                //如果不为空那我们可以进行重命名
                if (insName != NULL && insName[0] == '%') {
                    char newName[10];
                    clear_tmp(insName);
                    newName[0] = '%';
                    int index = 1;
                    int rep_count = countVariable;
                    while (rep_count) {
                        newName[index] = (rep_count % 10) + '0';
                        rep_count /= 10;
                        index++;
                    }
                    int j = 1;
                    for (int i = index - 1; i >= 1; i--) {
                        insName[j] = newName[i];
                        j++;
                    }
                    countVariable++;
                }
            }
        }
        currNode = get_next_inst(currNode);
    }

    clear_visited_flag(entry);
    //通过true false block的方式设置
    // 跳过funcBegin
    BasicBlock *tail = currentFunction->tail;
    currNode = get_next_inst( entry->head_node);
    while(currNode != get_next_inst(end->tail_node)){
        BasicBlock *block = currNode->inst->Parent;
        if(block->visited == false){
            block->visited = true;
            InstNode *blockTail = block->tail_node;
            if(block == tail){
                blockTail = get_prev_inst(blockTail);
            }
            Value *blockTailInsValue = ins_get_value(blockTail->inst);
            if(block->true_block){
                blockTailInsValue->pdata->instruction_pdata.true_goto_location = block->true_block->id;
            }
            if(block->false_block){
                blockTailInsValue->pdata->instruction_pdata.false_goto_location = block->false_block->id;
            }
        }
        currNode = get_next_inst(currNode);
    }
}

void insertPhiCopies(DomTreeNode *node, HashMap *varStack){
    // 记录一下哪些会被push
    HashSet *pushed = HashSetInit();

    BasicBlock *block = node->block;

    // replace all use phi 这个block里面对phi函数
    InstNode *currNode = block->head_node;
    while(currNode != block->tail_node){

        currNode = get_next_inst(currNode);
    }

    // schedule_copies
    HashSet *copySet = HashSetInit();

    HashMap *varReplace = HashMapInit();

    HashSet *used_by_another = HashSetInit();

    // for all successors
    if(block->true_block){
        InstNode *trueCurr = block->true_block->head_node;
        //第一个基本块不可能还有phi node 其他基本块应该都有label
        trueCurr = get_next_inst(trueCurr);
        while(trueCurr->inst->Opcode == Phi){
            Value *phiDest = ins_get_value(trueCurr->inst);
            //each src, dest
            HashSet *phiSet = phiDest->pdata->pairSet;
            HashSetFirst(phiSet);
            for(pair *phiInfo = HashSetNext(phiSet); phiInfo != NULL; phiInfo = HashSetNext(phiSet)){
                Value *src = phiInfo->define;
                if(src != NULL){
                    // 如果不是undefine
                    CopyPair *tempPair = createCopyPair(src,phiDest);
                    HashSetAdd(copySet,tempPair);
                    HashMapPut(varReplace,src,src);
                    HashMapPut(varReplace,phiDest,phiDest);
                    HashSetAdd(used_by_another,src);
                }
            }
            trueCurr = get_next_inst(trueCurr);
        }
    }

    if(block->false_block){
        InstNode *falseCurr = block->false_block->head_node;
        falseCurr = get_next_inst(falseCurr);
        while(falseCurr->inst->Opcode == Phi){
            Value *phiDest = ins_get_value(falseCurr->inst);
            HashSet *phiSet = phiDest->pdata->pairSet;
            HashSetFirst(phiSet);
            for(pair *phiInfo = HashSetNext(phiSet); phiInfo != NULL; phiInfo = HashSetNext(phiSet)){
                Value *src = phiInfo->define;
                if(src != NULL){
                    CopyPair *copyPair = createCopyPair(src, phiDest);
                    HashSetAdd(copySet,copyPair);
                    HashMapPut(varReplace,src,src);
                    HashMapPut(varReplace,phiDest,phiDest);
                    HashSetAdd(used_by_another,src);
                }
            }
            falseCurr = get_next_inst(falseCurr);
        }
    }

    HashSet *workList = HashSetInit();
    HashSetFirst(copySet);
    for(CopyPair *copyPair = HashSetNext(copySet); copyPair != NULL; copyPair = HashSetNext(copySet)){
        Value *dest = copyPair->dest;
        if(!HashSetFind(used_by_another,dest)){
            HashSetRemove(copySet,copyPair);
            HashSetAdd(workList,copyPair);//
        }
    }

    while(HashSetSize(workList) != 0 || HashSetSize(copySet) != 0){
        while(HashSetSize(workList) != 0){
            HashSetFirst(copySet);
            CopyPair *copyPair = HashSetNext(copySet);
            HashSetRemove(copySet, copyPair);


            //if dest -> live out of b
            //不管三七二十一 我们都去insert一个temp
            if(HashSetFind(block->out,copyPair->dest)){
                //insert a copy from dest to a new temp t at phi node defining dest;

                //push(t, Stack(dest))
                stack *destStack = HashMapGet(varStack,copyPair->dest);
                assert(destStack != NULL);

                //push t -> stack [dest]
            }

            //insert a copy operation from map[src] to dest at the end of
            Value *src = HashMapGet(varReplace,copyPair->src);
            insertCopies(block,copyPair->dest,src);

            // map[src] <- dest
            HashMapPut(varReplace,copyPair->src, copyPair->dest);

            HashSetFirst(copySet);
            for(CopyPair *tempPair = HashSetNext(copySet); tempPair != NULL; tempPair = HashSetNext(copySet)){
                if(tempPair->dest == src){
                    HashSetAdd(workList,tempPair);
                }
            }
        }

        if(HashSetSize(copySet) != 0){
            HashSetFirst(copySet);
            CopyPair *copyPair = HashSetNext(copySet);
            HashSetRemove(copySet,copyPair);

            //insert a copy from dest to a new temp t at the end of b

            // map[dest] <- t
            //HashMapPut(varReplace,copyPair->dest,);
            HashSetAdd(workList,copyPair);
        }
    }

    HashSetFirst(node->children);
    for(DomTreeNode *domNode = HashSetNext(node->children); domNode != NULL; domNode = HashSetNext(node->children)){
        insertPhiCopies(domNode,varStack);
    }

    // for each name n
    //for
}

InstNode *newCopyOperation(Value *src){
    // 此时挂载了
    Instruction *copyIns = ins_new_unary_operator(CopyOperation,src);
    assert(copyIns != NULL);
    copyIns->Opcode = CopyOperation;
    InstNode *copyInsNode = new_inst_node(copyIns);
    assert(copyInsNode != NULL);
    return copyInsNode;
}

void HashSetClean(HashSet *set){
    assert(set != NULL);
    HashSetFirst(set);
    for(void *key = HashSetNext(set); key != NULL; key = HashSetNext(set)){
        HashSetRemove(set,key);
    }
}

void calculateNonLocals(Function *currentFunction){
    BasicBlock *entry = currentFunction->entry;
    BasicBlock *end = currentFunction->tail;

    InstNode *currNode = entry->head_node;
    InstNode *tailNode = end->tail_node;

    clear_visited_flag(entry);

    nonLocals = HashSetInit();
    killed = HashSetInit();
    while(currNode != get_next_inst(tailNode)){
        BasicBlock *block = currNode->inst->Parent;
        if(block->visited == false){
            block->visited = true;
            // 每次都要重新计算killed
            HashSetClean(killed);
        }
        if(currNode->inst->Opcode == Load){
            Value *lhs = ins_get_lhs(currNode->inst);
            if(lhs != NULL && isLocalVar(lhs) && !HashSetFind(killed,lhs)){
                HashSetAdd(nonLocals,lhs);
            }
        }

        if(currNode->inst->Opcode == Store){
            Value *rhs = ins_get_rhs(currNode->inst);
            if(rhs != NULL && isLocalVar(rhs) && !HashSetFind(killed,rhs)){
                HashSetAdd(killed, rhs);
            }
        }
        currNode = get_next_inst(currNode);
    }

    HashSetFirst(nonLocals);
    printf("nonLocals: ");
    for(Value *nonLocalValue = HashSetNext(nonLocals); nonLocalValue != NULL; nonLocalValue = HashSetNext(nonLocals)){
        printf("%s ", nonLocalValue->name);
    }
    HashSetDeinit(killed);
}

// TODO 解决自引用的问题
bool correctPhiNode(Function *currentFunction){

    bool changed = false;
    BasicBlock *entry = currentFunction->entry;
    BasicBlock *end = currentFunction->tail;

    InstNode *currNode = entry->head_node;

    while(currNode != get_next_inst(end->tail_node)) {
        if(currNode->inst->Opcode == Phi && currNode->inst->user.value.use_list == NULL){
            InstNode *tempNode = entry->head_node;
            bool flag = false;
            Value *insValue = ins_get_value(currNode->inst);
            while(tempNode != get_next_inst(end->tail_node)){
                if(tempNode->inst->Opcode == Phi){
                    HashSet *pairSet = tempNode->inst->user.value.pdata->pairSet;
                    HashSetFirst(pairSet);
                    for(pair *phiInfo = HashSetNext(pairSet); phiInfo != NULL; phiInfo = HashSetNext(pairSet)){
                        Value *define = phiInfo->define;
                        if(define == insValue){
                            flag = true;
                        }
                    }
                }
                tempNode = get_next_inst(tempNode);
            }
            if(flag == true){
                currNode = get_next_inst(currNode);
            }else{
                printf("delete a dead phi\n");
                InstNode *next = get_next_inst(currNode);
                delete_inst(currNode);
                currNode = next;
                changed = true;
            }
        }else{
            currNode = get_next_inst(currNode);
        }
    }


    currNode = entry->head_node;
    while(currNode != get_next_inst(end->tail_node)){
        if(currNode->inst->Opcode == Phi){
            HashSet *phiSet = currNode->inst->user.value.pdata->pairSet;

            if(HashSetSize(phiSet) == 1){
                changed = true;
                HashSetFirst(phiSet);
                pair *phiInfo = HashSetNext(phiSet);
                Value *define = phiInfo->define;
                Value *insValue = ins_get_value(currNode->inst);
                value_replaceAll(insValue,define);


                // 还不对因为有可能在另一个phi函数里面引用到了这个变量所以我们还得替换！
                // 从头到尾去判断
                InstNode *tempNode = entry->head_node;
                while(tempNode != get_next_inst(end->tail_node)){
                    if(tempNode->inst->Opcode == Phi){
                        HashSet *pairSet = tempNode->inst->user.value.pdata->pairSet;
                        HashSetFirst(pairSet);
                        for(pair *phiInfo1 = HashSetNext(pairSet); phiInfo1 != NULL; phiInfo1 = HashSetNext(pairSet)){
                            if(phiInfo1->define == insValue){
                                phiInfo1->define = define;
                            }
                        }
                    }
                    tempNode = get_next_inst(tempNode);
                }

                InstNode *nextNode = get_next_inst(currNode);
                delete_inst(currNode);
                currNode = nextNode;
            }else if(HashSetSize(phiSet) == 0){
                changed = true;
                InstNode *nextNode = get_next_inst(currNode);
                delete_inst(currNode);
                currNode = nextNode;
            }else{
              currNode = get_next_inst(currNode);
            }
        }else{
            currNode = get_next_inst(currNode);
        }
    }


    //再解决
    currNode = entry->head_node;
    while(currNode != get_next_inst(end->tail_node)){
        if(currNode->inst->Opcode == Phi){
            Value *insValue = ins_get_value(currNode->inst);
            HashSet *pairSet = currNode->inst->user.value.pdata->pairSet;
            HashSetFirst(pairSet);
            unsigned int phiSize = HashSetSize(pairSet);
            unsigned int repeatSize = 0;
            for(pair *phiInfo = HashSetNext(pairSet); phiInfo != NULL; phiInfo = HashSetNext(pairSet)){
                if(phiInfo->define == insValue){
                    repeatSize++;
                }
            }
            if(phiSize - repeatSize == 1){
                changed = true;
                HashSetFirst(pairSet);
                for(pair *phiInfo = HashSetNext(pairSet); phiInfo != NULL; phiInfo = HashSetNext(pairSet)){
                    if(phiInfo->define == insValue){
                        HashSetRemove(pairSet,phiInfo);
                    }
                }
            }
        }
        currNode = get_next_inst(currNode);
    }
    if(changed) correctPhiNode(currentFunction);
}

CopyPair *createCopyPair(Value *src, Value *dest){
    CopyPair *copyPair = (CopyPair*)malloc(sizeof(CopyPair));
    assert(src != NULL);
    assert(dest != NULL);
    copyPair->src = src;
    copyPair->dest = dest;
    return copyPair;
}

void SSADeconstruction(Function *currentFunction){
    BasicBlock *entry = currentFunction->entry;
    BasicBlock *end = currentFunction->tail;

    clear_visited_flag(entry);
    InstNode *currNode = entry->head_node;
    while(currNode != end->tail_node){
        // 每个基本块只遍历一次
        BasicBlock *block = currNode->inst->Parent;
        if(!block->visited){
            block->visited = true;
            //跳过Label
            InstNode *blockNode = get_next_inst(block->head_node);
            // 如果当前基本块上存在phi函数
            if(blockNode->inst->Opcode == Phi) {
                HashSetFirst(block->preBlocks);
                for(BasicBlock *prevBlock = HashSetNext(block->preBlocks);
                     prevBlock != NULL; prevBlock = HashSetNext(block->preBlocks)) {
                    printf("currNode\n");
                    if (prevBlock->true_block && prevBlock->false_block) {// 是关键边
                        // 分割当前关键边
                        BasicBlock *newBlock = bb_create();
                        InstNode *prevTail = prevBlock->tail_node;
                        printf("currNode\n");
                        Instruction *newBlockLabel = ins_new_zero_operator(Label);
                        InstNode *newBlockLabelNode = new_inst_node(newBlockLabel);

                        //直接跳转语句
                        Instruction *newBlockBr = ins_new_zero_operator(br);
                        InstNode *newBlockBrNode = new_inst_node(newBlockBr);

                        ins_insert_after(prevTail, newBlockLabelNode);
                        ins_insert_after(newBlockLabelNode, newBlockBrNode);
                        // 维护这个基本块中的信息 TODO 没有维护支配信息等
                        bb_set_block(newBlock,newBlockLabelNode, newBlockBrNode);
                        newBlock->Parent = currentFunction;
                        newBlock->id = -2; // 如果是-2的话 表示是新的节点
                        // 前一个基本块到底是true 还是 false 是block
                        if (prevBlock->true_block == block) {
                            prevBlock->true_block = newBlock;
                        } else if (prevBlock->false_block == block) {
                            prevBlock->false_block = newBlock;
                        }
                        HashSetAdd(newBlock->preBlocks, prevBlock);
                        newBlock->true_block = block;
                        //移除原来那个边
                        HashSetRemove(block->preBlocks, prevBlock);
                        HashSetAdd(block->preBlocks, newBlock);
                        // 修改phiInfo里面的信息
                        InstNode *correctNode = get_next_inst(block->head_node);
                        while(correctNode->inst->Opcode == Phi){
                            Value *insValue = ins_get_value(correctNode->inst);
                            HashSet *phiSet = insValue->pdata->pairSet;
                            HashSetFirst(phiSet);
                            for(pair *phiInfo = HashSetNext(phiSet); phiInfo != NULL; phiInfo = HashSetNext(phiSet)){
                                if(phiInfo->from == prevBlock){
                                    //原来是来自prevBlock的更改为来自新的Block
                                    phiInfo->from = newBlock;
                                }
                            }
                            correctNode = get_next_inst(correctNode);
                        }
                    }else{
                        //不动
                    }
                }
            }
            //进行拷贝操作
            InstNode* phiNode = get_next_inst(block->head_node);
            while(phiNode->inst->Opcode == Phi){
                Value *insValue = ins_get_value(phiNode->inst);
                HashSet *phiSet = insValue->pdata->pairSet;
                HashSetFirst(phiSet);
                for(pair *phiInfo = HashSetNext(phiSet); phiInfo != NULL; phiInfo = HashSetNext(phiSet)){
                    Value *src = phiInfo->define;
                    if(src != NULL){
                        BasicBlock *from = phiInfo->from;
                        insertCopies(from,insValue,src);
                    }
                }
                InstNode *nextNode = get_next_inst(phiNode);
                delete_inst(phiNode);
                phiNode = nextNode;
            }
        }
        currNode = get_next_inst(currNode);
    }
}