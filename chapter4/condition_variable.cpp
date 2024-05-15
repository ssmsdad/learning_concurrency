

#include<mutex>
#include<condition_variable>
#include<queue>
#include<thread>
#include<iostream>


// 只允许在队尾插入元素（push），在队头删除元素（pop）
std::queue<int> data_queue;
std::mutex data_mutex;
std::condition_variable data_cond;

void data_preparation_thread(){
    for(int i=0;i<10;++i){
        // 暂停 100 毫秒
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::lock_guard<std::mutex> lk(data_mutex);
        data_queue.push(i);
        // 通知等待中的线程，唤醒其中一个，并检查条件是否满足
        data_cond.notify_one();
    }
}

void data_processing_thread(){
    while(true){
        std::unique_lock<std::mutex> lk(data_mutex);
        // 如果 data_queue 为空，就对互斥量进行解锁并将此进程置为阻塞或等待状态，不为空就继续执行
        data_cond.wait(lk,[]{return !data_queue.empty();});
        int data=data_queue.front();
        data_queue.pop();
        lk.unlock();
        std::cout<<data<<std::endl;
        if(data==9)
            break;
    }
}

int main(){
    std::thread t1(data_preparation_thread);
    std::thread t2(data_processing_thread);
    t1.join();
    t2.join();
    return 0;
}