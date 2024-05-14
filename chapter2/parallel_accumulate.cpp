

#include<iostream>
#include <iterator>
#include<numeric>
#include<thread>
#include<vector>


template<typename Iterator, typename T>
struct accumulate_block{
    void operator()(Iterator first,Iterator last,T &result){
        result=std::accumulate(first,last,result);
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
    
    std::vector<T> results(num_threads);
    std::vector<std::thread> threads(num_threads-1);
    Iterator block_start=first;
    for(unsigned long i=0;i<(num_threads-1);i++){
        Iterator block_end=block_start;
        std::advance(block_end,block_size);
        threads[i]=std::thread(accumulate_block<Iterator,T>(),block_start,block_end,std::ref(results[i]));
        block_start=block_end;
    }
    accumulate_block<Iterator,T>()(block_start,last,results[num_threads-1]);
    for(auto &entry:threads){
        entry.join();
    }
    return std::accumulate(results.begin(),results.end(),init);
}

int main(){
    std::vector<int> v(1000,1);
    int sum=parallel_accumulate(v.begin(),v.end(),0);
    std::cout<<sum<<std::endl;
    return 0;
}