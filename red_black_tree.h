#ifndef RED_BLACK_TREE_H
#define RED_BLACK_TREE_H

#include <functional>

enum class Color{BLACK = false,RED = true};
enum class Traversal{PRE_ORDER,IN_ORDER,POST_ORDER};
enum class Direction{LEFT_RIGHT,RIGHT_LEFT};

template<Traversal,Direction>
struct opposite{};

template<> struct opposite<Traversal::PRE_ORDER,Direction::LEFT_RIGHT>{
    static const Traversal _t_value = Traversal::POST_ORDER;
    static const Direction _d_value = Direction::RIGHT_LEFT;
};
template<> struct opposite<Traversal::PRE_ORDER,Direction::RIGHT_LEFT>{
    static const Traversal _t_value = Traversal::POST_ORDER;
    static const Direction _d_value = Direction::LEFT_RIGHT;
};
template<> struct opposite<Traversal::IN_ORDER,Direction::LEFT_RIGHT>{
    static const Traversal _t_value = Traversal::IN_ORDER;
    static const Direction _d_value = Direction::RIGHT_LEFT;
};
template<> struct opposite<Traversal::IN_ORDER,Direction::RIGHT_LEFT>{
    static const Traversal _t_value = Traversal::IN_ORDER;
    static const Direction _d_value = Direction::LEFT_RIGHT;
};
template<> struct opposite<Traversal::POST_ORDER,Direction::LEFT_RIGHT>{
    static const Traversal _t_value = Traversal::PRE_ORDER;
    static const Direction _d_value = Direction::RIGHT_LEFT;
};
template<> struct opposite<Traversal::POST_ORDER,Direction::RIGHT_LEFT>{
    static const Traversal _t_value = Traversal::PRE_ORDER;
    static const Direction _d_value = Direction::LEFT_RIGHT;
};

template<typename Key,typename Value,class Compare = std::less<Key>>
class red_black_tree{
    struct Node{
        Color color;
        Node *left,*right,*parent;
        std::pair<const Key,Value> m_data;

