

#include <deque>
#include<iostream>
#include<future>
#include<thread>
#include<chrono>





std::mutex m;
std::deque<std::packaged_task<void()>> tasks;

bool gui_shutdown_message_received()
{
    return false;
}

// 从队列中取出一个任务并执行
void get_and_process_gui_message()
{
    std::packaged_task<void()> task;
    // 这里使用{}是因为我们需要在获取任务后立即释放锁，在task执行过程中不再持有锁
    {
        std::lock_guard<std::mutex> lk(m);
        if(tasks.empty())
            return;
        task=std::move(tasks.front());
        tasks.pop_front();
    }
    task();
}

// GUI线程
void gui_thread()
{
    while(!gui_shutdown_message_received())
    {
        get_and_process_gui_message();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// 将任务包装成 std::packaged_task，并将其推入队列
template<typename Func>
std::future<void> post_task_for_gui_thread(Func f)
{
    std::packaged_task<void()> task(f);
    std::future<void> res=task.get_future();
    std::lock_guard<std::mutex> lk(m);
    tasks.push_back(std::move(task));
    return res;
}

int main()
{
    // auto p1 = std::async(std::launch::async,gui_thread);    异步执行gui_thread
    std::thread gui_bg_thread(gui_thread);      //同步执行gui_thread
    std::this_thread::sleep_for(std::chrono::seconds(1));   
    post_task_for_gui_thread([](){std::cout<<"hello world"<<std::endl;});
    std::this_thread::sleep_for(std::chrono::seconds(1));
    post_task_for_gui_thread([](){std::cout<<"hello world2"<<std::endl;});
    std::this_thread::sleep_for(std::chrono::seconds(1));
    post_task_for_gui_thread([](){std::cout<<"hello world3"<<std::endl;});
    std::this_thread::sleep_for(std::chrono::seconds(1));
    post_task_for_gui_thread([](){std::cout<<"hello world4"<<std::endl;});
    std::this_thread::sleep_for(std::chrono::seconds(1));
    post_task_for_gui_thread([](){std::cout<<"hello world5"<<std::endl;});
    std::this_thread::sleep_for(std::chrono::seconds(1));
    post_task_for_gui_thread([](){std::cout<<"hello world6"<<std::endl;});
    std::this_thread::sleep_for(std::chrono::seconds(1));
    post_task_for_gui_thread([](){std::cout<<"hello world7"<<std::endl;});
    std::this_thread::sleep_for(std::chrono::seconds(1));
    post_task_for_gui_thread([](){std::cout<<"hello world8"<<std::endl;});
    std::this_thread::sleep_for(std::chrono::seconds(1));
    post_task_for_gui_thread([](){std::cout<<"hello world9"<<std::endl;});
    std::this_thread::sleep_for(std::chrono::seconds(1));
    post_task_for_gui_thread([](){std::cout<<"hello world10"<<std::endl;});
    std::this_thread::sleep_for(std::chrono::seconds(1));
    gui_bg_thread.join();
    return 0;
}
