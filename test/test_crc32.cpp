//
// Created by 沈昊 on 2022/7/30.
//
#include <iostream>
#include <Util/crc32.hpp>
using namespace std;
int  main(){


    uint32_t value = crc32((const uint8_t*)"s", 1);
    cout << std::showbase << std::hex << value << endl;
    return 0;
}