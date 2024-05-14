

#include<iostream>
#include<thread>

class scoped_thread{
    std::thread t;
public:
    explicit scoped_thread(std::thread t_):t(std::move(t_)){
        if(!t.joinable())
            throw std::logic_error("No thread");
    }
    ~scoped_thread(){
        t.join();
    }
    scoped_thread(scoped_thread const&)=delete;
    scoped_thread& operator=(scoped_thread const&)=delete;
};

struct func{
    int &i;
    func(int &i_):i(i_){}
    void operator()(){
        for(unsigned j=0;j<10000;j++){
            i+=1;
        }
    }
};

int some_local_state=0;

void f(){
    // 这里如果写成scoped_thread t(std::thread(func(some_local_state)),编译器会发出警告，它被解析为一个函数声明，而不是一个对象的构造
    scoped_thread t{std::thread(func(some_local_state))};
}

int main(){
    f();
    std::cout<<some_local_state<<std::endl;
    return 0;
}