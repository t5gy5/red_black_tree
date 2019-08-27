#include <stdlib.h>
#include "rbtree.h"


#define __CAST__ (type,p) ((type)(p))
#define __CLEAR__(itr) ((RBNode*)((size_t)(itr) & BLACK_MASK))
#define _COLOR_(itr) ((size_t)((itr)->parent) & 1)
#define _SET_RED_(itr) (itr)->parent = (RBNode*)((size_t)((itr)->parent) | RED_MASK)
#define _SET_BLACK_(itr) (itr)->parent = (RBNode*)((size_t)((itr)->parent) & BLACK_MASK)
#define __SET_PARENT__(itr,ptr) (itr)->parent = (RBNode*)(_COLOR_(itr)|(size_t)(ptr))
#define _SET_COLOR_(itr,color) (itr)->parent = (RBNode*)((size_t)((itr)->parent) | color)

typedef struct RBNode_t{
    struct RBNode_t *left,*right,*parent;
    void* data;
} RBNode;

static size_t BLACK_MASK = ~1, RED_MASK = 1;

typedef struct RBTree_t{
    RBNode *m_root,*spares;
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


int  RBT_insert_node(RBTree*, RBNode**, Object*, RBNode*);
void RBT_delete_node(RBTree*,RBNode*);
void RBT_repair_tree_insert (RBNode**, RBNode*);
static inline void RBT_repair_tree_extract(RBNode** root, RBNode* itr);


RBTree* RBT_get(const Key* (*get_key)(const Object*), int (*compare)(const Key*,const Key*)){
    RBTree* This = (RBTree*)malloc(sizeof( RBTree));
    *This = (RBTree){NULL,NULL,0,get_key,compare};
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


static inline RBNode* RBT_advance_node_forward (RBNode* itr){
    __RBT__ADVANCE_NODE__(itr,left,right);
    return itr;
}
static inline RBNode* RBT_advance_node_backward(RBNode* itr){
    __RBT__ADVANCE_NODE__(itr,right,left);
    return itr;
}
/**
* direction != 0
* positive gives forward, negative gives backward(in order)
*/
RBNode* RBT_advance_node(RBNode* itr,int direction){
    size_t backward,forward;
    if(direction > 0){
        backward = RBT_LEFT_OFFSET; forward = RBT_RIGHT_OFFSET;
    }else{
        backward = RBT_RIGHT_OFFSET; forward = RBT_LEFT_OFFSET;
    }

    RBNode *temp_parent = itr;
    if( (itr = *(RBNode**)((size_t)(itr)+forward)) ){
        while( (temp_parent = itr, itr = *(RBNode**)((size_t)(itr)+backward)));
    }else{
        itr = temp_parent;
        temp_parent = __CLEAR__(itr->parent);
        while(temp_parent && itr == *(RBNode**)((size_t)(temp_parent)+forward)){
            itr = temp_parent;
            temp_parent = __CLEAR__(itr->parent);
        }
    }
    return temp_parent;
}

int RBT_advance(RBT_Iterator* itr,int direction){
    if(itr->data && direction && itr->current){
        itr->current = RBT_advance_node(itr->current,direction);
        itr->data = (itr->current? itr->current->data:NULL);
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
        return (RBT_Iterator){itr->data,itr};\
    }else{\
        return (RBT_Iterator){NULL,NULL};\
    }

RBT_Iterator RBT_begin(RBTree* tree){
    ___GET_ITERATOR__(left)
}
RBT_Iterator RBT_rbegin(RBTree*tree){
    ___GET_ITERATOR__(right)
}
RBT_Iterator RBT_end(RBTree*tree){
    return (RBT_Iterator){NULL,NULL};
}
RBT_Iterator RBT_rend(RBTree*tree){
    return (RBT_Iterator){NULL,NULL};
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
    size_t direction = LEFT;
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
            return (RBT_Iterator){itr->data,itr};
        }
    }
    return (RBT_Iterator){NULL, (RBNode*)((size_t)parent | direction | 2)};
}

Object* RBT_insert(RBTree* tree, Object* obj){
    RBNode** itr = &(tree->m_root);
    RBNode* parent = NULL;
    const Key* key = tree->get_key(obj);
    while(*itr){
        parent = *itr;
        int compare = tree->compare(key,tree->get_key(parent->data));
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

Object* RBT_exract(RBTree* tree, const Key* key){
    //Find node
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
            RBT_delete_node(tree,itr);
            return ret;
        }
    }
    return NULL;
}

Object* RBT_insert_itr(RBTree* tree, RBT_Iterator ptr, Object* obj){
    size_t option_et_ptr = (size_t)(ptr.current);
    if(ptr.data == NULL && (option_et_ptr & 2)){
        //itr comes from RBT_search with the location of insertion
        RBNode* itr_p = (RBNode*)(option_et_ptr & (~3));//future parent

        RBNode** itr;
        if(itr_p){
            itr = ((option_et_ptr & 1)? &(itr_p->right): &(itr_p->left));
        }else{
            itr = &(tree->m_root);
        }
        return RBT_insert_node(tree,itr,obj,itr_p) ? obj : NULL;
    }else{//Iterator doesn't come from RBT_search
        return NULL;
    }
}


Object* RBT_exract_itr(RBTree* tree, RBT_Iterator itr){
    if(itr.data && itr.current){
        RBT_delete_node(tree,itr.current);
    }else{
        return NULL;
    }
    return itr.data;
}

int RBT_insert_node(RBTree* tree,RBNode**itr, Object* obj,RBNode* parent){
    *itr = (RBNode*)malloc(sizeof(RBNode));
    if(*itr){
        ++tree->m_size;
        (**itr) = (RBNode){NULL, NULL, parent, obj};
        if(parent){
            _SET_RED_(*itr);
            RBT_repair_tree_insert(&(tree->m_root),*itr);
        }
    }else{//Out of memory
        return 0;
    }
    return 1;
}

void RBT_delete_node(RBTree*tree,RBNode* itr){
    --tree->m_size;
    //itr points to the to be deleted node
    RBNode* temp = itr->right;
    if(temp){
        //swap with smallest element in right sub tree
        while(temp->left) temp = temp->left;
        itr->data = temp->data;
        itr = temp;
    }
    //currently itr has at most 1 child
    temp = (itr->left ? itr->left : itr->right);
    if(temp){
        //swap with child
        itr->data = temp->data;
        itr = temp;
    }
    //currently itr has no children
    temp = __CLEAR__(itr->parent);
    if(temp){//itr isn't root
        //detach itr from parent, itr may have a sibling
        if(temp->left == itr){
            temp->left = NULL;
        }else{
            temp->right = NULL;
        }
        //if itr is RED skip, otherwise repair
        if(_COLOR_(itr) == BLACK){
            RBT_repair_tree_extract(&(tree->m_root),temp);
        }
    }
    free(itr);
}

#define __RBT__ROTATE__(itr,left,right)\
    RBNode* pivot = *itr, *child = pivot->right;\
    *itr = child;\
    __SET_PARENT__(child,__CLEAR__(pivot->parent));\
    if((pivot->right = child->left)) __SET_PARENT__(pivot->right,pivot);\
    child->left = pivot;\
    __SET_PARENT__(pivot,child);


static inline void RBT_rotate_left(RBNode** itr){
    __RBT__ROTATE__(itr,left,right);
}
static inline void RBT_rotate_right(RBNode** itr){
    __RBT__ROTATE__(itr,right,left);
}


/**
* takes the pointer to the inserted node
* and repairs the tree
*/
void RBT_repair_tree_insert (RBNode** root, RBNode* itr){                       //repair(itr):                                                                              |
    RBNode* parent = __CLEAR__(itr->parent);                                    //CASE 1:          grand_parent_B                              grand_parent_R               |
    size_t parent_side;                                                         //                /              \                            /              \              |
    while(parent && _COLOR_(parent) == RED){                                    //        parent_R                uncle_R ----->      parent_B                uncle_B       |
        RBNode* grand_parent = __CLEAR__(parent->parent);                       //      x/        \x            x/       \x         x/        \x            x/       \x     |
        parent_side = (parent == grand_parent->right?RIGHT:LEFT);               //  itr_R                                       itr_R                                       |
        RBNode* uncle = (parent_side ? grand_parent->left: grand_parent->right);//                                                                                          |
        if(uncle == NULL || _COLOR_(uncle) == BLACK){                           //                                              + repair(grand_parent)                      |
            size_t child_side = (itr == parent->right? RIGHT:LEFT);             //==========================================================================================|
            if(child_side != parent_side){                                      //CASE 2:(child_side == parent_side)                                                        |
                if(child_side == LEFT){                                         //                 grand_parent_B          rotate          parent_B                         |
                    RBT_rotate_right(&(grand_parent->right));                   //                /              \X        right          /         \                       |
                }else{                                                          //        parent_R                uncle_B  ----->    itr_R           grand_parent_R         |
                    RBT_rotate_left (&(grand_parent->left ));                   //       /        \X                                X/  \X          X/             \X       |
                }                                                               //  itr_R                                                                           uncle_B |
            }                                                                   // X/  \X                                                                                   |
            RBNode** grand_ref = root;                                          //==========================================================================================|                                                                                         |
            if(grand_parent->parent){                                           //CASE 3: (child_side != parent_side)                                                       |
                if(grand_parent == grand_parent->parent->left){                 //                 grand_parent_B                                grand_parent_B             |
                    grand_ref = &(grand_parent->parent->left );                 //                /              \X                             /              \X           |
                }else{                                                          //        parent_R                uncle_B  ----->          itr_R                uncle_B     |
                    grand_ref = &(grand_parent->parent->right);                 //      X/        \X                                      /     \X                          |
                }                                                               //                 itr_R                          parent_R                                  |
            }                                                                   //                                                 X/  \X       +CASE 2(itr<-- parent)      |
            if(parent_side == LEFT){                                            //******************************************************************************************|
                RBT_rotate_right(grand_ref);
            }else{
                RBT_rotate_left(grand_ref);
            }
            _SET_BLACK_(*grand_ref);
            _SET_RED_(grand_parent);
            break;
        }else{
            _SET_BLACK_(uncle);
            _SET_BLACK_(parent);
            parent = grand_parent->parent;
            _SET_RED_(grand_parent);
            itr = grand_parent;
        }
    }
    _SET_BLACK_(*root);
}


typedef struct RBT_Prep_t{
    RBNode* repair_from;
    size_t repair_futher;
}RBT_Preparation;

RBT_Preparation RBT_repair_lowest_layer(RBNode** root,RBNode* itr);
void RBT_repair_tree_final(RBNode** root,RBNode* itr);

/**
* takes the pointer to the parent of the
* deleted node and repairs the tree
*/
void RBT_repair_tree_extract(RBNode** root,RBNode* itr){
    //Technically RBT_repair final includes the cases from RBT_repair_lowest_layer given that NULL pointers can be dereferenced.
    //Even if you check for existence of cousins, you only need to do it once, so this is an optimization, and gives cleaner code.
    RBT_Preparation prep = RBT_repair_lowest_layer(root,itr);
    if(prep.repair_from && prep.repair_futher){
        RBT_repair_tree_final(root,prep.repair_from);
    }
}


RBNode* RBT_construct_sub_tree_from(RBNode* array[8],int left,int right){
    if(left == right){
        return array[left];
    }else if(left<right){
        int middle = (left+right+1)/2;
        array[middle]->left = RBT_construct_sub_tree_from(array,left,middle-1);
        array[middle]->left->parent = array[middle];
        if((array[middle]->right = RBT_construct_sub_tree_from(array,middle+1,right))){
            array[middle]->right->parent = array[middle];
        }
        return array[middle];
    }else{
        return NULL;
    }
}



RBT_Preparation RBT_repair_lowest_layer(RBNode** root,RBNode* itr){ //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++|
    size_t color = _COLOR_(itr);                                    //     The deleted node is black, therefore the sibling tree is not empty and has up to              |
    RBNode* end = __CLEAR__(itr->parent);                           // 4( if color = RED),7 (if color = BLACK) elements.                                                 |
    RBNode* begin = itr->left;                                      //     Notation of nodes: (id)(c) where (id) is a number corresponding to in order ordering, and     |
    while(begin->left) begin = begin->left;                         // (c) is the color of the node{R xor B}.                                                            |
    int count = 0;                                                  // The important part is the number of nodes + 1(itr == 0), not how they are colored or arranged, so |
    RBNode* array[8];                                               // we can take the liberty of showing just the important cases. Deleted node is left child of 0.     |
    while(begin != end){                                            // I. color = RED:                                                                                   |
        array[count++] = begin;                                     //   1.   0R                1B   | 2.  0R                1R    | 3.  0R                 2R           |
        begin = RBT_advance_node_forward(begin);                    //   [2]    \              /     | [3]   \              /  \   | [4]   \               /  \          |
    }                                                               //            1B ----->  0R      |         2B ----->  0B    2B |        2B ----->    1B    3B        |
    RBNode** itr_ref = root;                                        //                               |       /                     |       /  \         /                |
    if(end){                                                        //                               |     1R                      |     1R    3R     0R                 |
        itr_ref = (itr == end->left ? &(end->left): &(end->right)); //===================================================================================================|
    }                                                               // II. color = BLACK                                                                                 |
    *itr_ref = RBT_construct_sub_tree_from(array,0,count-1);        //   1.   0B               1B    | 2.   0B              1B     | 3.   0B                 2B          |
    (*itr_ref)->parent = end;                                       //   [2]    \             /      | [3]    \            /  \    | [4]    \               /  \         |
    switch(count){                                                  //            1B -----> 0R       |         2B -----> 0B    2B  |         2R ----->    1B    3B       |
        case 2:                                                     //                               |        /                    |        /  \         /               |
            _SET_RED_(array[0]);                                    //    black path is shortened,   |      1R                     |      1B    3B     0R                |
            return (RBT_Preparation){*itr_ref,color==BLACK};        //    continue repairing         |                             |                                     |
            break;                                                  //    from 1B                    |                             |                                     |
        case 3:                                                     //  -------------------------------------------------------------------------------------------------|
            _SET_COLOR_(array[1],color);                            //   4.   0B                2B   | 5.   0B                  3B                                       |
            break;                                                  //   [5]    \              /  \  | [6]    \              /      \                                    |
        case 4:                                                     //           3R  ----->  1B    4B|         4R ----->   1B        5B                                  |
            _SET_RED_(array[0]);                                    //          /  \        /     /  |        /  \        /  \      /                                    |
            _SET_COLOR_(array[2],color);                            //         2B   4B    0R    3R   |      2B    5B    0R    2R  4R                                     |
            break;                                                  //        /                      |     /  \                                                          |
        case 5:                                                     //      1R                       |   1R    3R                                                        |
            _SET_RED_(array[0]);                                    //  -------------------------------------------------------------------------------------------------|
            _SET_RED_(array[3]);                                    //   6.    0B                        3B                                                              |
            break;                                                  //   [7]      \                    /     \                                                           |
        case 6:                                                     //             4R      ----->   1R        5R                                                         |
            _SET_RED_(array[0]);                                    //          /      \           /  \      /  \                                                        |
            _SET_RED_(array[2]);                                    //        2B        6B       0B    2B  4B    6B                                                      |
            _SET_RED_(array[4]);                                    //       /  \      /                                                                                 |
            break;                                                  //     1R    3R  5R                                                                                  |
        case 7:                                                     //  -------------------------------------------------------------------------------------------------|
            _SET_RED_(array[1]);                                    //   7.    0B                          4B                                                            |
            _SET_RED_(array[5]);                                    //   [8]      \                      /    \                                                          |
            break;                                                  //             4R      ----->     2R        6R                                                       |
        case 8:                                                     //          /      \             /  \      /  \                                                      |
            _SET_RED_(array[0]);                                    //        2B        6B         1B    3B  5B    7B                                                    |
            _SET_RED_(array[2]);                                    //       /  \      /  \       /                                                                      |
            _SET_RED_(array[6]);                                    //     1R    3R  5R   7R    0R                                                                       |
            break;                                                  //  -------------------------------------------------------------------------------------------------|
    }                                                               // As you can see, the first step is to re balance the tree(left bias), and then recolor the nodes.  |
    return (RBT_Preparation){*itr_ref,0};                           // RBT_construct_sub_tree_from balances the tree, and paints the nodes black.                        |
}                                                                   // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++|

void RBT_repair_tree_final(RBNode** root,RBNode* itr){
    // Existence of cousins is guaranteed, since itr has black path length of x(x>=1, due to RBT_repair_lowest_layer),and itr's sibling has x+1. btw itr is currently black.


}
