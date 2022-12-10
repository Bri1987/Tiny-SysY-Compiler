#include "bblock.h"
#include "stdio.h"
void bblock_init(BasicBlock* this, Function* func) {
    //对头上的两个后继节点另外开辟use
    uint8_t *storage = (uint8_t *) malloc(sizeof(Use) * 2);
    Use *temp1 = (Use *) storage;
    temp1->Parent = &this->user;

    Use *temp2 = (Use *) (storage) + 1;
    temp2->Parent = &this->user;
    //
    this->user.use_list = (Use *) storage;
    value_init(&this->user.value);
    this->Parent = func;
    this->head_node = NULL;
    this->tail_node = NULL;
}


//// only used by bblock
InstNode* new_inst_node(Instruction* inst){
    InstNode* n = malloc(sizeof(InstNode));
    n->inst = inst;
    // 初始化结点
    sc_list_init(&(n->list));
    return n;
}

InstNode* bblock_get_inst_back(BasicBlock* this){
    return this->tail_node;
}

InstNode* bblock_get_inst_head(BasicBlock* this){
    return this->head_node;
}

Function *bblock_get_parent(BasicBlock* this){
    return this->Parent;
}

void moveBefore(BasicBlock *this,BasicBlock *MovePos){
    Use *temp = this->user.use_list;
    if(temp->Val == NULL){
        use_set_value(temp,&MovePos->user.value);
    }else{
        temp = temp + 1;
        if(temp->Val == NULL) {
            use_set_value(temp,&MovePos->user.value);
        }else{
            printf("already have two kids!\n");
        }
    }
}

void moveAfter(BasicBlock *this,BasicBlock *MovePos){
    Use *temp = MovePos->user.use_list;
    if(temp->Val == NULL){
        use_set_value(temp,&this->user.value);
    }else{
        temp = temp + 1;
        if(temp->Val == NULL){
            use_set_value(temp,&this->user.value);
        }else{
            printf("already have two kids!\n");
        }
    }
}

InstNode *get_prev_inst(InstNode *this){
    struct sc_list *list = this->list.prev;
    InstNode *temp = sc_list_entry(list,InstNode,list);
    return temp;
}

InstNode *get_next_inst(InstNode *this){
    struct sc_list *list = this->list.next;
    InstNode *temp = sc_list_entry(list,InstNode,list);
    return temp;
}

InstNode *search_inst_node(InstNode *head,int id){
    while(head != NULL){
        if(head->inst->i == id)
            return head;
        else
            head = get_next_inst(head);
    }
}

// 注意程序的第一条语句需要提前处理
void bblock_divide(InstNode *head){
    InstNode *temp = head;
    while(temp != NULL){
        if(temp->inst->Opcode == Goto
                || temp->inst->Opcode == IF_Goto){
            temp->inst->user.value.is_out = 1;
            InstNode *Next = get_next_inst(temp);
            Next->inst->user.value.is_in = 1;
            int id = temp->inst->user.value.pdata->var_pdata.iVal;

            InstNode *Go = search_inst_node(head,id);
            Go->inst->user.value.is_in = 1;

            InstNode *Go_Prev = get_prev_inst(Go);
            Go_Prev->inst->user.value.is_out = 1;
        }
        if(temp->inst->Opcode == Return){
            temp->inst->user.value.is_out = 1;
        }
        temp = get_next_inst(temp);
    }


    temp = head;
    InstNode *this_in = temp;
    while(temp != NULL){
        if(temp->inst->user.value.is_out == (unsigned int)1){
            BasicBlock* tempBlock = (BasicBlock*)malloc(sizeof(BasicBlock));
            bblock_init(tempBlock,NULL);
            tempBlock->head_node = this_in;
            tempBlock->tail_node = temp;
            bb_set_block(tempBlock,this_in,temp);
            this_in = get_next_inst(temp);
        }
        temp = get_next_inst(temp);
    }

    temp = head;
    BasicBlock *now = temp->inst->Parent;
    BasicBlock *next = NULL;
    BasicBlock *jump = NULL;
    /* 直接的儿子放在第一个use的第一条边，如果有Goto的话放在第二条边 */
    while(temp != NULL){
        if(temp->inst->user.value.is_out == (unsigned)1){
            next = get_next_inst(temp)->inst->Parent;
            moveBefore(now,next);
            now = next;
        }
        if(temp->inst->Opcode == Goto || temp->inst->Opcode == IF_Goto){
            jump = search_inst_node(head,temp->inst->user.value.pdata->instruction_pdata.goto_location)->inst->Parent;
            moveBefore(now,jump);
        }
        temp = get_next_inst(temp);
    }
}


void bb_set_block(BasicBlock *this,InstNode *head,InstNode *tail){
    for(;head != tail;head = get_next_inst(head)){
        ins_set_parent(head->inst,this);
    }
    ins_set_parent(tail->inst,this);
}

void ll_analysis(InstNode *tail){
//    while(tail != head){
//         int num = tail->inst->user.value.NumUserOperands;
//         for(int i = 0; i < num; i++){
//             Use *temp = tail->inst->user.use_list + i;
//             Value *this = temp->Val;
//             Value *def = (Value*)tail->inst;
//             ll_isexsit()
//         }
//         tail = get_prev_inst(tail);
//    }
}

void ins_node_add(InstNode *head,InstNode *this){
    InstNode *temp = head;
    for(;temp->list.next != &head->list; temp = get_next_inst(temp)){
    }
    sc_list_add_tail(&head->list,&this->list);
}