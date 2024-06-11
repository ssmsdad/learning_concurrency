

#include <boost/thread/pthread/shared_mutex.hpp>
#include<iostream>
#include<list>
#include <memory>
#include <utility>
#include <mutex>
#include <boost/thread/shared_mutex.hpp>
#include <vector>
#include<thread>


template<typename Key, typename Value, typename Hash=std::hash<Key>>
class threadsafe_lookup_table
{
private:
    class bucket_type
    {
    private:
        typedef std::pair<Key, Value> bucket_value;
        typedef std::list<bucket_value> bucket_data;
        typedef typename bucket_data::iterator bucket_iterator;

        bucket_data data;
        mutable boost::shared_mutex mutex;

        bucket_iterator find_entry_for(Key const& key)
        {
            return std::find_if(data.begin(), data.end(), [&](bucket_value const& item) { return item.first == key; });
        }
    
    public:
        Value value_for(Key const &key, Value const& default_value)
        {
            boost::shared_lock<boost::shared_mutex> lock(mutex);
            bucket_iterator const found_entry=find_entry_for(key);
            return found_entry == data.end() ? default_value : found_entry->second;
        }
        void add_or_update_mapping(Key const& key, Value const& value)
        {
            std::unique_lock<boost::shared_mutex> lock(mutex);
            /*
            iterator const：
            这是一个常量迭代器，意味着你不能改变这个迭代器本身（即你不能使它指向其他元素），但是你可以通过这个迭代器来修改它所指向的元素（假设元素本身不是常量），类似于常量指针，即int *const p，你不能通过p++来使p指向其他值，但是你可以通过*p=5来修改p所指向的值
            const iterator：
            这是一个迭代器常量，意味着你不能通过这个迭代器来修改它所指向的元素，但是你可以使这个迭代器指向其他元素，类似于指向常量的指针，即const int *p，你不能通过p来修改p所指向的值，但是你可以通过p++来使p指向其他值
            */
            bucket_iterator const found_entry = find_entry_for(key);
            if(found_entry == data.end())
            {
                data.push_back(bucket_value(key, value));
            }
            else
            {
                found_entry->second = value;
            }
        }
        void remove_mapping(Key const& key)
        {
            std::unique_lock<boost::shared_mutex> lock(mutex);
            bucket_iterator const found_entry = find_entry_for(key);
            if(found_entry != data.end())
            {
                data.erase(found_entry);
            }
        }
    };

    std::vector<std::unique_ptr<bucket_type>> buckets;
    Hash hasher;

    bucket_type& get_bucket(Key const& key) const
    {
        std::size_t const bucket_index = hasher(key) % buckets.size();
        return *buckets[bucket_index];
    }

public:
    threadsafe_lookup_table(unsigned num_buckets = 19, Hash const& hasher_ = Hash()) : buckets(num_buckets), hasher(hasher_)
    {
        for(unsigned i = 0; i < num_buckets; ++i)
        {
            // 释放指针指向的对象，并将指针置为nullptr
            buckets[i].reset(new bucket_type);
        }
    }

    threadsafe_lookup_table(threadsafe_lookup_table const& other) = delete;
    threadsafe_lookup_table& operator=(threadsafe_lookup_table const& other) = delete;

    Value value_for(Key const& key, Value const& default_value = Value()) const
    {
        return get_bucket(key).value_for(key, default_value);
    }

    void add_or_update_mapping(Key const& key, Value const& value)
    {
        get_bucket(key).add_or_update_mapping(key, value);
    }

    void remove_mapping(Key const& key)
    {
        get_bucket(key).remove_mapping(key);
    }
};

int main(){
    std::thread t1([](){
        threadsafe_lookup_table<int, std::string> lookup_table;
        lookup_table.add_or_update_mapping(1, "one");
        lookup_table.add_or_update_mapping(2, "two");
        lookup_table.add_or_update_mapping(3, "three");
        lookup_table.add_or_update_mapping(4, "four");
        lookup_table.add_or_update_mapping(5, "five");

        std::cout << "Value for key 1: " << lookup_table.value_for(1) << std::endl;
        std::cout << "Value for key 2: " << lookup_table.value_for(2) << std::endl;
        std::cout << "Value for key 3: " << lookup_table.value_for(3) << std::endl;
        std::cout << "Value for key 4: " << lookup_table.value_for(4) << std::endl;
        std::cout << "Value for key 5: " << lookup_table.value_for(5) << std::endl;
    });

    std::thread t2([](){
        threadsafe_lookup_table<int, std::string> lookup_table;
        lookup_table.remove_mapping(1);
        lookup_table.remove_mapping(2);
        lookup_table.remove_mapping(3);
        lookup_table.remove_mapping(4);
        lookup_table.remove_mapping(5);

        std::cout << "Value for key 1: " << lookup_table.value_for(1) << std::endl;
        std::cout << "Value for key 2: " << lookup_table.value_for(2) << std::endl;
        std::cout << "Value for key 3: " << lookup_table.value_for(3) << std::endl;
        std::cout << "Value for key 4: " << lookup_table.value_for(4) << std::endl;
        std::cout << "Value for key 5: " << lookup_table.value_for(5) << std::endl;
    });

    t1.join();
    t2.join();
    return 0;
}