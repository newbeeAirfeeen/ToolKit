//
// Created by 沈昊 on 2022/4/3.
//
#include <spdlog/logger.hpp>
#include <iostream>
#include <functional>
#include <type_traits>
using namespace std;

int main(){

    logger::initialize("logs/test_logger.log", spdlog::level::info);
    Info("Info ,{}, {}, {}.{}", 1, "name", 1.1, 2.2);

    Warn("warning");
    Error("error");
    Critical("critical");




    return 0;
}