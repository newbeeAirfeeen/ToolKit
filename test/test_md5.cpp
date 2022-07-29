//
// Created by 沈昊 on 2022/4/29.
//
#include <iostream>
#include <Util/MD5.h>
int main(){
    using namespace std;
    using namespace toolkit;
    MD5 md5("127.0.0.1:56702:0");
    auto str = md5.rawdigest();
    const char*data = str.data();





    return 0;
}