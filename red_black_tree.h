#ifndef RED_BLACK_TREE_H
#define RED_BLACK_TREE_H

#include <functional>

enum class Color{BLACK = false,RED = true};
enum class Traversal{PRE_ORDER,IN_ORDER,POST_ORDER};
enum class Direction{LEFT,RIGHT};

template<typename Key,typename Value,class Compare = std::less<Key>>
class red_black_tree{

    red_black_tree& operator=(const red_black_tree&);
    red_black_tree& operator=(red_black_tree&&);
    struct Node{
        Color color;
        Node *left,*right,*parent;
        std::pair<const Key,Value> m_data;

        Node(const std::pair<Key,Value>& data): color(Color::RED),left(nullptr),right(nullptr),parent(nullptr),m_data(data){}
        Node(std::pair<Key,Value>&& data):      color(Color::RED),left(nullptr),right(nullptr),parent(nullptr),m_data(data){}
        Node(const Node&) = delete;
        Node&operator=(const Node&) = delete;

        ~Node(){
            std::cout<<"deleted: "<<m_data.first<<' '<<(color == Color::BLACK? "black":"red")<<std::endl;
        }
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
            if(itr){
                bool not_found = true;
                while(not_found && itr->parent){
                    if(itr->parent->right!=itr && itr->parent->right){
                        itr = itr->parent->right;
                        not_found = false;
                    }else itr = itr->parent;
                }
                if(not_found)itr = nullptr;
            }
            return itr;
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
            if(itr){
                bool not_found = true;
                while(not_found && itr->parent){
                    if(itr->parent->right!=itr)not_found = false;
                    itr = itr->parent;
                }
                if(not_found) itr = nullptr;
            }
            return itr;
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
    template<Traversal _T>
    using iterator = m_iterator<_T,std::pair<const Key,Value>>;
    template<Traversal _T>
    using const_iterator = m_iterator<_T,const std::pair<const Key,Value>>;

    bool insert(const Key& key,const Value& value){
        Node **itr = &m_root,*par = nullptr;
        Compare cmp;
        while(*itr){
            par = *itr;
            if(cmp((*itr)->m_data.first,key))itr = &((*itr)->right);
            else if(cmp(key,(*itr)->m_data.first)) itr = &((*itr)->left);
            else return false;
        }
        (*itr) = new Node(std::make_pair(key,value));
        (*itr)->parent = par;
        ++m_size;
        repair_tree_insert(*itr);
        std::cout<<"succsesfuly inserted: "<<key<<". At"<<*itr<<". parent: "<<par<<". current size: "<<m_size<<std::endl;
        return true;
    }
    bool erase(const Key& key){
        return erase_by_node_ptr(find_node_by_key(key));
    }
    template<Traversal _tr>
    bool erase(iterator<_tr> itr){
        return erase_by_node_ptr(itr->m_ptr);
    }
    red_black_tree():m_root(nullptr),m_size(0) {}

