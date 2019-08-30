#include <stdlib.h>
#include <stdio.h>
#include "rbtree.h"

#define _CLEAR_(itr) ((RBNode*)((size_t)(itr) & BLACK_MASK))
#define _COLOR_(itr) ((size_t)((itr)->parent) & 1)
#define _SET_RED_(itr) (itr)->parent = (RBNode*)((size_t)((itr)->parent) | RED_MASK)
#define _SET_BLACK_(itr) (itr)->parent = (RBNode*)((size_t)((itr)->parent) & BLACK_MASK)
#define _SET_PARENT_(itr,ptr) (itr)->parent = (RBNode*)(_COLOR_(itr)|(size_t)(ptr))
#define _SET_COLOR_(itr,color) (itr)->parent = (RBNode*)((size_t)((itr)->parent) | (color))
#define _CHILD_REF_OFFSET_(itr,offset) (RBNode**)((size_t)(itr)+(offset))
#define _CHILD_REF_PTR_(parent,itr) ((parent)->left == (itr) ? &((parent)->left): &((parent)->right))
#define _REPAINT_(itr,color) (itr)->parent = (RBNode*)( ((size_t)((itr)->parent)&BLACK_MASK)|(color))



void print_data(FILE*,void*);

typedef struct RBNode_t{
    struct RBNode_t *left,*right,*parent;
    void* data;
} RBNode;

typedef struct RBTree_t{
    RBNode *m_root;
    size_t m_size;
    const Key* (*get_key)(const Object*);
    int (*compare)(const Key*,const Key*);
} RBTree;

static size_t BLACK_MASK = ~1, RED_MASK = 1;

enum RBT_Direction{LEFT = 0,RIGHT = 1};
enum RBT_Color {BLACK = 0,RED = 1};

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
    *This = (RBTree){NULL,0,get_key,compare};
    return This;
}

void RBT_destroy_node(RBNode* node,void (*deallocate)(Object*)){
    if(node){
        RBT_destroy_node(node->left,deallocate);
        RBT_destroy_node(node->right,deallocate);
        if(deallocate) deallocate(node->data);
        free(node);
    }

}

void RBT_destroy(RBTree* tree, void (*deallocate)(Object*)){
    RBT_destroy_node(tree->m_root,deallocate);
    free(tree);
}

size_t RBT_size(RBTree* tree){
    return tree->m_size;
}


#define __RBT__ADVANCE_NODE__(itr,left,right)\
    if(itr->right){\
        itr = itr->right;\
        while(itr->left) itr = itr->left;\
    }else{\
        RBNode* temp_parent = _CLEAR_(itr->parent);\
        while(temp_parent && temp_parent->right == itr){\
            itr = temp_parent;\
            temp_parent = _CLEAR_(itr->parent);\
        }\
        itr = temp_parent;\
    }\

