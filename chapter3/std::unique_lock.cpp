
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
    // 使用std::defer_lock参数，表示互斥量不会被立即锁定，更灵活
    std::unique_lock<std::mutex> lock_a(lhs.m,std::defer_lock); 
    std::unique_lock<std::mutex> lock_b(rhs.m,std::defer_lock); 
    std::lock(lock_a,lock_b); 
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

