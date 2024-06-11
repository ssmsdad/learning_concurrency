

#include <iostream>
#include <condition_variable>
#include <mutex>
#include <ratio>
#include <thread>
#include <chrono>

// duration(时延)对象，time_point（时间点）对象，二者第二个模板参数都是时间单位
std::chrono::duration<short,std::ratio<60,1>> timeout(5);   // 5分钟,std::ratio前边的单位是秒，后边的单位根据与秒的转换替换为想要的单位
std::chrono::time_point<std::chrono::system_clock,std::chrono::minutes> tp=std::chrono::time_point_cast<std::chrono::minutes>(std::chrono::system_clock::now()+timeout);    // 当前时间+5分钟



std::condition_variable cv;
std::mutex mtx;
bool ready = false;

void wait_thread_until() {
    std::unique_lock<std::mutex> lk(mtx);
    auto now = std::chrono::system_clock::now();
    auto timeout = now + std::chrono::seconds(5);
    // 使用循环来防止虚假唤醒（即使没有其他线程发出通知，它也可能被唤醒。这种现象可能是由于操作系统层面的原因，或者是硬件级别的中断等原因造成的），保证及时唤醒了，也要检查ready的值
    while(!ready){
        // wait_until()方法的参数是一个时间点，如果在这个时间点之前没有被唤醒，就会返回超时状态
        // 返回值是一个 std::cv_status 枚举类型的值,std::cv_status::timeout或std::cv_status::no_timeout
        if (cv.wait_until(lk, timeout) == std::cv_status::timeout) {
            std::cout << "Thread waited for more than 5 seconds." << std::endl;
        } else {
            std::cout << "Thread was notified before the timeout." << std::endl;
        }
    }
}

void wait_thread_for(){
    std::unique_lock<std::mutex> lk(mtx);
    while(!ready){
    // wait_for()方法的参数是一个时间段，如果在这个时间段内没有被唤醒，就会返回超时状态
        if (cv.wait_for(lk, std::chrono::seconds(5)) == std::cv_status::timeout) {
            std::cout << "Thread waited for more than 5 seconds." << std::endl;
        } else {
            std::cout << "Thread was notified before the timeout." << std::endl;
        }
    }
}

int main() {
    std::thread t(wait_thread_for);

    // 主线程做一些工作，然后通知等待的线程
    // 这里我们故意不通知，以测试超时情况
    // std::this_thread::sleep_for(std::chrono::seconds(6));
    std::cout<<"timeout:"<<timeout.count()<<std::endl;
    std::cout<<"tp:"<<tp.time_since_epoch().count()<<std::endl;
    // 等待线程结束，即使线程被阻塞，也会等待线程结束
    t.join();
    return 0;
}



