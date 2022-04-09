/*
* @file_name: session_helper.cpp
* @date: 2022/04/04
* @author: oaho
* Copyright @ hz oaho, All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/
#include "session_helper.hpp"
#include <atomic>
static thread_local std::atomic<size_t> identify{0};
session_helper& get_session_helper(){
    static thread_local session_helper helper;
    return helper;
}

bool session_helper::create_session(const basic_session::pointer& session_pointer){
    if(!session_pointer){
        return false;
    }
    auto identify_index = identify.fetch_add(1);
    auto it = session_map.find(identify_index);
    if(it != session_map.end()){
        return false;
    }
    session_pointer->identify = identify_index;
    session_map.emplace(identify_index, session_pointer);
    return true;
}
bool session_helper::remove_session(const basic_session::pointer& session_pointer){
    if(!session_pointer){
        return true;
    }
    auto it = session_map.find(session_pointer->identify);
    if( it == session_map.end()){
        return true;
    }
    session_map.erase(it);
    return true;
}

bool session_helper::clear(){
    session_map.clear();
    return true;
}