        Node(const std::pair<Key,Value>& data): color(Color::RED),left(nullptr),right(nullptr),parent(nullptr),m_data(data){}
        Node(std::pair<Key,Value>&& data):      color(Color::RED),left(nullptr),right(nullptr),parent(nullptr),m_data(std::move(data)){}
        Node(const Node& cpy):color(cpy.color),left(nullptr),right(nullptr),parent(nullptr),m_data(cpy.m_data){}
        Node(Node&& cpy):color(cpy.color),left(nullptr),right(nullptr),parent(nullptr),m_data(std::move(cpy.m_data)){}
        Node& operator=(const Node&) = delete;

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
    template<bool,Traversal,Direction>
    struct Advance_ptr{
        typename red_black_tree::Node* ptr_begin(typename red_black_tree::Node* itr){}
        typename red_black_tree::Node* ptr_end  (typename red_black_tree::Node* itr){}
        void operator()(typename red_black_tree::Node*& itr){}
    };
    #define __PRE_ORDER_ADVANCE__(X,Y,itr)do{\
    if(itr){\
        if(itr->X) itr = itr->X;\
        else if(itr->Y)itr = itr->Y;\
        else{\
            bool not_found = true;\
            while(not_found && itr->parent){\
                if(itr->parent->Y!=itr && itr->parent->Y){\
                    itr = itr->parent->Y;\
                    not_found = false;\
                }else itr = itr->parent;\
            }\
            if(not_found)itr = nullptr;\
        }\
    }}while(false)
    #define __PRE_ORDER_END__(X,Y,itr)do{\
    if(itr){\
        bool not_found = true;\
        while(not_found && itr->parent){\
            if(itr->parent->Y!=itr && itr->parent->Y){\
                itr = itr->parent->Y;\
                not_found = false;\
            }else itr = itr->parent;\
        }\
        if(not_found)itr = nullptr;\
    }}while(false)

    template<bool D>struct Advance_ptr<D,Traversal::PRE_ORDER,Direction::LEFT_RIGHT>{
        typename red_black_tree::Node* ptr_begin(typename red_black_tree::Node* itr){
            return itr;
        }
        typename red_black_tree::Node* ptr_end  (typename red_black_tree::Node* itr){
            __PRE_ORDER_END__(left,right,itr);
            return itr;
        }
        void operator()(typename red_black_tree::Node*& itr){
            __PRE_ORDER_ADVANCE__(left,right,itr);
        }
    };

    template<bool D>struct Advance_ptr<D,Traversal::PRE_ORDER,Direction::RIGHT_LEFT>{
        typename red_black_tree::Node* ptr_begin(typename red_black_tree::Node* itr){
            return itr;
        }
        typename red_black_tree::Node* ptr_end  (typename red_black_tree::Node* itr){
            __PRE_ORDER_END__(right,left,itr);
            return itr;
        }
        void operator()(typename red_black_tree::Node*& itr){
            __PRE_ORDER_ADVANCE__(right,left,itr);
        }
    };
    #undef __PRE_ORDER_ADVANCE__
    #undef __PRE_ORDER_END__

    #define __IN_ORDER_ADVANCE__(X,Y,itr)do{\
    if(itr){\
        if(itr->Y){\
            itr = itr->Y;\
            while(itr->X) itr = itr->X;\
        }else{\
            bool not_found = true;\
            while(not_found && itr->parent){\
                if(itr->parent->Y!=itr)not_found = false;\
                itr = itr->parent;\
            }\
            if(not_found) itr = nullptr;\
        }\
    }}while(false)
    #define __IN_ORDER_END__(X,Y,itr)do{\
    if(itr){\
        bool not_found = true;\
        while(not_found && itr->parent){\
            if(itr->parent->Y!=itr)not_found = false;\
            itr = itr->parent;\
        }\
        if(not_found) itr = nullptr;\
    }}while(false)

    template<bool D> struct Advance_ptr<D,Traversal::IN_ORDER,Direction::LEFT_RIGHT>{
        typename red_black_tree::Node* ptr_begin(typename red_black_tree::Node* itr){
            while(itr->left) itr = itr->left;
            return itr;
        }
        typename red_black_tree::Node* ptr_end  (typename red_black_tree::Node* itr){
            __IN_ORDER_END__(left,right,itr);
            return itr;
        }
        void operator()(typename red_black_tree::Node*& itr){
            __IN_ORDER_ADVANCE__(left,right,itr);
        }
    };
    template<bool D> struct Advance_ptr<D,Traversal::IN_ORDER,Direction::RIGHT_LEFT>{
        typename red_black_tree::Node* ptr_begin(typename red_black_tree::Node* itr){
            while(itr->right) itr = itr->right;
            return itr;
        }
        typename red_black_tree::Node* ptr_end  (typename red_black_tree::Node* itr){
            __IN_ORDER_END__(right,left,itr);
            return itr;
        }
        void operator()(typename red_black_tree::Node*& itr){
            __IN_ORDER_ADVANCE__(right,left,itr);
        }
    };
    #undef __IN_ORDER_ADVANCE__
    #undef __IN_ORDER_END__

    #define __POST_ORDER_ADVANCE__(X,Y,itr)do{\
    if(itr){\
        if(itr->parent){\
            if(itr->parent->Y==itr || itr->parent->Y==nullptr){\
                itr = itr->parent;\
            }else{\
                itr = itr->parent->Y;\
                while(itr->X || itr->Y) {\
                    if(itr->X)itr = itr->X;\
                    else if(itr->Y)itr =itr->Y;\
                }\
            }\
        }else itr = nullptr;\
    }}while(false)
    #define __POST_ORDER_BEGIN__(X,Y,itr)do{\
    while(itr->X || itr->Y) {\
        if(itr->X)itr = itr->X;\
        else if(itr->Y)itr =itr->Y;\
    }}while(false)

    template<bool D> struct Advance_ptr<D,Traversal::POST_ORDER,Direction::LEFT_RIGHT>{
        typename red_black_tree::Node* ptr_begin(typename red_black_tree::Node* itr){
            __POST_ORDER_BEGIN__(left,right,itr);
            return itr;
        }
        typename red_black_tree::Node* ptr_end  (typename red_black_tree::Node* itr){
            this->operator()(itr);
            return itr;
        }
        void operator()(typename red_black_tree::Node*& itr){
            __POST_ORDER_ADVANCE__(left,right,itr);
        }
    };
    template<bool D> struct Advance_ptr<D,Traversal::POST_ORDER,Direction::RIGHT_LEFT>{
        typename red_black_tree::Node* ptr_begin(typename red_black_tree::Node* itr){
            __POST_ORDER_BEGIN__(right,left,itr);
            return itr;
        }
        typename red_black_tree::Node* ptr_end  (typename red_black_tree::Node* itr){
            this->operator()(itr);
            return itr;
        }
        void operator()(typename red_black_tree::Node*& itr){
            __POST_ORDER_ADVANCE__(right,left,itr);
        }
    };
    #undef __POST_ORDER_BEGIN__
    #undef __POST_ORDER_ADVANCE__
    template<Traversal _T,Direction _D,class Storable>
    class m_iterator{
        typename red_black_tree::Node* m_ptr;
    public:
        typedef m_iterator self_type;
        typedef Storable value_type;
        typedef Storable& reference;
        typedef Storable* pointer;
        typedef std::bidirectional_iterator_tag iterator_category;
        typedef int difference_type;

        m_iterator(typename red_black_tree::Node* ptr):m_ptr(ptr){}
        m_iterator(const m_iterator& rhs):m_ptr(rhs.m_ptr){}
        self_type& operator=(const self_type& rhs){m_ptr = rhs.m_ptr;}
        reference operator*(){return m_ptr->m_data;}
        pointer operator->(){return &(m_ptr->m_data);}
        bool operator==(const m_iterator& rhs){return m_ptr == rhs.m_ptr;}
        bool operator!=(const m_iterator& rhs){return m_ptr != rhs.m_ptr;}

        self_type& operator++(){
            Advance_ptr<true,_T,_D>()(m_ptr);
            return *this;
        }
        self_type operator++(int){
            m_iterator temp(*this);
            ++(*this);
            return temp;
        }
        self_type& operator--(){
            Advance_ptr<true,opposite<_T,_D>::_t_value,opposite<_T,_D>::_d_value>()(m_ptr);
            return *this;
        }
        self_type operator--(int){
            m_iterator temp(*this);
            --(*this);
            return temp;
        }
        template<Traversal __T,Direction __D>
        operator m_iterator<__T,__D,Storable>(){
            return m_iterator<__T,__D,Storable>(m_ptr);
        }
        template<Traversal __T,Direction __D>
        operator m_iterator<__T,__D,const Storable>(){
            return m_iterator<__T,__D,const Storable>(m_ptr);
        }
        friend class red_black_tree;
    };

