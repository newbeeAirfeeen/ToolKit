//
// Created by 沈昊 on 2022/8/14.
//




#include "buffer.hpp"
#include "steady_ticker.hpp"

int main(){

    steady_ticker<std::chrono::steady_clock> clock;

    buffer buff;
    for(int i = 0 ;i < 100 *10000;i++){
        auto f = std::make_shared<buffer>();
    };


    return 0;
}