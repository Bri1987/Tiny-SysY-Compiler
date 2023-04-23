//
// Created by Valerian on 2023/1/18.
//
#include "dominance.h"
HashMap *mapping; // 不想再改数据结构了 就这样将就用吧

//只针对BasicBlock类型
void HashSetCopy(HashSet *dest,HashSet *src){
    HashSetFirst(src);
    for(BasicBlock *key = HashSetNext(src); key != NULL; key = HashSetNext(src)){
        if(!HashSetFind(dest,key)){
            HashSetAdd(dest,key);
        }
    }
}

bool HashSetDifferent(HashSet *lhs,HashSet *rhs){
    //左边是小的 右边是大的
    unsigned int leftSize = HashSetSize(lhs);
    unsigned int rightSize = HashSetSize(rhs);
    if(leftSize != rightSize) return true;
    HashSetFirst(lhs);
    for(BasicBlock *key = HashSetNext(lhs); key != NULL; key = HashSetNext(lhs)){
        if(!HashSetFind(rhs,key)) return true;
    }
    return false;
}

DomTreeNode *createDomTreeNode(BasicBlock *block,BasicBlock *parent){
    DomTreeNode *domTreeNode = (DomTreeNode*)malloc(sizeof(DomTreeNode));
    memset(domTreeNode,0,sizeof(DomTreeNode));
    domTreeNode->children = HashSetInit();
    domTreeNode->block = block;
    domTreeNode->parent = parent;
    block->domTreeNode = domTreeNode;
    return domTreeNode;
}


void calculate_dominance(Function *currentFunction) {
    // 注意要删除不可达的BasicBlock
    BasicBlock *entry = currentFunction->entry;
    entry->dom = HashSetInit();
    HashSetAdd(entry->dom,entry);

    HashSet *allNode = HashSetInit();
    //拿到第一条InstNode
    BasicBlock *prev = nullptr;
    InstNode *cur = entry->head_node;
    InstNode *end = currentFunction->tail->tail_node;
    while(cur != end){
        BasicBlock *parent = cur->inst->Parent;
        if(prev != parent){
            HashSetAdd(allNode,parent);
        }
        cur = get_next_inst(cur);
    }
    BasicBlock *endBlock = end->inst->Parent;
    if(!HashSetFind(allNode,endBlock)){
        HashSetAdd(allNode,endBlock);
    }
    HashSetFirst(allNode);
    cur = entry->head_node;
    while(cur != end){
        BasicBlock *block = cur->inst->Parent;
        if(block != entry && block->dom == NULL){
            block->dom = HashSetInit();
        }
        if(!block->visited && block != entry){
            HashSetFirst(allNode);
            printf("addBlock :");
            for(BasicBlock *addBlock = HashSetNext(allNode); addBlock != NULL; addBlock = HashSetNext(allNode)){
                printf(" b%d",addBlock->id);
                HashSetAdd(block->dom,addBlock);
            }
            printf("\n");
            block->visited = true;
        }
        cur = get_next_inst(cur);
    }

    printf("right before all\n");

    //全集计算完毕
    bool changed = true;
    while(changed){
        changed = false;
        //对于除了entry外的每一个block
        cur = currentFunction->entry->head_node;


        printf("iterate begin!\n");

        clear_visited_flag(cur->inst->Parent);

        printf("cleared all visited flag!\n");
        while(cur != end){
            BasicBlock *parent = cur->inst->Parent;
            //除了头节点

            if(parent->visited != true && parent != entry){
                //
                printf("current at : b%d",parent->id);
                parent->visited = true;

                //新建一个hashset
                HashSet *newSet = HashSetInit();

                //让它为全集
                HashSetCopy(newSet,allNode);

                //开始迭代
                //找到前驱节点
                HashSet *prevBlocks = get_prev_block(parent);

                HashSetFirst(prevBlocks);

                //对于所有前驱节点
                printf(" prevBlocks:");
                for(BasicBlock *prevBlock = HashSetNext(prevBlocks); prevBlock != NULL; prevBlock = HashSetNext(prevBlocks)){
                    printf(" b%d",prevBlock->id);
                    HashSet *prevDom = prevBlock->dom;
                    newSet = HashSetIntersect(newSet,prevDom);
                }
                printf(" ");

                HashSetFirst(newSet);

                //并自己
                HashSetAdd(newSet,parent);

                printf("newSet contain:");
                for(BasicBlock *key = HashSetNext(newSet);key != NULL; key = HashSetNext(newSet)){
                    printf(" b%d",key->id);
                }

                //判断跟现在的集合是否有差别
                changed = HashSetDifferent(newSet,parent->dom);

                if(changed){
                    HashSetDeinit(parent->dom);
                    parent->dom = newSet;
                }else{
                    HashSetDeinit(newSet);
                }
                printf(" changed : %d\n",changed);
            }
            cur = get_next_inst(cur);
        }
        printf("This round changed : %d\n",changed);
    }
    printf("after dominance!\n");
    HashSetDeinit(allNode);
}

