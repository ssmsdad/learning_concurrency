

#include<iostream>
#include <iterator>
#include<vector>
#include<thread>
#include<future>


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

template<typename Iterator, typename MatchType>
Iterator parallel_find(Iterator first,Iterator last,MatchType match){
    struct find_element{
        void operator()(Iterator begin,Iterator end,MatchType match,
        std::promise<Iterator>* result,std::atomic<bool>* done_flag){
            try{
                for(;(begin!=end) && !done_flag->load();++begin){
                    if(*begin==match){
                        result->set_value(begin);
                        done_flag->store(true);
                        return;
                    }
                }
            }catch(...){
                try{
                    result->set_exception(std::current_exception());
                    done_flag->store(true);
                }catch(...){}
            }
        }
    };
    unsigned long const length=std::distance(first,last);
    if(!length)
        return last;
    unsigned long const min_per_thread=25;
    unsigned long const max_threads=(length+min_per_thread-1)/min_per_thread;
    unsigned long const hardware_threads=std::thread::hardware_concurrency();
    unsigned long const num_threads=std::min(hardware_threads!=0?hardware_threads:2,max_threads);
    unsigned long const block_size=length/num_threads;
    
    std::promise<Iterator> result;
    std::atomic<bool> done_flag(false);
    std::vector<std::thread> threads(num_threads-1);
    // 这里使用一个{}来创建一个局部作用域，这样可以确保最终在检查done_flag位来确定是否找到对应元素时，所有线程都已经完成
    {
        join_threads joiner(threads);
        Iterator block_start=first;
        for(unsigned long i=0;i<(num_threads-1);++i){
            Iterator block_end=block_start;
            std::advance(block_end,block_size);
            threads[i]=std::thread(find_element(),block_start,block_end,match,&result,&done_flag);
            block_start=block_end;
        }
        find_element()(block_start,last,match,&result,&done_flag);
    }
    if(!done_flag.load()){
        return last;
    }
    return result.get_future().get();
}


int main(){
    std::vector<int> v(1000000,1);
    v[500000]=2;
    auto result=parallel_find(v.begin(),v.end(),2);
    if(result==v.end()){
        std::cout<<"not found"<<std::endl;
    }else{
        std::cout<<"found"<<std::endl;
    }
    return 0;
}