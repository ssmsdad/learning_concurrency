

#include <future>
#include <iostream>
#include <thread>



void set_promise(std::promise<int>& p,int value) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    p.set_value(value);
}

/*
    总结：
        future仅支持在很多线程在等待的时候，只有一个线程能获取等待结果。当多个线程需要等待相同的事件的结果，你就需要使用 std::shared_future 来替代 std::future 了,例子见shared_future_test.cpp
        future对象调用share()方法变为shared_future对象，shared_future对象可以被多次get，并且支持拷贝其对象的实例
*/
int main() {

    std::promise<int> p1;
    std::future<int> f1 = p1.get_future();
    // std::shared_future 可以被多个线程共享，而 std::future 不能。使用share()方法可以将 std::future 转换为 std::shared_future
    std::shared_future<int> sf1 = f1.share();
    // 每个 std::promise 对象只 set_value 一次
    std::thread t1(set_promise, std::ref(p1), 42);

    // shared_future 对象可以拷贝，而 future 对象不能
    std::shared_future<int> sf1_copy = sf1;
    std::cout << "Value: " << sf1_copy.get() << std::endl;

    std::cout << "Waiting for value to be set..." << std::endl;
    std::cout << "Value: " << sf1.get() << std::endl;
    // shared_future对象可以被多次get
    std::cout << "Value again: " << sf1.get() << std::endl;

    t1.join();

    return 0;
}
