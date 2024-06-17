

#include <algorithm>
#include<iostream>
#include <iterator>
#include<vector>
#include<thread>
#include<future>
#include<numeric>
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

// 为了将算法最大程度的并行，需要首先对最后一个元素进行更新，这样你就能将这个值传递给下一个数据块
template<typename Iterator>
void parallel_partial_sum(Iterator first,Iterator last){
    typedef typename Iterator::value_type value_type;
    struct process_element{
        void operator()(Iterator first,Iterator last,
        std::future<value_type>* previous_end_value, std::promise<value_type>* end_value){
            try{
                Iterator end=last;
                ++end;
                std::partial_sum(first,end,first);
                if(previous_end_value){
                    value_type addend=previous_end_value->get();
                    // 更新块结尾的值
                    *last+=addend;
                    if(end_value)
                    {
                        end_value->set_value(*last);
                    }
                    // 更新块中的其他值
                    std::for_each(first,end,[addend](value_type& item){item+=addend;});
                }
                // 第一个块的处理
                else if(end_value){
                    end_value->set_value(*last);
                }
            }catch(...){
                if(end_value){
                    end_value->set_exception(std::current_exception());
                }else{
                    throw;
                }
            }
        }
    };
    unsigned long const length=std::distance(first,last);
    if(!length)
        return;
    unsigned long const min_per_thread=25;
    unsigned long const max_threads=(length+min_per_thread-1)/min_per_thread;
    unsigned long const hardware_threads=std::thread::hardware_concurrency();
    unsigned long const num_threads=std::min(hardware_threads!=0?hardware_threads:2,max_threads);
    unsigned long const block_size=length/num_threads;
    
    std::vector<std::thread> threads(num_threads-1);
    std::vector<std::promise<value_type>> end_values(num_threads-1);
    std::vector<std::future<value_type>> previous_end_values;
    previous_end_values.reserve(num_threads-1);
    join_threads joiner(threads);
    Iterator block_start=first;
    for(unsigned long i=0;i<(num_threads-1);i++){
        Iterator block_end=block_start;
        // 每个块中的其他元素和尾元素是分开处理的，因此需要将块的结尾向前移动一个位置
        std::advance(block_end,block_size-1);
        threads[i]=std::thread(process_element(),block_start,block_end,
        (i==0)?nullptr:&previous_end_values[i-1],&end_values[i]);
        block_start=block_end;
        ++block_start;
        // 保存每个块的结尾值
        previous_end_values.push_back(end_values[i].get_future());
    }
    Iterator final_element=block_start;
    std::advance(final_element,std::distance(block_start,last)-1);
    process_element()(block_start,last,(num_threads>1)?&previous_end_values.back():nullptr,nullptr);
} 


int main(){
    std::vector<int> v(1000);
    // std::iota()函数生成一个序列，从1开始，递增1，直到v.size()结束
    std::iota(v.begin(),v.end(),1);
    parallel_partial_sum(v.begin(),v.end());
    std::copy(v.begin(),v.end(),std::ostream_iterator<int>(std::cout," "));
    std::cout<<std::endl;
    return 0;
}