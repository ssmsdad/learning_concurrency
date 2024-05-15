

#include<iostream>
#include<algorithm>
#include<mutex>
#include<list>

std::list<int> some_list;
std::mutex some_mutex;


void add_to_list(int new_value){
    std::lock_guard<std::mutex> guard(some_mutex);
    some_list.push_back(new_value);
}

bool find_in_list(int value){
    std::lock_guard<std::mutex> guard(some_mutex);
    return std::find(some_list.begin(),some_list.end(),value)!=some_list.end();
}

int main(){
    add_to_list(42);
    std::cout<<"contains(1)="<<find_in_list(1)<<", contains(42)="<<find_in_list(42)<<std::endl;
    return 0;
}

class Y
{
private:
    int some_detail;
    mutable std::mutex m;
    int get_detail() const{
        // 防的是不同线程中的相同的 Y 对象调用 get_detail() 函数，这两个线程会互斥地访问 some_detail，而同一线程中的不同 Y 对象可以并发地访问 some_detail
        std::lock_guard<std::mutex> lock_a(m); // 1
        return some_detail;
    }
public:
    Y(int sd):some_detail(sd){}
friend bool operator==(Y const& lhs, Y const& rhs)
{
    // std::mutex 是不可递归的，这意味着如果一个线程试图获取它已经持有的互斥锁，那么这个行为是未定义的，通常会导致死锁。
    if(&lhs==&rhs)
        return true;
    // 每个函数调用都会获取各自对象的互斥锁，因为这两个互斥锁是不同的（除非 lhs 和 rhs 是同一个对象），所以这两个函数调用可以并发执行，不会互斥。
    int const lhs_value=lhs.get_detail(); // 2
    int const rhs_value=rhs.get_detail(); // 3
    return lhs_value==rhs_value; // 4
}
};