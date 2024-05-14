

#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

class Singleton {
public:
    std::once_flag onceFlag;
    static Singleton& getInstance() {
        static Singleton instance;
        return instance;
    }

    void print() {
        std::cout << "Singleton printed" << std::endl;
    }

private:
    Singleton() {} // 私有构造函数
    Singleton(const Singleton&); // 阻止复制构造
    Singleton& operator=(const Singleton&); // 阻止赋值操作
};

void initializeSingleton() {
    // std::call_once 确保其中的代码块在整个应用程序的生命周期中只执行一次。
    std::call_once(Singleton::getInstance().onceFlag, &Singleton::getInstance);
}


int main() {
    std::thread t1(initializeSingleton);
    std::thread t2(initializeSingleton);
    std::thread t3(initializeSingleton);

    t1.join();
    t2.join();
    t3.join();

    // 虽然调用了三次initializeSingleton函数，但是只有第一次调用会创建Singleton实例
    Singleton::getInstance().print(); // 输出 "Singleton printed"

    return 0;
}