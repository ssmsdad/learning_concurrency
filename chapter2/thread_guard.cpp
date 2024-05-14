

#include<iostream>
#include<thread>

class thread_guard{
    std::thread &t;
public:
    explicit thread_guard(std::thread &t_):t(t_){}
    ~thread_guard(){
        if(t.joinable()){
            t.join();
        }
    }
    thread_guard(thread_guard const&)=delete;
    thread_guard& operator=(thread_guard const&)=delete;
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
    func my_func(some_local_state);
    std::thread t(my_func);
    thread_guard g(t);
}

int main(){
    f();    
    // 在函数 f() 函数结束后，thread_guard 类的析构函数将被调用，这将导致线程 t 被 join()，这样就可以保证线程 t 在 main 函数结束前结束，确保得到正确的some_local_state值
    std::cout<<some_local_state<<std::endl;
    return 0;
}
