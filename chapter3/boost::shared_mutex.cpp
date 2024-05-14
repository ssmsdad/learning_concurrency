


#include <iostream>
#include <map>
#include <string>
#include <mutex>
#include <boost/thread/shared_mutex.hpp>
#include<thread>

class dns_entry{
public:
    dns_entry() = default;
    dns_entry(std::string const& ip_address, std::string const& port) : ip_address(ip_address), port(port) {}
    std::string ip_address;
    std::string port;
};

class dns_cache{
    std::map<std::string, dns_entry> entries;
    /*boost::shared_mutex 是一个同步机制，它允许多个线程同时持有读锁（共享锁），但只允许一个线程持有写锁（独占锁）。
    当有线程持有写锁时，其他线程不能持有读锁或写锁；当有线程持有读锁时，其他线程可以获取读锁，但不能获取写锁。*/
    mutable boost::shared_mutex entry_mutex;
public:
    dns_entry find_entry(std::string const& domain) const;
    void update_or_add_entry(std::string const& domain, dns_entry const& dns_details);
};

dns_entry dns_cache::find_entry(std::string const& domain) const{
    // 获取共享锁
    boost::shared_lock<boost::shared_mutex> lck(entry_mutex);
    std::map<std::string, dns_entry>::const_iterator const it = entries.find(domain);
    if(it == entries.end()){
        std::cout << "Entry not found in cache" << std::endl;
    }
    return (it == entries.end()) ? dns_entry() : it->second;
}

void dns_cache::update_or_add_entry(std::string const& domain, dns_entry const& dns_details){
    // 获取独占锁
    std::lock_guard<boost::shared_mutex> lck(entry_mutex);
    entries[domain] = dns_details;
}

int main(){
    // 共享一个dns_cache对象
    dns_cache cache;
    // 在Lambda表达式中使用引用捕获，以便在多个线程中共享dns_cache对象
    std::thread t1([&cache]{
        cache.update_or_add_entry("www.example.com", dns_entry("192.168.167.53", "80"));
    });
    std::thread t2([&cache]{
        dns_entry entry = cache.find_entry("www.example.com");
        std::cout << "IP Address: " << entry.ip_address << std::endl;
        std::cout << "Port: " << entry.port << std::endl;
    });
    std::thread t3([&cache]{
        cache.update_or_add_entry("www.example.com", dns_entry("192.168.167.55", "80"));
    });
    std::thread t4([&cache]{
        dns_entry entry = cache.find_entry("www.example.com");
        std::cout << "IP Address: " << entry.ip_address << std::endl;
        std::cout << "Port: " << entry.port << std::endl;
    });

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    return 0;
}

