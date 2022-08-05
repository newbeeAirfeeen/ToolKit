//
// Created by 沈昊 on 2022/8/3.
//
#include "stun_method.h"

namespace stun {
    bool is_stun_method(uint16_t value) {
        switch (value) {
            case binding_request:
            case shared_secret:
            case binding_response:
                return true;
            default:
                return false;
        }
        return false;
    }
};// namespace stun