

#include <algorithm>
#include<iostream>
#include <memory>
#include<mutex>
#include <thread>


template<typename T>
class threadsafe_list{
    struct node{
        std::mutex m;
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
        node():next(){}
        node(T const& value):data(std::make_shared<T>(value)){}
    };
    node head;
    
    public:
    threadsafe_list(){}
    ~threadsafe_list(){
        // 因为head是类的成员，所以默认析构只会删除head节点，所以需要手动删除其他节点
        remove_if([](node const&){return true;});
    }
    threadsafe_list(threadsafe_list const& other)=delete;
    threadsafe_list& operator=(threadsafe_list const& other)=delete;

    void push_front(T const& value){
        std::unique_ptr<node> new_node(new node(value));
        std::lock_guard<std::mutex> lk(head.m);
        new_node->next=std::move(head.next);
        head.next=std::move(new_node);
    }

    template<typename Function>
    void for_each(Function f){
        node* current=&head;
        std::unique_lock<std::mutex> lk(head.m);
        while(node* const next=current->next.get()){
            std::unique_lock<std::mutex> next_lk(next->m);
            lk.unlock();
            f(*next->data);
            current=next;
            lk=std::move(next_lk);
        }
    }

    template<typename Predicate>
    std::shared_ptr<T> find_first_if(Predicate p){
        node* current=&head;
        std::unique_lock<std::mutex> lk(head.m);
        while(node* const next=current->next.get()){
            std::unique_lock<std::mutex> next_lk(next->m);
            lk.unlock();
            if(p(*next->data)){
                return next->data;
            }
            current=next;
            lk=std::move(next_lk);
        }
        return std::shared_ptr<T>();
    }

    template<typename Predicate>
    void remove_if(Predicate p){
        node* current=&head;
        std::unique_lock<std::mutex> lk(head.m);
        while(node* const next=current->next.get()){
            std::unique_lock<std::mutex> next_lk(next->m);
            if(p(*next->data)){
                std::unique_ptr<node> old_next=std::move(current->next);
                current->next=std::move(next->next);
                next_lk.unlock();
            }
            else{
                lk.unlock();
                current=next;
                lk=std::move(next_lk);
            }
        }
    }
};

int main(){
    threadsafe_list<int> list;
    std::thread t1([&list]{
        list.push_front(1);
        list.push_front(2);
        list.push_front(3);
        list.push_front(4);
        list.push_front(5);
    });
    std::thread t2([&list]{
        list.for_each([](int& value){std::cout<<value<<std::endl;});
        std::shared_ptr<int> data = list.find_first_if([](int &value){return value==3;});
        std::cout<<*data<<std::endl;
    });
    std::thread t3([&list]{
        list.remove_if([](int &value){return value==3;});
        list.for_each([](int& value){std::cout<<value<<std::endl;});
    });
    t1.join();
    t2.join();
    t3.join();
    return 0;
    
}
