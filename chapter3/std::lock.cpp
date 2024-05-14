
#include<iostream>
#include<mutex>

class some_big_object{
    friend void swap(some_big_object& lhs,some_big_object& rhs);
    private:
        int a;
    public:
        some_big_object(int a):a(a){
            std::cout<<"some_big_object constructor called"<<std::endl;
        }
};
void swap(some_big_object& lhs,some_big_object& rhs){
    std::cout<<"swap called"<<std::endl;
    std::cout<<lhs.a<<" "<<rhs.a<<std::endl;
    int temp=lhs.a;
    lhs.a=rhs.a;
    rhs.a=temp;
    std::cout<<lhs.a<<" "<<rhs.a<<std::endl;
    std::cout<<"swap done"<<std::endl;
}

class X
{
    friend void swap(X& lhs, X& rhs);
    private:
        some_big_object some_detail;
        std::mutex m;
    public:
        X(some_big_object const& sd):some_detail(sd){}

};

void swap(X& lhs, X& rhs){
    if(&lhs==&rhs)
        return;
    // std::lock使用了一种特殊的锁定顺序，可以避免死锁
    std::lock(lhs.m,rhs.m); // 1
    // std::adopt_lock告诉 std::lock_guard，传入的互斥量已经被锁定。这意味着 std::lock_guard 不会尝试再次锁定互斥量，而是“接收”已经被锁定的互斥量
    std::lock_guard<std::mutex> lock_a(lhs.m,std::adopt_lock); // 2
    std::lock_guard<std::mutex> lock_b(rhs.m,std::adopt_lock); // 3
    swap(lhs.some_detail,rhs.some_detail);
}

int main(){
    some_big_object a(1);
    some_big_object b(2);
    X x(a);
    X y(b);
    swap(x,y);
    return 0;
}

