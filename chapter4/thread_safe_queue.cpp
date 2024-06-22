

#include<mutex>
#include<condition_variable>
#include<queue>
#include<thread>
#include<iostream>


// 将 std::queue 包装在一个类中，这样不必关心 std::queue 的线程安全问题，不用考虑上锁和解锁的问题
// 用一个互斥量保护整个队列，这样就可以在多个线程中安全地访问队列，但是不高效，因为只有一个线程可以访问队列，更高效的方法是见第六章
template< typename T>
class thread_safe_queue{
private:
    std::queue<T> data_queue;
    mutable std::mutex data_mutex;
    std::condition_variable data_cond;
public:
    void push(T new_value){
        std::lock_guard<std::mutex> lk(data_mutex);
        data_queue.push(new_value);
        data_cond.notify_one();
    }
    void wait_and_pop(T& value){
        std::unique_lock<std::mutex> lk(data_mutex);
        data_cond.wait(lk,[this]{return !this->data_queue.empty();});
        value=data_queue.front();
        data_queue.pop();
    }
    std::shared_ptr<T> wait_and_pop(){
        // 使用std::unique_lock是因为wait函数会在等待期间释放锁，而std::lock_guard不支持这种操作
        std::unique_lock<std::mutex> lk(data_mutex);
        // 在 C++ 中，当你在类的成员函数中访问类的成员变量时，可以省略 this->。编译器会自动理解你是在访问当前对象的成员，所以可以省略 this->。
        data_cond.wait(lk,[this]{return !data_queue.empty();});
        std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
        data_queue.pop();
        return res;
    }
    bool try_pop(T& value){
        std::lock_guard<std::mutex> lk(data_mutex);
        if(data_queue.empty())
            return false;
        value=data_queue.front();
        data_queue.pop();
        return true;
    }
    std::shared_ptr<T> try_pop(){
        std::lock_guard<std::mutex> lk(data_mutex);
        if(data_queue.empty())
            return std::shared_ptr<T>();
        std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
        data_queue.pop();
        return res;
    }
    bool empty() const{
        std::lock_guard<std::mutex> lk(data_mutex);
        return data_queue.empty();
    }
};

int main(){
    thread_safe_queue<int> tsq;
    std::thread t1([&](){
        for(int i=0;i<10;++i){
            tsq.push(i);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
    std::thread t2([&](){
        while(true){
            // 两种wait_and_pop的方式
            /*int value;
            tsq.wait_and_pop(value);     
            std::cout<<value<<std::endl;
            if(value==9)
                break;*/
            auto res =tsq.wait_and_pop();
            std::cout<<*res<<std::endl;
            if(*res==9)
                break;
        }
    });
    t1.join();
    t2.join();
    return 0;
}