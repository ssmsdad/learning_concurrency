


#include <future>
#include<iostream>
#include<algorithm>
#include<list>

/*
    函数式编程（Functional Programming, FP）要避免使用可变数据，函数应该是纯函数，即函数的输出只依赖于输入，不依赖于外部状态。
    使用不可变数据，这意味着一旦数据被创建，就不能被修改。所有的状态变化都通过创建新的数据结构来实现 
    可以看到，这里的sequential_quick_sort函数并没有修改输入的list，而是不断使用splice方法，返回了一个新的list,说白了就是不用共享数据，不用加锁，不用考虑线程安全问题
*/

template<typename T>
std::list<T> sequential_quick_sort(std::list<T> input)
{
    if(input.empty())
    {
        return input;
    }
    std::list<T> result;
    // 将input的第一个元素移到result的第一个位置
    result.splice(result.begin(),input,input.begin());
    // 选取pivot
    T const& pivot=*result.begin();
    // 重排，找到分界点
    auto divide_point=std::partition(input.begin(),input.end(),[&](T const& t){return t<pivot;});
    std::list<T> lower_part;
    // 将input中小于pivot的元素移到lower_part
    lower_part.splice(lower_part.end(),input,input.begin(),divide_point);
    // 使用std::async异步执行lower_part的排序
    std::future<std::list<T>> new_lower(std::async(sequential_quick_sort<T>,std::move(lower_part)));
    auto new_higher(sequential_quick_sort(std::move(input)));
    // 将new_higher中的元素移到result，在pivot之后
    result.splice(result.end(),new_higher);
    // 将new_lower中的元素移到result，在pivot之前
    // 使用get()方法获取异步执行的结果
    result.splice(result.begin(),new_lower.get());
    return result;
}


int main()
{
    std::list<int> l={4,4,7,1,3,9,2,5,8,6};
    l=sequential_quick_sort(l);
    for(auto const& i:l)
    {
        std::cout<<i<<' ';
    }
    std::cout<<std::endl;
    return 0;
}