

#include<iostream>
#include <memory>
#include<vector>
#include<thread>
#include<future>


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
    /* 
    这里注意必须使用std::move(new_value)来调用调用T的移动构造函数，否则会调用拷贝构造函数，执行work_queue.push(std::move(task))时，
    进入work_queue的只是task对象的副本，而不是task对象本身。在执行impl_type(F&& f_):f(std::move(f_)){}时，会将task对象的副本的f成员移动到impl对象的f成员中，
    但是由于c++不允许将非常量左值引用（task的副本）绑定到右值（F&& f）上，所以会报错。
    */
    std::shared_ptr<T> new_data(std::make_shared<T>(std::move(new_value)));
    std::unique_ptr<node> p(std::make_unique<node>());
    {
        std::unique_lock<std::mutex> tail_lk(tail_mutex);
        tail->data=std::move(new_data);
        tail->next=std::move(p);
        tail=tail->next.get();
    }
    data_cond.notify_one();
}



template<typename T>
bool thread_safe_queue<T>::empty(){
    std::lock_guard<T> head_lk(head_mutex);
    return head.get()==get_tail();
}



class join_threads{
    std::vector<std::thread>& threads;
public:
    explicit join_threads(std::vector<std::thread>& threads_):
    threads(threads_)
    {}
    ~join_threads(){
        for(unsigned long i=0;i<threads.size();++i){
            if(threads[i].joinable())
                threads[i].join();
        }
    }
};

// function_wrapper类的作用是将函数封装成一个类，这样就可以将函数作为参数传递给线程
class function_wrapper{
    struct impl_base{
        virtual void call()=0;
        virtual ~impl_base(){}
    };
    std::unique_ptr<impl_base> impl;
    template<typename F>
    struct impl_type:impl_base{
        F f;
        impl_type(F&& f_):f(std::move(f_)){}
        void call(){f();}
    };
public:
    template<typename F>
    function_wrapper(F&& f):
    impl(std::make_unique<impl_type<F>>(std::move(f)))
    {}
    void operator()(){impl->call();}
    function_wrapper()=default;
    function_wrapper(function_wrapper&& other):
    impl(std::move(other.impl))
    {}
    function_wrapper& operator=(function_wrapper&& other){
        impl=std::move(other.impl);
        return *this;
    }
    function_wrapper(const function_wrapper&)=delete;
    function_wrapper& operator=(const function_wrapper&)=delete;
};


class thread_pool{
    std::atomic_bool done;
    thread_safe_queue<function_wrapper> work_queue;
    std::vector<std::thread> threads;
    join_threads joiner;
    // 每一个线程都在循环检查done标志位，如果done为false，线程就会从队列中取出一个任务并执行
    // 如果执行完一个任务后，会继续从队列中取出下一个任务
    void worker_thread(){
        while(!done){
            function_wrapper task;
            if(work_queue.try_pop(task)){
                task();
            }else{
                // 如果队列为空，线程就会让出CPU，这样可以避免线程空转
                std::this_thread::yield();
            }
        }
    }
public:
    thread_pool():
        done(false),joiner(threads){
        unsigned const thread_count=std::thread::hardware_concurrency();
        try{
            for(unsigned i=0;i<thread_count;++i){
                threads.push_back(std::thread(&thread_pool::worker_thread,this));
            }
        }catch(...){
            done=true;
            throw;
        }
    }
    ~thread_pool(){
        done=true;
    }
    template<typename FunctionType>
    std::future<std::result_of_t<FunctionType()>> submit(FunctionType f){
        // std::result_of_t<FunctionType()>是函数f的返回值类型
        using result_type=std::result_of_t<FunctionType()>;
        std::packaged_task<result_type()> task(std::move(f));
        std::future<result_type> res(task.get_future());
        work_queue.push(std::move(task));
        return res;
    }
};


int main(){
    thread_pool pool;
    std::vector<std::future<int>> results;
    for(int i=0;i<8;++i){
        results.push_back(pool.submit([i]{
            return i*i;
        }));
    }
    for(auto&& result:results){
        std::cout<<result.get()<<std::endl;
    }
    return 0;
}