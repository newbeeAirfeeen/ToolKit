#include "net/asio.hpp"
#include <thread>
char buff[1024];
char buff2[1024];
#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <iostream>
#include <spdlog/logger.hpp>
using namespace std;
void receive(const std::shared_ptr<asio::ip::udp::socket> &sock, char *buf, size_t length) {

    auto end = std::make_shared<asio::ip::udp::endpoint>();
    Info("begin read");
    sock->async_receive_from(asio::buffer(buf, length), *end, [end, buf, length, sock](const std::error_code &e, size_t len) {
        if (e) {
            Error("error: {}", e.message());
            return;
        }

        Info("recv: {}", buf);
        //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        receive(sock, buf, length);
    });
}


//void test2(){
//
//    auto sock = std::make_shared<asio::ip::udp::socket>(context);
//    sock->open(asio::ip::udp::v4());
//    sock->set_option(option);
//    sock->bind(endpoint_);
//
//
//    receive(sock, buff, sizeof(buff));
//    auto sock2 = std::make_shared<asio::ip::udp::socket>(context);
//    sock2->open(asio::ip::udp::v4());
//    sock2->set_option(option);
//    sock2->bind(endpoint_);
//    receive(sock2, buff2, sizeof(buff2));
//}

int create_fd() {
    int fd = -1;
    if ((fd = (int) socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        return -1;
    }


    int opt = 1;
    //int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, static_cast<socklen_t>(sizeof(opt)));
    //if (ret == -1) {
    //    return ret;
    //}
#if defined(SO_REUSEPORT)
    int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (char *) &opt, sizeof opt);
    if (ret == -1) {
        throw std::runtime_error("");
    }
#endif

#if defined(_WIN32)
    unsigned long ul = noblock;
#else
    int ul = 1;
#endif//defined(_WIN32)
    //ret = ioctl(fd, FIONBIO, &ul);//设置为非阻塞模式
    //if (ret == -1) {
    //    return -1;
    //}
    return fd;
}

void asio_() {
    asio::io_context context(1);
    asio::io_context context2(1);
    asio::executor_work_guard<typename asio::io_context::executor_type> guard(context.get_executor());
    asio::executor_work_guard<typename asio::io_context::executor_type> guard2(context2.get_executor());

    asio::ip::udp::socket::reuse_address option(true);
    asio::ip::udp::endpoint endpoint_(asio::ip::udp::v4(), 9000);
    int opt = 1;
    auto sock_1 = std::make_shared<asio::ip::udp::socket>(context);
    sock_1->open(endpoint_.protocol());
    sock_1->set_option(option);
    int ret = setsockopt(sock_1->native_handle(), SOL_SOCKET, SO_REUSEPORT, (char *) &opt, sizeof opt);
    if (ret < 0) {
        throw std::runtime_error("sdf");
    }
    sock_1->bind(endpoint_);

    auto sock_2 = std::make_shared<asio::ip::udp::socket>(context2);
    sock_2->open(endpoint_.protocol());
    sock_2->set_option(option);
    ret = setsockopt(sock_2->native_handle(), SOL_SOCKET, SO_REUSEPORT, (char *) &opt, sizeof opt);
    if (ret < 0) {
        throw std::runtime_error("sdf2");
    }
    sock_2->bind(endpoint_);


    std::thread t([&]() {
        receive(sock_1, buff, sizeof buff);
        context.run();
    });

    receive(sock_2, buff2, sizeof buff2);
    context2.run();
    if (t.joinable()) t.join();
}

void listen_(int fd, const sockaddr *addr) {
    int ret = ::bind(fd, addr, sizeof(*addr));
    if (ret == -1) {
        throw std::bad_function_call();
    }

    char buf[100];
    sockaddr ad;
    socklen_t length = 0;
    Info("begin read..");
    while (true) {
        ret = ::recvfrom(fd, buf, sizeof(buf), 0, &ad, &length);
        if (ret < 0) {
            throw std::runtime_error(strerror(errno));
        }
        if (ret == 0) {
            continue;
        }
        Info("recv: {}", buf);
        break;
    }
}


void test3() {
    auto fd = create_fd();
    auto fd2 = create_fd();
    if (fd == -1 || fd2 == -1) {
        return;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9000);
    int ret = ::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    if (ret != 1) {
        throw std::invalid_argument("");
    }

    std::thread t([&]() {
        listen_(create_fd(), (const sockaddr *) &addr);
    });
    listen_(create_fd(), (const sockaddr *) &addr);

    if (t.joinable()) t.join();
}
int main() {
    logger::initialize("logs/test_srt_logger.log", spdlog::level::trace);


    asio_();
    return 0;
}