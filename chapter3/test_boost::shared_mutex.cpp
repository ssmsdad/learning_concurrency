

#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include <iostream>
#include <thread>

boost::shared_mutex mtx; // 创建一个共享互斥量
int data = 0; // 共享数据

void reader() {
    boost::shared_lock<boost::shared_mutex> lck(mtx); // 获取共享锁
    std::cout << "Reader: " << data << std::endl;
}

void writer() {
    boost::unique_lock<boost::shared_mutex> lck(mtx); // 获取独占锁
    data++;
    std::cout << "Writer updated data to: " << data << std::endl;
}

int main() {
    std::thread t1(reader);
    std::thread t2(writer);
    std::thread t3(reader);

    t1.join();
    t2.join();
    t3.join();

    return 0;
}
