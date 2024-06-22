

#include<iostream>
#include <memory>
#include<mutex>
#include<condition_variable>
#include <utility>
#include<thread>

template<typename T>
class thread_safe_queue{
private:
    struct node{
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
    };
    std::mutex head_mutex;
    std::unique_ptr<node> head;
    std::mutex tail_mutex;
    node* tail;
    std::condition_variable data_cond;
    node* get_tail(){
        std::lock_guard<std::mutex> tail_lk(tail_mutex);
        return tail;
    }
    std::unique_ptr<node> pop_head(){
        std::unique_ptr<node> old_head=std::move(head);
        head=std::move(old_head->next);
        return old_head;
    }
    std::unique_lock<std::mutex> wait_for_data(){
        std::unique_lock<std::mutex> head_lk(head_mutex);
        data_cond.wait(head_lk,[this]{return head.get()!=get_tail();});
        return std::move(head_lk);
    }
    std::unique_ptr<node> wait_pop_head(){
        std::unique_lock<std::mutex> head_lk(wait_for_data());
        return pop_head();
    }
    std::unique_ptr<node> wait_pop_head(T& value){
        // 条件变量常常与std::unique_lock<std::mutex>一起使用，因为std::unique_lock更加灵活，可以在后续的代码中通过lock()和unlock()函数来控制锁的加锁和解锁。
        std::unique_lock<std::mutex> head_lk(wait_for_data());
        value=std::move(*head->data);
        return pop_head();
    }
    std::unique_ptr<node> try_pop_head(){
        std::lock_guard<std::mutex> head_lk(head_mutex);
        if(head.get()==get_tail()){
            return std::unique_ptr<node>();
        }
        return pop_head();
    }
    std::unique_ptr<node> try_pop_head(T& value){
        std::lock_guard<std::mutex> head_lk(head_mutex);
        if(head.get()==get_tail()){
            return std::unique_ptr<node>();
        }
        value=std::move(*head->data);
        return pop_head();
    }
public:
    thread_safe_queue():
        head(new node),tail(head.get()){}
    thread_safe_queue(const thread_safe_queue& other)=delete;
    thread_safe_queue& operator=(const thread_safe_queue& other)=delete;

    std::shared_ptr<T> try_pop();
    bool try_pop(T& value);
    std::shared_ptr<T> wait_and_pop();
    void wait_and_pop(T& value);
    void push(T new_value);
    bool empty();
};

template<typename T>
std::shared_ptr<T> thread_safe_queue<T>::try_pop(){
    std::unique_ptr<node> const old_head=try_pop_head();
    return old_head?old_head->data:std::shared_ptr<T>();
}

template<typename T>
bool thread_safe_queue<T>::try_pop(T &value){
    std::unique_ptr<node> const old_head=try_pop_head(value);
    return old_head?true:false;
}

template<typename T>
std::shared_ptr<T> thread_safe_queue<T>::wait_and_pop(){
    std::unique_ptr<node> const old_head=wait_pop_head();
    return old_head->data;
}

template<typename T>
void thread_safe_queue<T>::wait_and_pop(T& value){
    std::unique_ptr<node> const old_head=wait_pop_head(value);
}

template<typename T>
void thread_safe_queue<T>::push(T new_value){
    std::shared_ptr<T> new_data(std::make_shared<T>(new_value));
    std::unique_ptr<node> p(std::make_unique<node>());
    {
        std::unique_lock<std::mutex> tail_lk(tail_mutex);
        tail->data=std::move(new_data);
        tail->next=std::move(p);
        tail=tail->next.get();
    }
    // 通知等待线程比较慢，所以不要在锁内调用
    data_cond.notify_one();
}

template<typename T>
bool thread_safe_queue<T>::empty(){
    std::lock_guard<T> head_lk(head_mutex);
    return head.get()==get_tail();
}



int main(){
    std::shared_ptr<int> p;
    thread_safe_queue<int> q;
    std::thread t1 = std::thread([&q, &p]() {
    p = q.wait_and_pop();
    if(p) {
        std::cout << *p << std::endl;
    }
});
    std::thread t2 = std::thread(&thread_safe_queue<int>::push, &q, 1);
    std::thread t3 = std::thread(&thread_safe_queue<int>::push, &q, 2);
    std::thread t4 = std::thread(&thread_safe_queue<int>::push, &q, 3);
    int value;
    std::thread t5 = std::thread([&q, &value]() {;
    q.wait_and_pop(value);
    std::cout << value << std::endl;
});
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    return 0;
}