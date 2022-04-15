//
// Created by 沈昊 on 2022/4/8.
//
#include <net/buffer.hpp>
#include <net/buffer_list.hpp>
#include <iostream>
int main(){

    buffer buf;

    buf.put_be<int>(0x11);
    buf.put_be<size_t>(12);
    buf.put_be24(12);
    auto data = buf.data();

    buf.remove(4);
   // auto i = buf.get_be<int>();
    auto s = buf.get_be<size_t>();
    auto _24 = buf.get_be24();


    return 0;
}