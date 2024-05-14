

#include<iostream>
#include<mutex>


// 一个简单的层次锁
// hierarchical_mutex 可以确保一个线程在尝试获取一个更高层次的锁之前，必须先释放所有更低层次的锁，本质是使用锁的层次值来设置线程的层次级
class hierarchical_mutex{
    std::mutex internal_mutex;
    unsigned long const hierarchy_value;
    unsigned long previous_hierarchy_value;
    static thread_local unsigned long this_thread_hierarchy_value;
    void check_for_hierarchy_violation(){
        if(this_thread_hierarchy_value<=hierarchy_value)
            throw std::logic_error("mutex hierarchy violated");
    }
    void update_hierarchy_value(){
        previous_hierarchy_value=this_thread_hierarchy_value;
        // 线程的层次值会被设置为这个锁的层次值
        this_thread_hierarchy_value=hierarchy_value;
    }
    public:
        explicit hierarchical_mutex(unsigned long value):
            hierarchy_value(value),
            previous_hierarchy_value(0){}
        void lock(){
            check_for_hierarchy_violation();
            internal_mutex.lock();
            update_hierarchy_value();
        }
        void unlock(){
            this_thread_hierarchy_value=previous_hierarchy_value;
            internal_mutex.unlock();
             
        }
 
         bool try_lock(){
            check_for_hierarchy_violation();
            if(!internal_mutex.try_lock())
                return false;
            update_hierarchy_value();
            return true;
        }
};

// 包含ULONG_MAX， unsigned long 类型可以表示的最大值
#include <climits>  

thread_local unsigned long hierarchical_mutex::this_thread_hierarchy_value(ULONG_MAX);

hierarchical_mutex high_level_mutex(10000);
hierarchical_mutex low_level_mutex(5000);
hierarchical_mutex other_mutex(6000);

int do_low_level_stuff(){
    std::cout<<"do_low_level_stuff"<<std::endl;
    return 1;
}

int low_level_func(){
    std::lock_guard<hierarchical_mutex> lk(low_level_mutex);
    return do_low_level_stuff();
}

void high_level_stuff(int some_param){
    std::cout<<"high_level_stuff"<<std::endl;
}

void high_level_func(){
    std::lock_guard<hierarchical_mutex> lk(high_level_mutex);
    high_level_stuff(low_level_func());
}

void thread_a(){
    high_level_func();
}

void do_other_stuff(){
    std::cout<<"do_other_stuff"<<std::endl;
}

void other_stuff(){
    high_level_func();
    do_other_stuff();
}

// 线程b不会正确运行，因为它先锁定了other_mutex，值为6000，然后尝试锁定high_level_mutex，值为10000
// 不可以从低级别锁升级到高级别锁
void thread_b(){
    std::lock_guard<hierarchical_mutex> lk(other_mutex);
    other_stuff();
}

#include<thread>

int main(){
    std::thread t1(thread_a);
    std::thread t2(thread_b);
    t1.join();
    t2.join();
    return 0;
}