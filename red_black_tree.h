#ifndef RED_BLACK_TREE_H
#define RED_BLACK_TREE_H

#include <functional>

enum class Color{BLACK = false,RED = true};
enum class Traversal{PRE_ORDER,IN_ORDER,POST_ORDER};
enum class Direction{LEFT,RIGHT};

template<typename Key,typename Value,class Compare = std::less<Key>>
class red_black_tree{
    struct Node{
        Color color;
        Node *left,*right,*parent;
        std::pair<const Key,Value> m_data;

        Node(const std::pair<Key,Value>& data): color(Color::RED),left(nullptr),right(nullptr),parent(nullptr),m_data(data){}
        Node(std::pair<Key,Value>&& data):      color(Color::RED),left(nullptr),right(nullptr),parent(nullptr),m_data(data){}
        Node(const Node&) = delete;
        Node&operator=(const Node&) = delete;
    };
    Node* m_root;
    std::size_t m_size;

    #define __ROTATE_AROUND_NODE__(X,Y) do{\
    if(itr && itr->Y){\
        if(itr->parent){\
            if(itr->parent->left==itr)itr->parent->left = itr->Y;\
            else itr->parent->right= itr->Y;\
        }else m_root = itr->Y;\
        Node* temp = itr->Y;\
        temp->parent = itr->parent;\
        itr->Y = temp->X;\
        if(itr->Y)itr->Y->parent = itr;\
        temp->X = itr;\
        itr->parent = temp;\
    }\
    }while(false)
    void rotate_left(Node* itr){
        __ROTATE_AROUND_NODE__(left,right);
    }
    void rotate_right(Node*itr){
        __ROTATE_AROUND_NODE__(right,left);
    }
    #undef __ROTATE_AROUND_NODE__
    template<bool,Traversal>
    struct Advance_ptr{
        typename red_black_tree::Node* ptr_begin(typename red_black_tree::Node* itr){}
        typename red_black_tree::Node* ptr_end  (typename red_black_tree::Node* itr){}
        void operator()(typename red_black_tree::Node*& itr){}
    };
    template<bool D>struct Advance_ptr<D,Traversal::PRE_ORDER>{
        typename red_black_tree::Node* ptr_begin(typename red_black_tree::Node* itr){
            return itr;
        }
        typename red_black_tree::Node* ptr_end  (typename red_black_tree::Node* itr){
            return itr? itr->parent:itr;
        }
        void operator()(typename red_black_tree::Node*& itr){
            if(itr){
                if(itr->left) itr = itr->left;
                else if(itr->right)itr = itr->right;
                else{
                    bool not_found = true;
                    while(not_found && itr->parent){
                        if(itr->parent->right!=itr && itr->parent->right){
                            itr = itr->parent->right;
                            not_found = false;
                        }else itr = itr->parent;
                    }
                    if(not_found)itr = nullptr;
                }
            }
        }
    };
    template<bool D> struct Advance_ptr<D,Traversal::IN_ORDER>{
        typename red_black_tree::Node* ptr_begin(typename red_black_tree::Node* itr){
            while(itr->left) itr = itr->left;
            return itr;
        }
        typename red_black_tree::Node* ptr_end  (typename red_black_tree::Node* itr){
            return itr? itr->parent:itr;
        }
        void operator()(typename red_black_tree::Node*& itr){
            if(itr){
                if(itr->right){
                    itr = itr->right;
                    while(itr->left) itr = itr->left;
                }else{
                    bool not_found = true;
                    while(not_found && itr->parent){
                        if(itr->parent->right!=itr)not_found = false;
                        itr = itr->parent;
                    }
                    if(not_found) itr = nullptr;
                }
            }
        }
    };
    template<bool D> struct Advance_ptr<D,Traversal::POST_ORDER>{
        typename red_black_tree::Node* ptr_begin(typename red_black_tree::Node* itr){
            while(itr->left || itr->right) {
                if(itr->left)itr = itr->left;
                else if(itr->right)itr =itr->right;
            }
            return itr;
        }
        typename red_black_tree::Node* ptr_end  (typename red_black_tree::Node* itr){
            this->operator()(itr);
            return itr;
        }
        void operator()(typename red_black_tree::Node*& itr){
            if(itr){
                if(itr->parent){
                    if(itr->parent->right==itr || itr->parent->right==nullptr){
                        itr = itr->parent;
                    }else if(itr->parent->right){
                        itr = itr->parent->right;
                        while(itr->left || itr->right) {
                            if(itr->left)itr = itr->left;
                            else if(itr->right)itr =itr->right;
                        }
                    }
                }else itr = nullptr;
            }
        }
    };
    template<Traversal _T,class Storable>
    class m_iterator{
        typename red_black_tree::Node* m_ptr;
    public:
        typedef m_iterator self_type;
        typedef Storable value_type;
        typedef Storable& reference;
        typedef Storable* pointer;
        typedef std::forward_iterator_tag iterator_category;
        typedef int difference_type;

        m_iterator(typename red_black_tree::Node* ptr):m_ptr(ptr){}
        m_iterator(const m_iterator& rhs):m_ptr(rhs.m_ptr){}
        self_type& operator=(const self_type& rhs){m_ptr = rhs.m_ptr;}
        reference operator*(){return m_ptr->m_data;}
        pointer operator->(){return &(m_ptr->m_data);}
        bool operator==(const m_iterator& rhs){return m_ptr == rhs.m_ptr;}
        bool operator!=(const m_iterator& rhs){return m_ptr != rhs.m_ptr;}

        self_type& operator++(){Advance_ptr<true,_T>()(m_ptr);}
        self_type operator++(int){
            m_iterator temp(*this);
            ++(*this);
            return temp;
        }
        friend class red_black_tree;
    };

