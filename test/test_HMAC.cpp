//
// Created by 沈昊 on 2022/8/1.
//
#include <wolfssl/openssl/hmac.h>
#include <wolfssl/openssl/evp.h>
#include <iostream>
#include <iterator>
using namespace std;
int main(){
    const char* key1 = "1234567";
    char buf[20] = {0};
    unsigned int length = 20;
    const char* data = "shenhao";
    HMAC_CTX* ctx = new HMAC_CTX;
    HMAC_CTX_init(ctx);
    HMAC_Init_ex(ctx, (const void*)key1, (int)strlen(key1), EVP_sha1(), nullptr);
    HMAC_Update(ctx, (const unsigned char*)data, strlen(data));
    auto ret = HMAC_Final(ctx, (unsigned char*)buf, &length);
    HMAC_CTX_cleanup(ctx);
    if( ret <= 0){
        return -1;
    }

    cout << showbase;
    for(int i = 0 ;i < 20;i++)
        cout << hex << buf[i];


    return 0;
}