    ~red_black_tree() {
        this->delete_tree();
    }

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
        m_root->color = Color::BLACK;
    }
    typename red_black_tree::Node* construct_sub_tree_from(typename red_black_tree::Node* arr[8],int left,int right){
        if(left == right){
            arr[left]->left = arr[left]->right = nullptr;
            return arr[left];
        }
        int middle = (left+right)/2;
        Node* center = arr[middle];
        if(left != middle){
            center->left = construct_sub_tree_from(arr,left,middle-1);
            center->left->parent = center;
        }else{
            center->left = nullptr;
        }
        center->right = construct_sub_tree_from(arr,middle+1,right);
        center->right->parent = center;

        return center;

    }
    std::pair<typename red_black_tree::Node*,bool> pre_balance(typename red_black_tree::Node* itr){
        Node* node_ptr_arr[8];
        Advance_ptr<true,Traversal::IN_ORDER> advance;
        Node* ptr_end = advance.ptr_end(itr);
        Node* ptr_itr=advance.ptr_begin(itr);
        unsigned node_count = 0;
        while(ptr_itr != ptr_end){
            node_ptr_arr[node_count++] = ptr_itr;
            advance(ptr_itr);
        }
        bool should_repair_further = false;
        switch(node_count){
        case 2:
            node_ptr_arr[0]->color = Color::BLACK;
            node_ptr_arr[1]->color = Color::RED;
            should_repair_further = true;
            break;
        case 3:
            node_ptr_arr[1]->color = itr->color;
            node_ptr_arr[0]->color = node_ptr_arr[2]->color = Color::BLACK;
            break;
        case 4:
            if( itr->right->color == Color::RED ){
                node_ptr_arr[0]->color = node_ptr_arr[1]->color = node_ptr_arr[3]->color = Color::BLACK;
                node_ptr_arr[2]->color = Color::RED;
            }else{
                node_ptr_arr[0]->color = node_ptr_arr[2]->color = Color::BLACK;
                node_ptr_arr[1]->color = node_ptr_arr[3]->color = Color::RED;
            }
            break;
        case 5:
            node_ptr_arr[0]->color = node_ptr_arr[2]->color = node_ptr_arr[3]->color = Color::BLACK;
            node_ptr_arr[1]->color = node_ptr_arr[4]->color = Color::RED;
            break;
        case 6:
            node_ptr_arr[0]->color = node_ptr_arr[2]->color = node_ptr_arr[3]->color = node_ptr_arr[5]->color = Color::BLACK;
            node_ptr_arr[1]->color = node_ptr_arr[4]->color = Color::RED;
            break;
        case 7:
            node_ptr_arr[0]->color = node_ptr_arr[2]->color = node_ptr_arr[3]->color = node_ptr_arr[5]->color = Color::BLACK;
            node_ptr_arr[1]->color = node_ptr_arr[4]->color = node_ptr_arr[6]->color = Color::RED;
            break;
        case 8:
            node_ptr_arr[0]->color = node_ptr_arr[2]->color = node_ptr_arr[3]->color =
            node_ptr_arr[4]->color = node_ptr_arr[6]->color = Color::BLACK;
            node_ptr_arr[1]->color = node_ptr_arr[5]->color = node_ptr_arr[7]->color = Color::RED;
            break;
            default: std::cout<<"\nHIBA!\n";
        }
        return std::make_pair(construct_sub_tree_from(node_ptr_arr,0,node_count-1),should_repair_further);
    }
    void repair_recurse(typename red_black_tree::Node* itr){
        if(itr->parent!=nullptr){
            Node* sibling = (itr->parent->left==itr? itr->parent->right:itr->parent->left);
            if(sibling->color == Color::RED){
                sibling->color = Color::BLACK;
                if(itr = itr->parent->left){
                    rotate_left(itr);
                }else{
                    rotate_right(itr);
                }
                itr->parent->color = Color::RED;
                repair_recurse(itr);
            }else{
                Node* parent = itr->parent;
                if(parent->color == Color::RED){
                    parent->color = Color::BLACK;
                    if(sibling->left->color == Color::BLACK && sibling->right->color == Color::BLACK){
                        sibling->color= Color::RED;
                    }else if(sibling->left->color == Color::RED){
                        if(itr == itr->parent->left){
                            rotate_right(sibling);
                            rotate_left(itr->parent);
                        }else{
                            sibling->color = Color::RED;
                            sibling->left->color = Color::BLACK;
                            rotate_right(parent);
                        }
                    }else if(sibling->right->color == Color::RED){
                        if(itr == itr->parent->left){
                            sibling->color = Color::RED;
                            sibling->right->color = Color::BLACK;
                            rotate_left(parent);
                        }else{
                            rotate_left(sibling);
                            rotate_right(itr->parent);
                        }
                    }else{
                        if(itr == itr->parent->right){
                            rotate_right(itr->parent);
                        }else{
                            rotate_left(itr->parent);
                        }
                        sibling->color = Color::RED;
                        sibling->left->color = sibling->right->color = Color::BLACK;
                    }
                }else{
                    if(sibling->left->color == Color::BLACK && sibling->right->color == Color::BLACK){
                        sibling->color = Color::RED;
                        repair_recurse(itr->parent);
                    }else if(sibling->left->color == Color::RED){
                        if(itr == itr->parent->left){
                            itr->parent->color = Color::BLACK;
                            rotate_right(sibling);
                            rotate_left(itr->parent);
                            sibling->parent->color = Color::BLACK;
                        }else{
                            sibling->left->color = Color::BLACK;
                            rotate_right(itr->parent);
                        }

                    }else if(sibling->right->color == Color::RED){
                        if(itr == itr->parent->left){
                            sibling->right->color = Color::BLACK;
                            rotate_left(itr->parent);
                        }else{
                            itr->parent->color = Color::BLACK;
                            rotate_left(sibling);
                            rotate_right(itr->parent);
                            sibling->parent->color = Color::BLACK;
                        }
                    }else{
                        if(itr == itr->parent->left){
                            rotate_right(sibling);
                            rotate_left(itr->parent);
                        }else{
                            rotate_left(sibling);
                            rotate_right(itr->parent);
                        }
                        sibling->parent->color = Color::BLACK;
                    }
                }
            }

        }
    }
    void repair_tree_erase(typename red_black_tree::Node* itr){
        Node* t_parent = itr->parent;
        bool is_left_child = (itr == itr->parent->left);
        std::pair<Node*,bool> balanced_sub_tree = pre_balance(itr);
        if(t_parent){
            if(is_left_child){
                t_parent->left = balanced_sub_tree.first;
                t_parent->left->parent = t_parent;
            }else{
                t_parent->right = balanced_sub_tree.first;
                t_parent->right->parent = t_parent;
            }
            if(balanced_sub_tree.second){
                repair_recurse(balanced_sub_tree.first);
            }
        }else{
            m_root = balanced_sub_tree.first;
            m_root->parent = nullptr;
            m_root->color = Color::BLACK;
        }

    }
    bool erase_by_node_ptr(typename red_black_tree::Node* itr){
        if(itr == nullptr) return false;
        if(itr->left && itr->right){
            Node* temp = itr->right;
            while(temp->left)temp = temp->left;

            Color p = temp->color;
            temp->color = itr->color;
            itr->color = p;

            temp->left = itr->left;
            temp->left->parent = temp;
            itr->left = nullptr;

            Node* t_right = temp->right;
            if(temp == itr->right){
                temp->parent = itr->parent;
                if(itr->parent){
                    if(itr->parent->right == itr){
                        itr->parent->right = temp;
                    }else{
                        itr->parent->left = temp;
                    }
                }else{
                    m_root = temp;
                }

                temp->right = itr;
                temp->right->parent = temp;
                itr->right = t_right;
                if(itr->right) itr->right->parent = itr;

            }else{
                temp->right = itr->right;
                temp->right->parent = temp;

                itr->right = t_right;
                if(itr->right) itr->right->parent = itr;

                Node* t_parent = temp->parent;
                if(itr->parent){
                    if(itr == itr->parent->left){
                        itr->parent->left = temp;
                    }else{
                        itr->parent->right= temp;
                    }
                    temp->parent = itr->parent;

                    t_parent->left = itr;
                    itr->parent = t_parent;
                }else{
                    m_root = temp;
                    m_root->parent = nullptr;

                    itr->parent = t_parent;
                    itr->parent->left = itr;
                }
            }

        }
        if(itr == m_root){
            m_root = (itr->right?itr->right:itr->left);
            if(m_root){
                m_root->color = Color::BLACK;
                m_root->parent = nullptr;
            }
            delete itr;
        }else{
            Node* repair_itr = itr->parent;
            if(itr->color == Color::RED){
                if(itr == itr->parent->right){
                    itr->parent->right = nullptr;
                }else{
                    itr->parent->right = nullptr;
                }
                delete itr;
            }else{
                Node* child = (itr->left?itr->left:itr->right);
                if(child){
                    child->parent = itr->parent;
                    if(itr->parent->left == itr){
                        itr->parent->left = child;
                    }else{
                        itr->parent->right = child;
                    }
                    child->color = Color::BLACK;
                    delete itr;
                }else{
                    if(itr->parent->right == itr){
                        itr->parent->right = nullptr;
                    }else{
                        itr->parent->left = nullptr;
                    }
                    delete itr;
                    repair_tree_erase(repair_itr);
                }
            }
        }
        --m_size;
        return true;
    }
    void delete_tree(){
        Advance_ptr<true,Traversal::POST_ORDER> adc;
        Node* itr = adc.ptr_begin(m_root);
        while(itr){
            Node* temp = itr;
            adc(itr);
            delete temp;
        }
    }
};

#endif // RED_BLACK_TREE_H