public:
    bool insert(const Key& key,const Value& value){
        Node **itr = &m_root,*par = nullptr;
        Compare cmp;
        while(*itr){
            par = *itr;
            if(cmp(itr->m_data->first,key))itr = &((*itr)->right);
            else if(cmp(key,itr->m_data->first)) itr = &((*itr)->left);
            else return false;
        }
        (*itr) = new Node(key,value);
        (*itr)->parent = par;
        ++m_size;
        repair_tree_insert(*itr);
        return true;
    }
    template<Traversal _T>
    using iterator = m_iterator<_T,std::pair<const Key,Value>>;
    template<Traversal _T>
    using const_iterator = m_iterator<_T,const std::pair<const Key,Value>>;
    red_black_tree():m_root(nullptr),m_size(0) {}
    ~red_black_tree() {}

private:
    typename red_black_tree::Node* find_node_by_key(const Key& key)const{
        typename red_black_tree::Node* itr = m_root;
        Compare cmp;
        while(itr){
            if(cmp(key,itr->m_data.first))itr = itr->left;
            else if(cmp(itr->m_data.first,key)) itr = itr->right;
            else return itr;
        }
        return itr;
    }
    void repair_tree_insert(typename red_black_tree::Node* itr){
        /*if(itr->parent==nullptr || itr->parent->color == Color::BLACK){
            if(itr->parent == nullptr) itr->color == Color::BLACK;
        }else{
            Node* grand_parent = itr->parent->parent;
            Node* uncle = (itr->parent->parent->left == itr->parent?itr->parent->parent->right: itr->parent->parent->left);
            if(uncle == nullptr || uncle->color == Color::BLACK){
                if(grand_parent->left && itr == grand_parent->left->right){
                    rotate_left(grand_parent);
                }else if(grand_parent->right && itr == grand_parent->right->left){
                    rotate_right(grand_parent);
                }
                if(itr->parent->right == itr){
                    rotate_left(grand_parent);
                }else{
                    rotate_right(grand_parent);
                }
                grand_parent->color = Color::RED;
                grand_parent->parent->color = Color::BLACK;
                m_root->color = Color::BLACK;
            }else{
                uncle->color = itr->parent->color = Color::BLACK;
                grand_parent->color = Color::RED;
                repair_tree_insert(grand_parent);
            }
        }*/
        while(itr->parent && itr->parent->color == Color::RED){
            Node* grand_parent = itr->parent->parent;
            Node* uncle = (itr->parent->parent->left == itr->parent?itr->parent->parent->right: itr->parent->parent->left);
            if(uncle == nullptr || uncle->color == Color::BLACK){
                if(grand_parent->left && itr == grand_parent->left->right){
                    rotate_left(itr->parent);
                }else if(grand_parent->right && itr == grand_parent->right->left){
                    rotate_right(itr->parent);
                }
                if(itr->parent->right == itr){
                    rotate_left(grand_parent);
                }else{
                    rotate_right(grand_parent);
                }
                grand_parent->color = Color::RED;
                grand_parent->parent->color = Color::BLACK;
                itr = m_root;
            }else{
                uncle->color = itr->parent->color = Color::BLACK;
                grand_parent->color = Color::RED;
                itr = grand_parent;
            }
        }
        if(itr->parent == nullptr){
            itr->color == Color::BLACK;
        }
    }
    void repair_tree_erase(typename red_black_tree::Node* itr){

    }
    bool erase_by_node_ptr(typename red_black_tree::Node* itr){
        if(itr == nullptr) return false;

        return true;
    }

};

#endif // RED_BLACK_TREE_H
