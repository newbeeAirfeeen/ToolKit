//
// Created by 沈昊 on 2022/4/3.
//
#include <fmt/fmt.h>
#include <fmt/bundled/color.h>
#include <iostream>

using  namespace std;

/*
 *
 * https://fmt.dev/latest/syntax.html
 *
 * format string syntax:
 *  surrounded by curly braces {}.
 *  if you need to include brace character in the
 *  literal text, it can be escaped doubling {{}}
 *
 *  replacement_field = "{  [arg_id] [":" (format_spec | chrono_format_spec)] }"
 *  arg_id = digit+
 *
 * if format_spec or chorno_format_spec is provided, must have ":"
 *
 * format_spec [[fill]align][sign]["#"]["0"]["width"]["." precision]["L"][type]
 * fill = a character other than "{", "}"
 * align =
 *      "<" : left-aligned
 *      ">" : right-aligned
 *      "^" : centered within the available space
 *      format_spec ::=  [[fill]align][sign]["#"]["0"][width]["." precision]["L"][type]
 *  fill        ::=  <a character other than '{' or '}'>
 *  align       ::=  "<" | ">" | "^"
 *  sign        ::=  "+" | "-" | " "
 *  width       ::=  integer | "{" [arg_id] "}"
 *  precision   ::=  integer | "{" [arg_id] "}"
 *  type        ::=  "a" | "A" | "b" | "B" | "c" | "d" | "e" | "E" | "f" | "F" | "g" | "G" |
                 "o" | "p" | "s" | "x" | "X"
 * */


void basic_use(){
    std::cout << fmt::format("Hell,world") << std::endl;
    std::string s = fmt::format("The answer is {}.", 42);
    std::cout << s << std::endl;
}

void avoid_memory_alloc(){
    //这里可以避免构造string

    auto out = fmt::memory_buffer();
    fmt::format_to(std::back_inserter(out), "For a moment, {} happend", "nothing");

    fmt::print(out.data());
}

void pass_named_arg(){

    fmt::print("I'd rather be {1} than {0}.", "right", "happy\n");
    fmt::print("Hello, {name}! The answer is {number}. Goodbye, {name}.\n",
               fmt::arg("name", "World"), fmt::arg("number", 42));
}


void format_spec(){
    //保证输出正确的情况下向右shift
    cout << fmt::format("{0:->10}\n", "", "hello,world");
    cout << fmt::format("{0:-^100}\n{1: ^+100.4}\n{0:-^100}\n", "", 1.098765);
                        //"{0:^{100}}\n"
                        //"{0:-^{100}}\n", "hello,world");
    cout << fmt::format("{0:#b}\n", 15);
    cout << fmt::format("{0:#B}\n", 15);
    cout << fmt::format("{0:b}\n", 15);
    cout << fmt::format("{0:o}\n", 15);
    cout << fmt::format("{0:#x}\n", 15);
    cout << fmt::format("{0:X}\n", 15);
    cout << fmt::format("{0:a}\n", 15.1);
    cout << fmt::format("{0:e}\n", 15.1);
}


void test_color(){
//    fmt::format()


}
int main(){

    basic_use();
    avoid_memory_alloc();
    pass_named_arg();
    format_spec();
    return 0;
}