public:
    template<Traversal _T>
    using iterator = m_iterator<_T,Direction::LEFT_RIGHT,std::pair<const Key,Value>>;
    template<Traversal _T>
    using const_iterator = m_iterator<_T,Direction::LEFT_RIGHT,const std::pair<const Key,Value>>;
    template<Traversal _T>
    using reverse_iterator = m_iterator<_T,Direction::RIGHT_LEFT,std::pair<const Key,Value>>;
    template<Traversal _T>
    using const_reverse_iterator = m_iterator<_T,Direction::RIGHT_LEFT,const std::pair<const Key,Value>>;

    std::pair<bool,iterator<Traversal::IN_ORDER>> insert(const Key& key,const Value& value){
        Node **itr = &m_root,*par = nullptr;
        Compare cmp;
        while(*itr){
            par = *itr;
            if(cmp((*itr)->m_data.first,key))itr = &((*itr)->right);
            else if(cmp(key,(*itr)->m_data.first)) itr = &((*itr)->left);
            else return std::make_pair(false,iterator<Traversal::IN_ORDER>(*itr));
        }
        (*itr) = new Node(std::make_pair(key,value));
        (*itr)->parent = par;
        ++m_size;
        repair_tree_insert(*itr);
        std::cout<<"succsesfuly inserted: "<<key<<". At"<<*itr<<". parent: "<<par<<". current size: "<<m_size<<std::endl;
        return std::make_pair(true,iterator<Traversal::IN_ORDER>(*itr));
    }
    bool erase(const Key& key){
        return erase_by_node_ptr(find_node_by_key(key));
    }
    bool erase(iterator<Traversal::IN_ORDER> itr){
        return erase_by_node_ptr(itr->m_ptr);
    }
    bool erase(reverse_iterator<Traversal::IN_ORDER> itr){
        return erase_by_node_ptr(itr->m_ptr);
    }
    void buta_kiir(Node* itr,int szam=1){
        if(itr){
            buta_kiir(itr->left,szam+1);
            std::cout<<std::string(szam,' ')<<itr->m_data.first<<","<<(itr->color==Color::BLACK?"black":"red");
            if(itr->parent){
                std::cout<<". szulo: "<<itr->parent->m_data.first;
            }std::cout<<std::endl;
            buta_kiir(itr->right,szam+1);
        }
    }
    Node* root(){
        return m_root;
    }
    red_black_tree():m_root(nullptr),m_size(0) {}
    red_black_tree(const red_black_tree& cpy):m_root(nullptr),m_size(0){
        this->construct_from_tree(cpy);
    }
    red_black_tree(red_black_tree&& cpy):m_root(cpy.m_root),m_size(cpy.m_size){
        cpy.m_root = nullptr;
        cpy.m_size = 0;
    }

    red_black_tree& operator=(const red_black_tree& rhs){
        if(&rhs != this){
            this->delete_tree();
            this->construct_from_tree(rhs);
        }
        return *this;
    }
    red_black_tree& operator=(red_black_tree&& rhs){
        this->m_root = rhs.m_root;
        this->m_size = rhs.m_size;
        rhs.m_size = 0;
        rhs.m_root = nullptr;
    }

    ~red_black_tree() {
        this->delete_tree();
    }

    template<Traversal Tr>
    iterator<Tr> begin(){
        return iterator<Tr>(Advance_ptr<true,Tr,Direction::LEFT_RIGHT>().ptr_begin(m_root));
    }
    template<Traversal Tr>
    iterator<Tr> begin(iterator<Tr> itr){
        return iterator<Tr>( Advance_ptr<true,Tr,Direction::LEFT_RIGHT>().ptr_begin(itr.m_ptr) );
    }
    template<Traversal Tr>
    const_iterator<Tr> cbegin()const{
        return const_iterator<Tr>(Advance_ptr<true,Tr,Direction::LEFT_RIGHT>().ptr_begin(m_root));
    }
    template<Traversal Tr>
    const_iterator<Tr> cbegin(const_iterator<Tr> itr)const{
        return const_iterator<Tr>( Advance_ptr<true,Tr,Direction::LEFT_RIGHT>().ptr_begin(itr.m_ptr) );
    }

    template<Traversal Tr>
    iterator<Tr> end(iterator<Tr> itr = iterator<Tr>(nullptr)){
        return iterator<Tr>( Advance_ptr<true,Tr,Direction::LEFT_RIGHT>().ptr_end(itr.m_ptr) );
    }
    template<Traversal Tr>
    const_iterator<Tr> cend(const_iterator<Tr> itr = const_iterator<Tr>(nullptr))const{
        return const_iterator<Tr>( Advance_ptr<true,Tr,Direction::LEFT_RIGHT>().ptr_end(itr.m_ptr) );
    }

    template<Traversal Tr>
    reverse_iterator<Tr> rbegin(){
        return reverse_iterator<Tr>(Advance_ptr<true,Tr,Direction::RIGHT_LEFT>().ptr_begin(m_root));
    }
    template<Traversal Tr>
    reverse_iterator<Tr> rbegin(reverse_iterator<Tr> itr){
        return reverse_iterator<Tr>( Advance_ptr<true,Tr,Direction::RIGHT_LEFT>().ptr_begin(itr.m_ptr) );
    }
    template<Traversal Tr>
    const_reverse_iterator<Tr> crbegin()const{
        return const_reverse_iterator<Tr>(Advance_ptr<true,Tr,Direction::RIGHT_LEFT>().ptr_begin(m_root));
    }
    template<Traversal Tr>
    const_reverse_iterator<Tr> crbegin(const_reverse_iterator<Tr> itr)const{
        return const_reverse_iterator<Tr>( Advance_ptr<true,Tr,Direction::RIGHT_LEFT>().ptr_begin(itr.m_ptr) );
    }

    template<Traversal Tr>
    reverse_iterator<Tr> rend(reverse_iterator<Tr> itr = reverse_iterator<Tr>(nullptr)){
        return reverse_iterator<Tr>( Advance_ptr<true,Tr,Direction::RIGHT_LEFT>().ptr_end(itr.m_ptr) );
    }
    template<Traversal Tr>
    const_reverse_iterator<Tr> crend(const_reverse_iterator<Tr> itr = const_iterator<Tr>(nullptr))const{
        return const_reverse_iterator<Tr>( Advance_ptr<true,Tr,Direction::RIGHT_LEFT>().ptr_end(itr.m_ptr) );
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
        Advance_ptr<true,Traversal::IN_ORDER,Direction::LEFT_RIGHT> advance;
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
            should_repair_further = (node_ptr_arr[0]->color==node_ptr_arr[1]->color);
            node_ptr_arr[0]->color = Color::BLACK;
            node_ptr_arr[1]->color = Color::RED;
            break;
        case 3:
            node_ptr_arr[1]->color = itr->color;
            node_ptr_arr[0]->color = node_ptr_arr[2]->color = Color::BLACK;
            break;
        case 4:
            node_ptr_arr[1]->color = itr->color;
            node_ptr_arr[0]->color = node_ptr_arr[2]->color = Color::BLACK;
            node_ptr_arr[3]->color = Color::RED;

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
            default: std::cout<<"\nERROR!\n";
        }
        return std::make_pair(construct_sub_tree_from(node_ptr_arr,0,node_count-1),should_repair_further);
    }
    void repair_recurse(typename red_black_tree::Node* itr){
        if(itr->parent!=nullptr){
            Node* sibling = (itr->parent->left==itr? itr->parent->right:itr->parent->left);
            if(sibling->color == Color::RED){
                sibling->color = Color::BLACK;
                if(itr == itr->parent->left){
                    rotate_left(itr->parent);
                }else{
                    rotate_right(itr->parent);
                }
                itr->parent->color = Color::RED;
                sibling = (itr->parent->left==itr? itr->parent->right:itr->parent->left);
            }
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
    void repair_tree_erase(typename red_black_tree::Node* itr){
        Node* t_parent = itr->parent;
        bool is_left_child = (itr->parent && itr == itr->parent->left);
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
        }
        m_root->color = Color::BLACK;
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
        Advance_ptr<true,Traversal::POST_ORDER,Direction::LEFT_RIGHT> adc;
        Node* itr = adc.ptr_begin(m_root);
        while(itr){
            Node* temp = itr;
            adc(itr);
            delete temp;
        }
    }
    std::pair<typename red_black_tree::Node*,std::size_t> construct_from_tree(const red_black_tree& cpy){
        if(cpy.m_root){
            Node* root = new Node(cpy.m_root.m_data);
            Node** itr = &root;
            Node* c_itr = cpy.m_root;
            while(c_itr){
                if(c_itr->left){
                    c_itr = c_itr->left;
                    (*itr)->left = new Node(*c_itr);
                    (*itr)->left->parent = (*itr);
                    itr = &((*itr)->left);
                }else if(c_itr->right){
                    c_itr = c_itr->right;
                    (*itr)->right= new Node(*c_itr);
                    (*itr)->right->parent= (*itr);
                    itr = &((*itr)->right);
                }else{
                    bool not_found = true;
                    while(c_itr->parent && not_found){
                        itr = &((*itr)->parent);
                        if(c_itr != c_itr->parent->right && c_itr->parent->right){
                            c_itr = c_itr->parent->right;
                            (*itr)->right = new Node(*c_itr);
                            (*itr)->right->parent = (*itr);
                            itr = &((*itr)->right);
                            not_found = false;
                        }else{
                            c_itr = c_itr->parent;
                        }
                    }
                    if(not_found){
                        c_itr = nullptr;
                    }
                }
            }
            return std::make_pair(root,cpy.m_size);
        }
        return std::make_pair(nullptr,0);
    }
};

#endif // RED_BLACK_TREE_H
