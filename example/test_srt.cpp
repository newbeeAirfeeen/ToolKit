
 /// Created by 沈昊 on 2022/4/26.

#include <protocol/srt/srt.hpp>
#include <event_poller_pool.hpp>
void test_address(){
    const std::string address = "127.0.0.1";
    auto end = asio::ip::address::from_string(address);
    auto ad_array = end.to_v4().to_bytes();
    const unsigned char* data = ad_array.data();
    const uint32_t pp = *((uint32_t*)data);
    int a = 1;
}


void test_connect(){
    using namespace srt;
    auto poller = event_poller_pool::Instance().get_poller(false);
    asio::ip::udp::socket _socket(poller->get_executor());
    auto core = std::make_shared<srt_core>(*poller, _socket, false);
    auto endpoint = asio::ip::address_v4::loopback();
    asio::ip::udp::socket::endpoint_type peer(endpoint, 5000);

    core->async_connect(peer, [](const std::error_code& e){
        Info(e.message());

    });
    event_poller_pool::Instance().wait();
}

int main(){

    logger::initialize("logs/test_logger.log", spdlog::level::trace);


    test_connect();
    return 0;
}

