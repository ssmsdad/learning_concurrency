

// future仅支持在很多线程在等待的时候，只有一个线程能获取等待结果。当多个线程需要等待相同的事件的结果，你就需要使用 std::shared_future 来替代 std::future 了

#include <future>
#include <iostream>
#include <thread>



int return_int(){
    return 42;
}

int main(){
    // auto f = std::async(std::launch::async, return_int);
    // std::shared_future<int> sf = f.share();
    std::shared_future<int> sf2 = std::async(std::launch::async, return_int).share();

    std::thread t1([sf2](){
        std::cout << "Thread 1: " << sf2.get() << std::endl;
    });

    std::thread t2([sf2](){
        std::cout << "Thread 2: " << sf2.get() << std::endl;
    });

    t1.join();
    t2.join();
    return 0;
}