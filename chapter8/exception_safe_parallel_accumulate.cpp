



#include <future>
#include<iostream>
#include <iterator>
#include<numeric>
#include<thread>
#include<vector>

/*类比第二章没有异常安全的parallel_accumulate*/
template<typename Iterator, typename T>
struct accumulate_block{
    T operator()(Iterator first,Iterator last){
        return std::accumulate(first,last,T());
    }
};

// join_threads类的作用是确保在析构时所有线程都被join，防止线程泄漏
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

template<typename Iterator,typename T>
T parallel_accumulate(Iterator first,Iterator last,T init){
    unsigned long const length=std::distance(first,last);
    if(!length)
        return init;
    unsigned long const min_per_thread=25;
    // 将最大线程数向上取整，如果 length 是101，那么 (101+25-1)/25 将得到5，这意味着你需要5个线程来处理这101个数据
    unsigned long const max_threads=(length+min_per_thread-1)/min_per_thread;
    // 获取硬件支持的线程数
    unsigned long const hardware_threads=std::thread::hardware_concurrency();
    std::cout<<"hardware_threads:"<<hardware_threads<<std::endl;
    // 线程数取硬件支持的线程数和最大线程数的最小值
    unsigned long const num_threads=std::min(hardware_threads!=0?hardware_threads:2,max_threads);
    std::cout<<"num_threads:"<<num_threads<<std::endl;
    // 每个线程处理的数据块大小
    unsigned long const block_size=length/num_threads;
    std::cout<<"block_size:"<<block_size<<std::endl;
    
    std::vector<std::future<T>> futures(num_threads-1);
    std::vector<std::thread> threads(num_threads-1);
    join_threads joiner(threads);
    Iterator block_start=first;
    for(unsigned long i=0;i<(num_threads-1);i++){
        Iterator block_end=block_start;
        std::advance(block_end,block_size);
        // 使用std::packaged_task,如果任务在执行过程中抛出了异常，这个异常可以被传递回原始线程，并在获取返回值时重新抛出,而不会调用std::terminate()终止程序
        std::packaged_task<T(Iterator,Iterator)> task((accumulate_block<Iterator,T>()));
        futures[i]=task.get_future();
        threads[i]=std::thread(std::move(task),block_start,block_end); // 6
        block_start=block_end;
    }
    T last_result=accumulate_block<Iterator,T>()(block_start,last);
    T result=init;
    for(unsigned long i=0;i<(num_threads-1);i++){
        result+=futures[i].get();
    }
    result+=last_result;
    return result;
}

int main(){
    std::vector<int> v(1000,1);
    int sum=parallel_accumulate(v.begin(),v.end(),0);
    std::cout<<sum<<std::endl;
    return 0;
}