void calculate_dominance_frontier(Function *currentFunction){
    BasicBlock *entry = currentFunction->entry;
    BasicBlock *end = currentFunction->tail;

    if(entry == end){
        entry->df = HashSetInit();
        return;
    }
    HashSet *allBlocks = HashSetInit();
    //
    clear_visited_flag(entry);

    //
    InstNode *currNode = entry->head_node;
    while(currNode != get_next_inst(end->tail_node)){
        BasicBlock *block = currNode->inst->Parent;
        if(block->visited == false){
            block->visited = true;
            if(!HashSetFind(allBlocks,block)){
                HashSetAdd(allBlocks,block);
            }
        }
        currNode = get_next_inst(currNode);
    }

    //还需要一个tempSet
    HashSet *tempSet = HashSetInit();
    HashSetFirst(allBlocks);
    for(BasicBlock *block = HashSetNext(allBlocks); block != NULL; block = HashSetNext(allBlocks)){
        HashSetAdd(tempSet,block);
    }

    //
    HashSetFirst(allBlocks);
    for(BasicBlock *X = HashSetNext(allBlocks); X != NULL; X = HashSetNext(allBlocks)){
        X->df = HashSetInit();
        HashSetFirst(tempSet);
        for(BasicBlock *Y = HashSetNext(tempSet); Y != NULL; Y = HashSetNext(tempSet)){
            if(HashSetFind(Y->dom,X) && Y->true_block != NULL && !HashSetFind(Y->true_block->dom,X)){
                HashSetAdd(X->df,Y->true_block);
            }
            if(HashSetFind(Y->dom,X) && Y->false_block != NULL && !HashSetFind(Y->false_block->dom,X)){
                HashSetAdd(X->df,Y->false_block);
            }
        }
    }

    HashSetFirst(allBlocks);
    for(BasicBlock *block = HashSetNext(allBlocks); block != NULL; block = HashSetNext(allBlocks)){
        HashSetFirst(block->df);
        printf("b%d df : ",block->id);
        for(BasicBlock *df = HashSetNext(block->df); df != NULL; df = HashSetNext(block->df)){
            printf("b%d ",df->id);
        }
        printf("\n");
    }
    HashSetDeinit(allBlocks);
    HashSetDeinit(tempSet);
    clear_visited_flag(entry);
}

void calculate_iDominator(Function *currentFunction){
    BasicBlock *entry = currentFunction->entry;
    BasicBlock *end = currentFunction->tail;

    //不动的
    InstNode *head = entry->head_node;
    InstNode *tail = end->tail_node;

    clear_visited_flag(head->inst->Parent);
    //从头来计算
    InstNode *curNode = head;
    while(curNode != get_next_inst(tail)){
        BasicBlock *curBlock = curNode->inst->Parent;

        if(curBlock->visited) {
            curNode = get_next_inst(curNode);
        }else{
            curBlock->visited = true;
            HashSet *tempSet = HashSetInit();
            HashSetCopy(tempSet,curBlock->dom);
            //去掉本身
            HashSetRemove(tempSet,curBlock);
            HashSetFirst(curBlock->dom);
            for(BasicBlock *key = HashSetNext(curBlock->dom);key != NULL; key = HashSetNext(curBlock->dom)){
                //除去现在这个
                if(key == curBlock) continue;

                HashSet *keySet = key->dom;
                HashSetFirst(tempSet);
                for(BasicBlock *key1 = HashSetNext(tempSet); key1 != NULL; key1 = HashSetNext(tempSet)){
                    if(key1 != key && HashSetFind(keySet,key1)){
                        HashSetRemove(tempSet,key1);
                    }
                }
            }
            HashSetFirst(tempSet);
            curBlock->iDom = NULL;
            for(BasicBlock *key = HashSetNext(tempSet); key != NULL; key = HashSetNext(tempSet)){
                printf("b%d, idom: b%d\n",curBlock->id,key->id);
                curBlock->iDom = key;
            }

            HashSetDeinit(tempSet);
            curNode = get_next_inst(curNode);
        }
    }
    clear_visited_flag(entry);
}

