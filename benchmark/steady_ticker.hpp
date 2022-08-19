//
// Created by 沈昊 on 2022/8/14.
//

#ifndef TOOLKIT_STEADY_TICKER_HPP
#define TOOLKIT_STEADY_TICKER_HPP


#include <chrono>
#include <iostream>
using namespace std;
template<typename clock_type>
class steady_ticker{
public:
    steady_ticker(){
        begin = clock_type::now();
    }


    ~steady_ticker(){
        end = clock_type::now();
        auto counts = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
        cout << "ticker: " << ((double)counts / 1000 / 1000 * 1.0) << "s" << endl;
    }
private:
    typename clock_type::time_point begin;
    typename clock_type::time_point end;
};


#endif//TOOLKIT_STEADY_TICKER_HPP
