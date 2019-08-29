#ifndef RBTREE_H
#define RBTREE_H



typedef void Key;
typedef void Object;
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
extern const size_t RBT_SIZE_OFFSET;
#define RBT_SIZE(tree) *(const size_t*)((size_t)(tree)+RBT_SIZE_OFFSET)

/**
* C++ like iterator.
* Modifying the key part of data pointed by the iterator corrupts the structure.
* For the ease of assignment nothing is const, but modify nothing.
* the iterator is for accessing the data only, and nothing else.
* iterator reached the end iff data == NULL
*/
typedef struct RBT_Iterator{
    Object*  data;
    RBNode* current;
}RBT_Iterator;
/**
* Returns a pointer to an empty Red Black Tree, where
* "compare" is used to compare keys, and
* "get_key" is used to get the pointer to the key from the stored objects.
* the relation between compare(a,b) and 0 is the same as between a and b.
*/
RBTree* RBT_get(const Key* (*get_key)(const Object*), int (*compare)(const Key*,const Key*));

/**
 Frees tree, while in the process applies "deallocate" to the stored objects
*/
void RBT_destroy(RBTree*, void (*deallocate)(Object*));

/**
* positive does ++itr.
* negative does --itr.
* zero does nothing
* return 0 iff iterator couldn't be moved
*/
int RBT_advance(RBT_Iterator*itr, int direction);

/**
* advances itr abs(direction) times in the corresponding direction
* returns 0 iff iterator reached and end, or couldn't be moved
*/
int RBT_advance_n(RBT_Iterator*itr, int direction);
/**
* Think of it as tree[key], but returns the whole object.
* Returns NULL if there is no corresponding object.
* Modifying the key corrupts the structure.
*/
Object* RBT_at(RBTree*tree, const Key*key);

/**
* Returns an iterator to the corresponding object in the tree
* In case the object is not inside the tree, the the iterator
* can be used for insertion.
*/
RBT_Iterator RBT_search(RBTree*,const Key*);

/**
* Returns given pointer if successful, otherwise gives the pointer
* to the object that obstructed the insertion.
* Returns NULL if memory allocation for Node failed.
* Invalidates any existing RBT_Iterators
* Not thread safe.
*/
Object* RBT_insert(RBTree*,Object*);

/**
* Given an  insertion iterator from RBT_search inserts the object, and returns the pointer to it, iff successful.
* Given anything else does nothing and return NULL.
* Successful insertion invalidates any RBT_Iterator.
* Not thread safe.
*/
Object* RBT_insert_itr(RBTree*,RBT_Iterator,Object*);

/**
* Deletes the corresponding Node and return the pointer to the object.
* Returns NULL if there is no corresponding object.
* Successful extraction invalidates any RBT_Iterator.
* Not thread safe.
*/
Object* RBT_exract(RBTree*,const Key*);

/**
* Deletes the corresponding Node and return the pointer to the object.
* Returns NULL if the iterator is invalid(data is NULL, or current is NULL).
* Successful extraction invalidates any RBT_Iterator.
* Not thread safe.
*/
Object* RBT_exract_itr(RBTree*,RBT_Iterator);

/**
* Gives iterator to smallest element
*/
RBT_Iterator RBT_begin(RBTree*);

/**
* Gives iterator to largest element
*/
RBT_Iterator RBT_rbegin(RBTree*);

/**
* Iterator ends when data is NULL;
*/
RBT_Iterator RBT_end(RBTree*);

/**
* Iterator ends when data is NULL;
*/
RBT_Iterator RBT_rend(RBTree*);

/**
* Returns number of elements in the tree.
* For higher performance consider the macro RBT_SIZE
*/
size_t RBT_size(RBTree*);

/**
* Restructures the tree into a perfectly balanced tree.
*/
void RBT_balance(RBTree*);

/**
* Makes a deep copy* of tree.
* where copy does a deep copy of the stored objects. copy needs to return NULL in case the copy failed
* In case of memory allocation error, destroy is used to
* remove the already copied objects.
* The copy is not perfect since it will be perfectly balanced.
*/
RBTree* RBT_copy(RBTree* tree,Object* (*copy)(const Object*),void (*destroy)(Object*));

#endif // RBTREE_H
