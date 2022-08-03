//
// Created by OAHO on 2022/7/30.
//
#include "stun_error.h"
#include "stun_error_code.h"
#include "Util/endian.hpp"
#include "stun_attributes.h"
#include "stun_packet.h"
namespace stun {

    void put_error_code(const stun_error_code& err, const std::shared_ptr<buffer> &buf) {
        constexpr uint32_t fixed_header = 4;
        /// 0                  1                   2                   3
        /// 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        /// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        /// | Reserved, should be 0                 |Class| Number |
        /// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        /// | Reason Phrase (variable) ..
        /// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

        ///  Reserved() + Class(3bit) + Number(8bit)
        buffer b;
        uint32_t cls = err / 100;
        b.put_be24(cls);

        category c;
        attribute_type attr;
        attr.attribute = stun::error_code;
        /// reserved + Class
        attr.value.append(b.begin(), b.end());
        /// The Number represents the error code modulo 100, and its
        /// value MUST be between 0 and 99.
        /// Number
        attr.value.append(1, static_cast<char>(err % 100));
        /// reason phrase
        attr.value += c.message(err);
        attr.length = attr.value.size();
        stun_add_attribute(buf, attr);
    }

    const char *stun_packet_category::name() const noexcept {
        return "stun_packet_category";
    }


    std::string stun_packet_category::message(int err) const {
        switch (err) {
            case stun::is_not_stun_packet:
                return "current packet is not stun packet";
            case stun::error_code_is_invalid:
                return "error code is not range of 300-699";
            case stun::attribute_is_invalid:
                return "attribute is invalid for current context";
        }

        return "unknown error";
    }

    const char *category::name() const noexcept {
        return "stun error";
    }
    /// page 37
    std::string category::message(int err) const {
        switch (err) {
                /// the client should contact an alternate server for this
                /// request. This error response MUST only be sent if the request
                /// included a USERNAME attribute and a valid MESSAGE-INTEGRITY attribute
            case stun_error_code::try_alternate:
                return "Try Alternate";
                /// the server may not be able to generate a valid MESSAGE-INTEGRITY for this error.
            case stun_error_code::bad_request:
                return "Bad Request";
                /// The request did not contain the correct credentials to proceed.
                /// The client should retry the request with proper credentials
            case stun_error_code::unauthorized:
                return "Unauthorized";
                /// The server received a STUN packet containing a comprehension-required
                /// attribute that it did not understand.
                /// the server must put this unknown attribute in the UNKNOWN-ATTRIBUTE of
                /// its error response
            case stun_error_code::unknown_attribute:
                return "Unknown Attribute";
                /// the NONCE used by the client was no longer valid. the client should retry
                /// using the NONCE provided in the response.
            case stun_error_code::stale_nonce:
                return "Stale Nonce";
                /// The server has suffered a temporary error.the client should  try again.
            case stun_error_code::server_error:
                return "Server Error";
            default: {
                return "unknown error";
            }
        }
    }

    std::error_category *generate_stun_packet_category() {
        static stun_packet_category category_;
        return &category_;
    }

    std::error_category *generate_category() {
        static category category_;
        return &category_;
    }

    std::error_code make_stun_error(int err, const std::error_category *_category){
        std::error_code e(err, *_category);
        return e;
    }
}// namespace stun