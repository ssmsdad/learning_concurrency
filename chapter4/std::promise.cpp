

#include<iostream>
#include<future>
#include<thread>




void print_thread(std::promise<int>& prom, int thread_id) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    prom.set_value(thread_id); // 设置 promise 的值
}


/*
    使用std::packaged_task与std::promise的区别：
        使用 std::promise 当你只需要设置一个值或异常，而不需要一个可调用的任务。
        使用 std::packaged_task 当你需要异步执行一个任务，并且需要获取这个任务的返回值或异常。
*/

int main() {
    std::promise<int> prom;
    // 注意：std::promise 对象只能与一个 std::future 对象相关联，如果你需要多个线程等待同一个任务的完成，你可以使用 std::shared_future。
    std::future<int> fut = prom.get_future(); // 获取与 promise 关联的 future
    // std::future<int> fut2 = prom.get_future();  错的，一个promise只能与一个future关联

    std::thread t1(print_thread, std::ref(prom), 1);

    std::cout << "Waiting for thread to set value..." << std::endl;
    // std::future::get 只能被调用一次。当你第一次调用 get 时，std::future 对象的内部状态被移动（或者说被 "获取"），这意味着 std::future 对象不再有关联的状态。
    int value = fut.get(); // 等待 future 的值被设置
    // int value2 = fut.get();  错的，一个future只能被get一次
    std::cout << "Value: " << value << std::endl;

    t1.join();

    return 0;
}

