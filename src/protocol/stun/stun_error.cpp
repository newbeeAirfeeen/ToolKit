//
// Created by OAHO on 2022/7/30.
//
#include "stun_error.h"


namespace stun{
    const char* category::name() const noexcept{
        return "stun error";
    }
    /// page 37
    std::string category::message(int err) const{
        switch(err){
                /// the client should contact an alternate server for this
                /// request. This error response MUST only be sent if the request
                /// included a USERNAME attribute and a valid MESSAGE-INTEGRITY attribute
            case 300: return "Try Alternate";
                /// the server may not be able to generate a valid MESSAGE-INTEGRITY for this error.
            case 400: return "Bad Request";
                /// The request did not contain the correct credentials to proceed.
                /// The client should retry the request with proper credentials
            case 401: return "Unauthorized";
                /// The server received a STUN packet containing a comprehension-required
                /// attribute that it did not understand.
                /// the server must put this unknown attribute in the UNKNOWN-ATTRIBUTE of
                /// its error response
            case 420: return "Unknown Attribute";
                /// the NONCE used by the client was no longer valid. the client should retry
                /// using the NONCE provided in the response.
            case 438: return "Stale Nonce";
                /// The server has suffered a temporary error.the client should  try again.
            case 500: return "Server Error";
            default:{
                return "success";
            }
        }
    }

    category* generate_category(){
        static category category_;
        return &category_;
    }
}