/**
* Returns NULL if reached the end.
*/
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
    size_t left,right;
    if(direction > 0){
        left = RBT_LEFT_OFFSET; right = RBT_RIGHT_OFFSET;
    }else{
        left = RBT_RIGHT_OFFSET; right = RBT_LEFT_OFFSET;
    }

    RBNode *temp_parent = itr;
    if( (itr = *_CHILD_REF_OFFSET_(itr,right)) ){
        while( (temp_parent = itr, itr = *_CHILD_REF_OFFSET_(itr,left)) );
    }else{
        itr = temp_parent;
        temp_parent = _CLEAR_(itr->parent);
        while(temp_parent && itr == *_CHILD_REF_OFFSET_(temp_parent,right)){
            itr = temp_parent;
            temp_parent = _CLEAR_(itr->parent);
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

int RBT_advance_n(RBT_Iterator*itr, int direction){
    if(direction == 0 || itr->data == NULL || itr->current == NULL){
        return 0;
    }else{
        int sign = (direction>0 ? 1: (direction = -direction), -1);
        RBNode* current = itr->current;
        while(direction-- && current){
            current = RBT_advance_node(current,sign);
        }
        *itr = (RBT_Iterator){current? current->data:NULL,current};
        return 1;
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
    temp = _CLEAR_(itr->parent);
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
    }else{
        tree->m_root = NULL;
    }
    free(itr);
}

#define __RBT__ROTATE__(itr,left,right)\
    RBNode* pivot = *itr, *child = pivot->right;\
    *itr = child;\
    _SET_PARENT_(child,_CLEAR_(pivot->parent));\
    if((pivot->right = child->left)) _SET_PARENT_(pivot->right,pivot);\
    child->left = pivot;\
    _SET_PARENT_(pivot,child);


static inline void RBT_rotate_left(RBNode** itr){
    __RBT__ROTATE__(itr,left,right);
}
static inline void RBT_rotate_right(RBNode** itr){
    __RBT__ROTATE__(itr,right,left);
}

/*
itr == I ; parent == P; grand_parent = G; uncle = U;
CASE 1:
        GB               GR
       /  \             /  \
     PR    UR ----->  PB    UB       + repair(G)
   X/  \X            X/\X  X/\X
  IR                IR

CASE 2:
         GB     rotate        PB
        /  \X   right G     /    \
      PR    UB  ------>   IR      GR
     /  \X              X/  \X  X/  \X
   IR                                UB
 X/  \X

CASE 3:
       GB     rotate        GB     rotate       IB
      /  \X   left P       /  \X   right G    /    \
    PR    UB  ----->     IR    UB  ------>  PR      GR
  X/  \X                /  \X             X/  \X  X/  \X
       IR             PR                               UB
      X/\X          X/  \X
*/

/**
* takes the pointer to the inserted node
* and repairs the tree
*/
void RBT_repair_tree_insert (RBNode** root, RBNode* itr){
    RBNode* parent = _CLEAR_(itr->parent);
    size_t parent_side;
    while(parent && _COLOR_(parent) == RED){
        RBNode* grand_parent = _CLEAR_(parent->parent);
        parent_side = (parent == grand_parent->right?RIGHT:LEFT);
        RBNode* uncle = (parent_side ? grand_parent->left: grand_parent->right);
        if(uncle == NULL || _COLOR_(uncle) == BLACK){
            size_t child_side = (itr == parent->right? RIGHT:LEFT);
            RBNode** grand_ref = root;
            if(grand_parent->parent){
                grand_ref = _CHILD_REF_PTR_(grand_parent->parent,grand_parent);
            }
            if(parent_side == LEFT){
                if(child_side == RIGHT){
                    RBT_rotate_left (&(grand_parent->left ));
                }
                RBT_rotate_right(grand_ref);
            } else {
                if(child_side == LEFT){
                    RBT_rotate_right(&(grand_parent->right));
                }
                RBT_rotate_left(grand_ref);
            }
            _SET_BLACK_(*grand_ref);
            _SET_RED_(grand_parent);
            break;
        } else {
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
* Takes the pointer to the parent of the
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

/**
* Constructs a left biased balanced tree, from the given array of nodes
* and return a pointer to the root of said tree.
*/
RBNode* RBT_construct_sub_tree_from(RBNode* array[8],int left,int right){
    if(left == right){
        array[left]->left = array[left]->right = NULL;
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
/*
    The deleted node is black, therefore the sibling tree is not empty and has up to 3( if color = RED),7 (if color = BLACK) nodes.
    Notation of nodes: (id)(c) where (id) is a number corresponding to in order ordering, and (c) is the color of the node{R xor B}.
    The important part is the number of nodes + 1([n]), not how they are colored or arranged, so
we can take the liberty of showing just the important cases. Deleted node is left child of 0.
-----------------------------------------------------------------------------------------------
I. color = RED:
    1.   0R                1B   | 2.  0R                1R    | 3.  0R                 2R
    [2]    \              /     | [3]   \              /  \   | [4]   \               /  \
            1B ----->  0R       |        2B ----->  0B    2B  |        2B ----->    1B    3B
                                |       /                     |       /  \         /
                                |     1R                      |     1R    3R     0R
-----------------------------------------------------------------------------------------------
II. color = BLACK
    1.   0B               1B    | 2.   0B              1B     | 3.   0B                 2B
    [2]    \             /      | [3]    \            /  \    | [4]    \               /  \
             1B -----> 0R       |         2B -----> 0B    2B  |         2R ----->    1B    3B
                                |        /                    |        /  \         /
    black path is shortened,    |      1R                     |      1B    3B     0R
    continue repairing from 1B  |                             |
-----------------------------------------------------------------------------------------------
    4.   0B                2B                   | 5.   0B                  3B
    [5]    \              /  \                  | [6]    \              /      \
            3R  ----->  1B    4B                |         4R ----->   1B        5B
           /  \        /     /                  |        /  \        /  \      /
         2B    4B    0R    3R                   |      2B    5B    0R    2R  4R
        /                                       |     /  \
      1R                                        |   1R    3R
-----------------------------------------------------------------------------------------------
6.    0B                         3B             |  7.    0B                          4B
[7]      \                    /      \          |  [8]      \                      /    \
           4R      ----->   1R        5R        |            4R      ----->     2R        6R
        /      \           /  \      /  \       |         /      \             /  \      /  \
     2B         6B       0B    2B  4B    6B     |       2B        6B         1B    3B  5B    7B
    /  \       /                                |      /  \      /  \       /
  1R    3R   5R                                 |    1R    3R  5R   7R    0R
-----------------------------------------------------------------------------------------------
As you can see, the first step is to re balance the tree(left bias), and then recolor the nodes.
RBT_construct_sub_tree_from balances the tree, and paints the nodes black.
*/

RBT_Preparation RBT_repair_lowest_layer(RBNode** root,RBNode* itr){
    //Get reference to itr.
    size_t color = _COLOR_(itr);
    RBNode** itr_ref = root;
    RBNode* parent = _CLEAR_(itr->parent);
    if( parent ) itr_ref = _CHILD_REF_PTR_(parent,itr);
    //Put the nodes in order in array, starting from itr.
    RBNode* array[8];
    int count = 0;
    itr->parent = NULL;
    while( itr->left ) itr = itr->left;
    while( itr ){
        array[count++] = itr;
        itr = RBT_advance_node_forward(itr);
    }
    //Emplace reconstructed subtree.
    *itr_ref = RBT_construct_sub_tree_from(array,0,count-1);
    (*itr_ref)->parent = parent;
    //Color the corresponding nodes red.
    switch( count ){
        case 2:
            _SET_RED_(array[0]);
            return (RBT_Preparation){*itr_ref,color==BLACK};
            break;
        case 3:
            _SET_COLOR_(array[1],color);
            break;
        case 4:
            _SET_RED_(array[0]);
            _SET_COLOR_(array[2],color);
            break;
        case 5:
            _SET_RED_(array[0]);
            _SET_RED_(array[3]);
            break;
        case 6:
            _SET_RED_(array[0]);
            _SET_RED_(array[2]);
            _SET_RED_(array[4]);
            break;
        case 7:
            _SET_RED_(array[1]);
            _SET_RED_(array[5]);
            break;
        case 8:
            _SET_RED_(array[0]);
            _SET_RED_(array[2]);
            _SET_RED_(array[6]);
            break;
    }
    return (RBT_Preparation){*itr_ref,0};
}

/*
Existence of cousins(1 removed) and siblings is guaranteed, since itr has black path length of X
(X>=1, due to RBT_repair_lowest_layer),and itr's sibling has X+1. itr is black.

itr == I ; itr->parent == P ; sibling == S ; close cousin == C; far cousin == F
CASE 1:
        PB                 PB
      X/  \X+1           X/  \X
    IB     SB    -----> IB    SR      ,black path shortened, repair from P.
         X/  \X             X/  \X
        CB    FB           CB    FB

CASE 2:
        PB          rotate            SB
      X/  \         left P          /    \
    IB     SR       ------>      PR       FB      ,repair from I
          /  \                 X/  \X+1  X/\X
        CB    FB              IB    CB
       X/\X  X/\X

CASE 3:
       PB          rotate            SB
     X/  \         left P          /    \
    IB     SB      ------>      PB       FB
         X/  \                X/  \X    X/\X
        CB    FR             IB    CB
             X/\X

CASE 4:
       PB          rotate          PB         rotate         CB
     X/  \         right S       X/  \        left P       /    \
    IB     SB      ------>      IB    CR      ------>    PB      SB
          /  \X                     X/  \              X/  \X  X/  \X
        CR    FR                         SB           IB            FR
       X/\X                            X/  \X
                                            FR
CASE 5:
       PB          rotate          PB         rotate         CB
     X/  \         right S       X/  \        left P       /    \
    IB     SB      ------>      IB    CR      ------>    PB      SB
          /  \X                     X/  \              X/  \X  X/  \X
        CR    FB                         SB           IB            FB
       X/\X                            X/  \X
                                            FB
CASE 6:
        PR                 PB
      X/  \X+1           X/  \
    IB     SB    -----> IB    SR
         X/  \X             X/  \X
        CB    FB           CB    FB
CASE 7:
       PR          rotate            SR
     X/  \         left P          /    \
    IB     SB      ------>      PB       FB
         X/  \                X/  \X    X/\X
        CB    FR             IB    CB
             X/\X
CASE 8:
       PR          rotate          PB         rotate         CR
     X/  \         right S       X/  \        left P       /    \
    IB     SB      ------>      IB    CR      ------>    PB      SB
          /  \X                     X/  \              X/  \X  X/  \X
        CR    FR                         SB           IB            FR
       X/\X                            X/  \X
                                            FR
CASE 9:
       PR          rotate          PR         rotate         CR
     X/  \         right S       X/  \        left P       /    \
    IB     SB      ------>      IB    CR      ------>    PB      SB
          /  \X                     X/  \              X/  \X  X/  \X
        CR    FB                         SB           IB            FB
       X/\X                            X/  \X
                                            FB
One can observe that the cases with at least 1 red cousin(3,4,5,7,8,9) can be solved with the same algorithm.
    *Let COLOR be the color of P.
    *If C is RED then rotate right around S.
    *Rotate left around P.
    *Color new root COLOR.
    *Color new root's children BLACK
The remaining cases are solved individually.
In particular in CASE 2 after the recoloring comes the above algorithm.
*/

void RBT_repair_tree_final(RBNode** root,RBNode* itr){
    size_t cousin_colors[2];
    RBNode* parent = itr->parent;
    RBNode** parent_ref;
    RBNode* grand_parent;
    while(parent){
        size_t child_side = (itr == parent->left? LEFT:RIGHT);
        RBNode* sibling = child_side == LEFT? parent->right:parent->left;
        cousin_colors[0] = _COLOR_(sibling->left);
        cousin_colors[1] = _COLOR_(sibling->right);
        if(cousin_colors[0] == BLACK && cousin_colors[1] == BLACK){
            if(_COLOR_(parent) == RED){
                _SET_BLACK_(parent);
                _SET_RED_(sibling);
                return;
            }else if(_COLOR_(sibling) == RED){
                parent_ref = root; grand_parent = _CLEAR_(parent->parent);
                if(grand_parent){
                    parent_ref = _CHILD_REF_PTR_(grand_parent,parent);
                }
                _SET_BLACK_(sibling);
                _SET_RED_(parent);
                if(child_side == LEFT){
                    RBT_rotate_left(parent_ref);
                }else{
                    RBT_rotate_right(parent_ref);
                }
                continue;
            }else{
                _SET_RED_(sibling);
                itr = parent;
                parent = itr->parent;
                continue;
            }
        }
        parent_ref = root; grand_parent = _CLEAR_(parent->parent);
        if(grand_parent){
            parent_ref = _CHILD_REF_PTR_(grand_parent,parent);
        }
        if(child_side == LEFT){
            if(cousin_colors[0] == RED){
                RBT_rotate_right(&(parent->right));
            }
            RBT_rotate_left(parent_ref);
        }else{
            if(cousin_colors[1] == RED){
                RBT_rotate_left(&(parent->left));
            }
            RBT_rotate_right(parent_ref);
        }
        _REPAINT_(*parent_ref,_COLOR_(parent));
        parent = *parent_ref;
        _SET_BLACK_(parent->left);
        _SET_BLACK_(parent->right);
        return;
    }
}

typedef struct RBNode_pair_t{
    RBNode* first,*second;
} RBNode_pair;

RBNode_pair RBT_construct_sub_tree_list(RBNode* node,size_t size);
void RBT_balanced_coloring(RBNode* node, size_t level,size_t remainder);

void RBT_balance(RBTree*tree){
    //Tree is flattened to an ordered list, left pointer is used for next node(in fact only that can be used);
    RBNode* list_root = tree->m_root;
    while(list_root->left) list_root = list_root->left;
    RBNode* itr = list_root,* next = RBT_advance_node_forward(itr);
    while(next){
        _SET_BLACK_(itr);
        itr->left = next;
        itr = itr->left;
        next = RBT_advance_node_forward(next);
    }
    //Create balanced tree from the list
    RBNode_pair tree_empty = RBT_construct_sub_tree_list(list_root,tree->m_size);
    tree->m_root = tree_empty.first;
    tree->m_root->parent = NULL;
    //Color the nodes
    size_t level = 0, size = tree->m_size;
    while(size){
        ++level; size>>=1;
    }
    RBT_balanced_coloring(tree->m_root,level,level%2);
    _SET_BLACK_(tree->m_root);
}

/*
    constructs a tree with size amount of nodes from node inclusive, and returns the tree and the rest of the list
*/
RBNode_pair RBT_construct_sub_tree_list(RBNode* node,size_t size){
    if(size == 1){
        RBNode* tail = node->left;
        node->left = node->right = NULL;
        return (RBNode_pair){node,tail};
    }else if (size == 0){
        return (RBNode_pair){NULL,node?node->left:NULL};
    }else{
        size_t left_size = size/2;
        size_t right_size = size - 1 - left_size;
        RBNode_pair left_tree_center = RBT_construct_sub_tree_list(node,left_size );
        RBNode* center = left_tree_center.second;
        RBNode_pair right_tree_rest  = RBT_construct_sub_tree_list(center->left,right_size);
        if((center->right = right_tree_rest.first)) center->right->parent = center;
        (center->left = left_tree_center.first)->parent = center;
        return (RBNode_pair){center,right_tree_rest.second};
    }
}

/*
    if level%2 == remainder, then color red
    tree is balanced therefore it has at most [sizeof(void*) * 8] levels, which means recursion is acceptable.
*/
void RBT_balanced_coloring(RBNode* node, size_t level,size_t remainder){
    _SET_COLOR_(node, (level&1) == remainder ? RED: BLACK );
    --level;
    if(node->left) RBT_balanced_coloring(node->left ,level,remainder);
    if(node->right)RBT_balanced_coloring(node->right,level,remainder);
}

RBTree* RBT_copy(RBTree* tree,Object* (*copy)(const Object*),void (*destroy)(Object*)){
    int failure = 0;
    RBTree* new_tree = (RBTree*) malloc(sizeof(RBTree));
    if(new_tree == NULL) return NULL;
    //copy a flattened tree
    RBNode* root = tree->m_root;
    while(root->left) root = root->left;

    RBNode* list = (RBNode*) malloc(sizeof(RBNode));
    if(list == NULL){
        free(new_tree);return NULL;
    }
    list->data = copy(root->data);
    if(list->data == NULL ){
        free(list);free(new_tree);return NULL;
    }
    root = RBT_advance_node_forward(root);
    RBNode* itr = list;
    while(root){
        itr->left = (RBNode*) malloc(sizeof(RBNode));
        if(itr->left == NULL){
            failure = 1; break;
        }
        itr->left->data = copy(root->data);
        if(itr->left->data == NULL){
            free(itr->left);
            failure = 1;break;
        }
        itr = itr->left;
        root = RBT_advance_node_forward(root);
    }
    itr->left = NULL;
    if(failure){//delete new tree;
        while(list){
            destroy(list->data);
            root = list;
            list = list->left;
            free(root);
        }
        free(new_tree);
        return NULL;
    }
    //list is a flattened copy of tree
    RBNode_pair b_tree = RBT_construct_sub_tree_list(list,tree->m_size);

    size_t level = 0, size = tree->m_size;
    while(size){
        ++level; size>>=1;
    }
    RBT_balanced_coloring(b_tree.first,level,level%2);
    *new_tree = *tree;
    new_tree->m_root = b_tree.first;
    new_tree->m_root->parent = NULL;
    _SET_BLACK_(new_tree->m_root);
    return new_tree;
}
