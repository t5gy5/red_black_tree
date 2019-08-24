#include <stdlib.h>
#include "rbtree.h"


#define __CAST__ (type,p) ((type)(p))
#define __CLEAR__(itr) ((RBNode*)((size_t)(itr) & BLACK_MASK))
#define _COLOR_(itr) ((size_t)((itr)->parent) & 1)
#define _SET_RED_(itr) (itr)->parent = (RBNode*)((size_t)((itr)->parent) | RED_MASK)
#define _SET_BLACK_(itr) (itr)->parent = (RBNode*)((size_t)((itr)->parent) & BLACK_MASK)

typedef struct RBNode_t{
    struct RBNode_t *left,*right,*parent;
    void* data;
} RBNode;

static size_t BLACK_MASK = ~1, RED_MASK = 1;

typedef struct RBTree_t{
    RBNode *m_root;
    size_t m_size;
    const Key* (*get_key)(const Object*);
    int (*compare)(const Key*,const Key*);
} RBTree;

enum Direction{LEFT = 0,RIGHT = 1};
enum Color {BLACK = 0,RED = 1};

RBTree __temp_tree;
RBNode __temp_node;
const size_t RBT_SIZE_OFFSET  = (size_t)(&(__temp_tree.m_size)) - (size_t)(&__temp_tree);
const size_t RBT_LEFT_OFFSET  = (size_t)(&(__temp_node.left  )) - (size_t)(&__temp_node);
const size_t RBT_RIGHT_OFFSET = (size_t)(&(__temp_node.right )) - (size_t)(&__temp_node);

void RBT_repair_tree_insert (RBNode*);
void RBT_repair_tree_extract(RBNode*);
int  RBT_insert_node(RBTree*, RBNode**, Object*, RBNode*);
void RBT_delete_node(RBNode*);


RBTree* get_RBTree(const Key* (*get_key)(const Object*), int (*compare)(const Key*,const Key*)){
    RBTree* This = (RBTree*)malloc(sizeof( RBTree));
    *This = {NULL,0,get_key,compare};
    return This;
}

size_t RBT_size(RBTree* tree){
    return tree->m_size;
}


#define __RBT__ADVANCE_NODE__(itr,left,right)\
    if(itr->right){\
        itr = itr->right;\
        while(itr->left) itr = itr->left;\
    }else{\
        RBNode* temp_parent = __CLEAR__(itr->parent);\
        while(temp_parent && temp_parent->right == itr){\
            itr = temp_parent;\
            temp_parent = __CLEAR__(itr->parent);\
        }\
        itr = temp_parent;\
    }\
    return itr

RBNode* __advance_node_forward (RBNode* itr){
    __RBT__ADVANCE_NODE__(itr,left,right);
}
RBNode* __advance_node_backward(RBNode* itr){
    __RBT__ADVANCE_NODE__(itr,right,left);
}
#undef __RBT__ADVANCE_NODE__

int RBT_advance(RBT_Iterator* itr,int direction){
    if(itr->data && direction){
        if(direction>0){
            itr->current = __advance_node_forward (itr->current);
        }else if(direction <0){
            itr->current = __advance_node_backward(itr->current);
        }
        if(itr->current){
            itr->data = itr->current->data;
        } else {
            itr->data = NULL;
        }
        return 1;
    }else{
        return 0;
    }
}

#undef __ITERATOR_ADVANCE_INORDER_

#define ___GET_ITERATOR__(dir)\
    RBNode* itr = tree->m_root;\
    if(itr){\
        while(itr->dir) itr = itr->dir;\
        return {itr->data,itr};\
    }else{\
        return {NULL,NULL};\
    }

RBT_Iterator RBT_begin(RBTree* tree){
    ___GET_ITERATOR__(left)
}
RBT_Iterator RBT_rbegin(RBTree*tree){
    ___GET_ITERATOR__(right)
}
RBT_Iterator RBT_end(RBTree*){
    return {NULL,NULL};
}
RBT_Iterator RBT_rend(RBTree*){
    return {NULL,NULL};
}
#undef ___GET_ITERATOR__


