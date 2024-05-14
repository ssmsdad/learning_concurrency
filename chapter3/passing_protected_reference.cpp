

#include<mutex>

class some_data{
    friend void foo();
private:
    int a;
    std::string b;
};

class data_wrapper{
private:
    some_data data;
    std::mutex m;
public:
    template<typename Function>
    void process_data(Function func){
        std::lock_guard<std::mutex> l(m);
        func(data);
    }
};

some_data* unprotected;

void malicious_function(some_data& protected_data){
    // 这里会有问题，因为把一个protected_data的引用传递给了unprotected，unprotected在互斥锁作用域之外
    // 所以注意不要将受保护数据的引用或指针传递到互斥锁作用域之外
    unprotected = &protected_data;
}

void foo(){
    data_wrapper x;
    x.process_data(malicious_function);
    unprotected->a = 42;
}

int main(){
    foo();
    return 0;
}