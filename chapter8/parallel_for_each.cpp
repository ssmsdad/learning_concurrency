

#include <bits/types/struct_sched_param.h>
#include <future>
#include<iostream>
#include <iterator>
#include<vector>
#include<thread>
#include<algorithm>


class join_threads
{
    std::vector<std::thread>& threads;
public:
    explicit join_threads(std::vector<std::thread>& threads_):
    threads(threads_)
    {}
    ~join_threads()
    {
        for(unsigned long i=0;i<threads.size();++i)
        {   
            if(threads[i].joinable())
            threads[i].join();
        }
    }
};  

template<typename Iterator, typename T>
void parallel_for_each(Iterator first,Iterator last,T f){
    unsigned long const length=std::distance(first,last);
    if(!length)
        return;
    unsigned long const min_per_thread=25;
    unsigned long const max_threads=(length+min_per_thread-1)/min_per_thread;
    unsigned long const hardware_threads=std::thread::hardware_concurrency();
    unsigned long const num_threads=std::min(hardware_threads!=0?hardware_threads:2,max_threads);
    unsigned long const block_size=length/num_threads;
    
    std::vector<std::future<void>> futures(num_threads-1);
    std::vector<std::thread> threads(num_threads-1);
    join_threads joiner(threads);
    Iterator block_start=first;
    for(unsigned long i=0;i<(num_threads-1);i++){
        Iterator block_end=block_start;
        std::advance(block_end,block_size);
        // 捕获列表中的=表示以值的方式捕获所有局部变量，这样就可以在lambda表达式中使用这些局部变量
        // 虽然值捕获可能涉及到复制成本，但在这个场景中，被捕获的变量（迭代器和函数对象）通常是轻量级的，复制成本较低。相比之下，保证引用捕获的变量在多线程环境下安全使用的复杂性和潜在风险，这种复制成本是可以接受的
        std::packaged_task<void(void)> task([=](){
            std::for_each(block_start,block_end,f);
        });
        futures[i]=task.get_future();
        threads[i]=std::thread(std::move(task));
        block_start=block_end;
    }
    std::for_each(block_start,last,f);
    // 这里虽然不需要返回值，但是可以使用std::future::get()将抛出的异常传递回原始线程，而不会调用std::terminate()终止程序
    for(unsigned long i=0;i<(num_threads-1);++i)
    {
        futures[i].get(); 
    }
}
void do_work(int &item){
    std::cout<<item<<std::endl;
}

int main(){
    std::vector<int> v(10,1);
    parallel_for_each(v.begin(),v.end(),do_work);
    return 0;
}