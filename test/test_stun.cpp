//
// Created by 沈昊 on 2022/7/29.
//
#include "Util/MD5.h"
#include "protocol/stun/stun.h"
using namespace std;

int SASLprep(uint8_t *s) {
    if (!s) {
        return -1;
    }
    uint8_t *strin = s;
    uint8_t *strout = s;
    for (;;) {
        uint8_t c = *strin;
        if (!c) {
            *strout = 0;
            break;
        }

        switch (c) {
            case 0xAD:
                ++strin;
                break;
            case 0xA0:
            case 0x20:
                *strout = 0x20;
                ++strout;
                ++strin;
                break;
            case 0x7F:
                return -1;
            default:
                if (c < 0x1F)
                    return -1;
                if (c >= 0x80 && c <= 0x9F)
                    return -1;
                *strout = c;
                ++strout;
                ++strin;
        };
    }

    return 0;
}

int main() {

    const std::string& buf = "pass";
    SASLprep((uint8_t *)buf.data());
    cout << buf << endl;
    std::string hash = "user:realm:pass";
    toolkit::MD5 Md5(hash);
    cout << std::showbase << std::hex << Md5.hexdigest() << endl;
    return 0;
}