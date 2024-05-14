

#include<memory>
#include<mutex>
#include<stack>
#include<exception>

struct empty_stack: std::exception{
    // 重载了std::exception的what()函数，结尾的const throw()表示这个函数不会抛出异常
    const char* what() const throw(){
        return "empty stack!";
    };
};

template<typename T>
class threadsafe_stack{
private:
    std::stack<T> data;
    mutable std::mutex m;
public:
    threadsafe_stack(){}
    threadsafe_stack(const threadsafe_stack& other){
        // 注意这里并没有使用成员初始化列表，因为这样可以确保复制结果的正确性
        std::lock_guard<std::mutex> lock(other.m);
        data = other.data;
    }
    threadsafe_stack& operator=(const threadsafe_stack&) = delete;
    void push(T new_value){
        std::lock_guard<std::mutex> lock(m);
        data.push(new_value);
    }
    std::shared_ptr<T> pop(){
        std::lock_guard<std::mutex> lock(m);
        if(data.empty()) throw empty_stack();
        std::shared_ptr<T> const res(std::make_shared<T>(data.top()));
        data.pop();
        return res;
    }
    void pop(T& value){
        std::lock_guard<std::mutex> lock(m);
        if(data.empty()) throw empty_stack();
        value = data.top();
        data.pop();
    }
    bool empty() const{
        std::lock_guard<std::mutex> lock(m);
        return data.empty();
    }
};