void DomTreeAddChild(DomTreeNode *parent, DomTreeNode *child){
    HashSetAdd(parent->children, child);
}

void calculate_DomTree(Function *currentFunction){
    BasicBlock *entry = currentFunction->entry;
    BasicBlock *end = currentFunction->tail;

    printf("entryBlock : %d",entry->id);
    printf(" endBlock : %d",end->id);
    printf("\n");
    //不动的
    InstNode *head = entry->head_node;
    InstNode *tail = end->tail_node;

    // 先对每一个BasicBlock进行建立DomNode
    clear_visited_flag(entry);

    InstNode *curNode = head;
    while(curNode != get_next_inst(tail)){
        BasicBlock *block = curNode->inst->Parent;
        if(block->visited == false){
            block->visited = true;
            assert(block->domTreeNode == NULL);
            block->domTreeNode = (DomTreeNode*)malloc(sizeof(DomTreeNode));
            memset(block->domTreeNode,0,sizeof(DomTreeNode));
            block->domTreeNode->block = block;
            block->domTreeNode->children = HashSetInit();
        }
        curNode = get_next_inst(curNode);
    }

    //从头来计算
    clear_visited_flag(head->inst->Parent);
    curNode = head;
    while(curNode != get_next_inst(tail)){
        BasicBlock *block = curNode->inst->Parent;
        if(block->visited == false){
            block->visited = true;
            BasicBlock *iDom = block->iDom;
            DomTreeNode *domTreeNode = block->domTreeNode;
            domTreeNode->parent = iDom;
            if(iDom != NULL){
                DomTreeNode *prevNode = iDom->domTreeNode;
                assert(prevNode != NULL);
                DomTreeAddChild(prevNode, domTreeNode);
                //printf("b%d add child b%d\n",prevNode->block->id,block->id);
                assert(HashSetFind(prevNode->children,domTreeNode));
            }
        }
        curNode = get_next_inst(curNode);
    }


    // 检查DomTree的构建是否成功
    InstNode *checkNode = head;

    //Function的跟节点保留
    currentFunction->root = entry->domTreeNode;
    assert(entry->domTreeNode != nullptr);

    DomTreePrinter(entry->domTreeNode);
    clear_visited_flag(checkNode->inst->Parent);
    while(checkNode != get_next_inst(tail)){
        BasicBlock *parent = checkNode->inst->Parent;
        if(parent->visited == false){
            parent->visited = true;
            DomTreeNode *domTreeNode = parent->domTreeNode;
            printf("b%d ",parent->id);
            if(parent->iDom != nullptr)
                printf("idom : b%d ",parent->iDom->id);
            HashSet *childSet = domTreeNode->children;
            HashSetFirst(childSet);
            for(DomTreeNode *childNode = HashSetNext(childSet); childNode != NULL; childNode = HashSetNext(childSet)){
                printf("b%d ",childNode->block->id);
            }
            printf("\n");
        }
        checkNode = get_next_inst(checkNode);
    }
}

void DomTreePrinter(DomTreeNode *root){
    if(HashSetSize(root->children) == 0){
        return;
    }
    HashSetFirst(root->children);
    for(DomTreeNode *key = HashSetNext(root->children); key != nullptr; key = HashSetNext(root->children)){
        BasicBlock *block = key->block;
        printf("block:%d",block->id);
        DomTreePrinter(key);
    }
    return;
}

