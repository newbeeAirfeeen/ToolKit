#ifdef SSL_ENABLE
#define ASIO_STANDALONE 1
#   define ASIO_HEADER_ONLY 1
#include <asio/ssl.hpp>
#include <asio/ip/udp.hpp>
#include <asio.hpp>
#include <array>
#include <iostream>

/* Certificate validation works exactly the same as with SSL Streams.
 */
bool callback(bool preverified, asio::ssl::verify_context &ctx)
{
    (void) preverified;
    (void) ctx;

    return 1;
}

int main()
{
    asio::io_context context;
    asio::ssl::dtls::context ctx(asio::ssl::dtls::context::dtls_client);

    std::cout << "Hello World client" << std::endl;

    ctx.set_verify_callback(callback);

    asio::ssl::dtls::socket<asio::ip::udp::socket> dtls_con(context, ctx);

    asio::ip::udp::endpoint ep(asio::ip::address_v4::loopback(), 9000);

    try
    {
        dtls_con.lowest_layer().connect(ep);

        dtls_con.handshake(asio::ssl::stream_base::handshake_type::client);

        char buffer[] = "Hello world!";
        dtls_con.send(asio::const_buffers_1(buffer, sizeof(buffer)));

        std::array<char, 200> recbuffer{};
        dtls_con.receive(asio::buffer(recbuffer));

        std::cout << "Received: " << recbuffer.data() << std::endl;
    }
    catch(std::exception &e)
    {
        std::cout << e.what() << std::endl;
    }
}
#else
int main(){
    return 0;
}
#endif