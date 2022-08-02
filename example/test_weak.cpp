//
// Created by 沈昊 on 2022/6/30.
//
#include <iostream>
#include <memory>
#include <utility>
#include <future>

template<typename In, typename Out>
class multiple_future{
public:
    using in_type = typename std::remove_cv<typename std::remove_reference<In>::type>::type;
    using out_type = typename std::remove_cv<typename std::remove_reference<Out>::type>::type;

    void set(const in_type& in, const std::weak_ptr<std::future<out_type>>& out){

    }
private:
    std::pair<in_type, std::weak_ptr<std::future<out_type>>> _pair;
};

int main(){

    std::weak_ptr<int> self;

    if(!self.lock()){

    }

    return 0;
}