void calculatePostDominance(Function *currentFunction){
    //还是先记录全集
    BasicBlock *entry = currentFunction->entry;

    clear_visited_flag(entry);

    BasicBlock *tail = currentFunction->tail;
    InstNode *exitNode = tail->tail_node;
    while(exitNode->inst->Opcode != Return){
        exitNode = get_prev_inst(exitNode);
    }

    // tail 不一定是exit 需要找到exit
    BasicBlock *exit = exitNode->inst->Parent;

    HashSet *allBlocks = HashSetInit();
    HashSet *workList = HashSetInit();
    HashSetAdd(workList,entry);

    //bfs 一下
    while(HashSetSize(workList) != 0){
        HashSetFirst(workList);
        BasicBlock *block = HashSetNext(workList);
        HashSetRemove(workList,block);
        HashSetAdd(allBlocks,block);
        //当前基本块一定是没有遍历过的
        block->visited = true;

        //把内存开了
        block->pDom = HashSetInit();
        block->rdf = HashSetInit();

        if(block->true_block && block->true_block->visited == false){
            HashSetAdd(workList,block->true_block);
        }
        if(block->false_block && block->false_block->visited == false){
            HashSetAdd(workList,block->false_block);
        }
    }
    HashSetAdd(exit->pDom,exit);

    HashSetFirst(allBlocks);
    HashSet *tempSet = HashSetInit();
    HashSetCopy(tempSet,allBlocks);
    HashSetFirst(allBlocks);
    for(BasicBlock *tempBlock = HashSetNext(allBlocks); tempBlock != NULL; tempBlock = HashSetNext(allBlocks)){
        //除了exit
        if(tempBlock != exit){
            HashSetCopy(tempBlock->pDom,tempSet);
        }
    }


    bool changed = true;
    while(changed){
        changed = false;
        HashSetFirst(allBlocks);
        for(BasicBlock *block = HashSetNext(allBlocks); block != NULL; block = HashSetNext(allBlocks)){
            //

            if(block != exit){
                HashSet *newSet = HashSetInit();
                HashSetCopy(newSet,tempSet);

                //for each successor block p
                if(block->true_block){
                    newSet = HashSetIntersect(newSet,block->true_block->pDom);
                }

                if(block->false_block){
                    newSet = HashSetIntersect(newSet,block->false_block->pDom);
                }

                HashSetAdd(newSet,block);

                if(HashSetDifferent(newSet,block->pDom)){
                    //如果不同 释放原来的

                    printf("different!\n");
                    HashSetDeinit(block->pDom);
                    block->pDom = newSet;
                    changed |= true;


                    printf("block %d",block->id);
                    printf("pdom :");
                    HashSetFirst(newSet);
                    for(BasicBlock *block = HashSetNext(newSet); block != NULL; block = HashSetNext(newSet)){
                        printf("b%d ",block->id);
                    }
                    printf("\n");
                }else{
                    HashSetDeinit(newSet);
                }
            }
        }
    }


    HashSetFirst(allBlocks);
    for(BasicBlock *block = HashSetNext(allBlocks); block != NULL; block = HashSetNext(allBlocks)){
        //打印看看求的对不对
        printf("block b%d : pdom : ",block->id);
        HashSetFirst(block->pDom);
        for(BasicBlock *pDomBlock = HashSetNext(block->pDom); pDomBlock != NULL; pDomBlock = HashSetNext(block->pDom)){
            printf("b%d",pDomBlock->id);
        }
        printf("\n");
    }

    //好好好现在是对的了 我们继续求rdf
    HashSetFirst(allBlocks);
    for(BasicBlock *X = HashSetNext(allBlocks); X != NULL; X = HashSetNext(allBlocks)){
        //
        HashSetFirst(tempSet);
        for(BasicBlock *Y = HashSetNext(tempSet); Y != NULL; Y = HashSetNext(tempSet)){
            // 如果Y->pDom 含有X
            if(HashSetFind(Y->pDom,X)){
                // Y 的preds里面的Z又没有X的就是X的reverse dominance froniter
                HashSetFirst(Y->preBlocks);
                for(BasicBlock *Z = HashSetNext(Y->preBlocks); Z != NULL; Z = HashSetNext(Y->preBlocks)){
                    if(!HashSetFind(Z->pDom,X)){
                        // 那么就代表了 X的RDF里面
                        HashSetAdd(X->rdf, Z);

                    }
                }
            }
        }
    }


    HashSetFirst(allBlocks);
    for(BasicBlock *block = HashSetNext(allBlocks); block != NULL; block = HashSetNext(allBlocks)){
        //
        printf("block b%d rdfs:",block->id);
        HashSetFirst(block->rdf);
        for(BasicBlock *rdf = HashSetNext(block->rdf); rdf != NULL; rdf = HashSetNext(block->rdf)){
            // 这样打印一下reverse dominance frontier
            printf("b%d",rdf->id);
        }

        printf("\n");
    }



    // 接下来我们再来求一下 post immediate dominance


    HashSetDeinit(tempSet);
}

void calculatePostDominanceTree(){

}