Object* RBT_at(RBTree* tree, const Key* key){
    RBNode* itr = tree->m_root;
    while(itr){
        int compare = tree->compare(key,tree->get_key(itr->data));
        if(compare < 0){
            itr = itr->left;
        }else if (compare > 0) {
            itr = itr->right;
        }else{
            return itr->data;
        }
    }
    return NULL;
}
RBT_Iterator RBT_search(RBTree* tree, const Key* key){
    RBNode* itr = tree->m_root;
    RBNode* parent = NULL;
    Direction direction = LEFT;
    while(itr){
        parent = itr;
        int compare = tree->compare(key,tree->get_key(itr->data));
        if(compare < 0){
            itr = itr->left;
            direction = LEFT;
        }else if (compare > 0) {
            itr = itr->right;
            direction = RIGHT;
        }else{
            return {itr->data,itr};
        }
    }
    return {NULL, (RBNode*)((size_t)parent | direction | 2)};
}




Object* RBT_insert(RBTree* tree, Object* obj){
    RBNode** itr = &(tree->m_root);
    RBNode* parent = NULL;
    while(*itr){
        parent = *itr;
        int compare = tree->compare(tree->get_key(obj),tree->get_key(parent->data));
        if(compare < 0){
            itr = &(parent->left);
        }else if (compare > 0) {
            itr = &(parent->right);
        }else{
            return parent->data;
        }
    }
    return RBT_insert_node(tree,itr,obj,parent)? obj : NULL;
}
Object* RBT_insert_itr(RBTree* tree, RBT_Iterator ptr, Object* obj){
    size_t option_et_ptr = (size_t)(ptr.current);
    if(ptr.data == NULL && (option_et_ptr & 2)){
        ///itr comes from RBT_search with the location of insertion
        RBNode* itr_p = (RBNode*)(option_et_ptr & (~3));///future parent

        RBNode** itr;
        if(itr_p){
            itr = ((option_et_ptr & 1)? &(itr_p->right): &(itr_p->left));
        }else{
            itr = &(tree->m_root);
        }
        return RBT_insert_node(tree,itr,obj,itr_p) ? obj : NULL;
    }else{///Iterator doesn't come from RBT_search
        return NULL;
    }
}

int RBT_insert_node(RBTree* tree,RBNode**itr, Object* obj,RBNode* parent){
    *itr = (RBNode*)malloc(sizeof(RBNode));
    if(*itr){
        ++tree->m_size;
        (**itr) = {NULL, NULL, parent, obj};
        if(parent){
            _SET_RED_(*itr);
            RBT_repair_tree_insert(*itr);
        }
    }else{///Out of memory
        return 0;
    }
    return 1;
}

void RBT_delete_node(RBNode* itr){
    ///itr points to the to be deleted node
    RBNode* temp = itr->right;
    if(temp){
        ///swap with smallest element in right sub tree
        while(temp->left) temp = temp->left;
        itr->data = temp->data;
        itr = temp;
    }
    ///currently itr has at most 1 child
    temp = (itr->left ? itr->left : itr->right);
    if(temp){
        ///swap with child
        itr->data = temp->data;
        itr = temp;
    }
    ///currently itr has no children
    temp = __CLEAR__(itr->parent);
    if(temp){///itr isn't root
        ///detach itr from parent, itr may have a sibling
        if(temp->left == itr){
            temp->left = NULL;
        }else{
            temp->right = NULL;
        }
        ///if itr is RED skip, otherwise repair
        if(_COLOR_(itr) == BLACK){
            RBT_repair_tree_extract(temp);
        }
    }
    free(itr);
}

Object* RBT_exract(RBTree* tree, const Key* key){
    ///Find node
    Object* ret = NULL;
    RBNode* itr = tree->m_root;
    while(itr){
        int compare = tree->compare(key,tree->get_key(itr->data));
        if(compare<0){
            itr = itr->left;
        }else if(compare>0){
            itr = itr->right;
        }else{
            ret = itr->data;
            --tree->m_size;
            RBT_delete_node(itr);
            return ret;
        }
    }
    return NULL;
}
Object* RBT_exract_itr(RBTree* tree, RBT_Iterator itr){
    if(itr.data && itr.current){
        --tree->m_size;
        RBT_delete_node(itr.current);
    }else{
        return NULL;
    }
    return itr.data;
}
/**
* takes the pointer to the inserted node
* and repairs the tree
*/
void RBT_repair_tree_insert(RBNode* itr){

}
/**
* takes the pointer to the parent of the
* deleted node and repairs the tree
*/
void RBT_repair_tree_extract(RBNode* itr){
}
