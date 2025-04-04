#pragma once

#include <set>
#include <vector>


//#include <boost/coroutine/all.hpp>
#include <boost/asio.hpp>
//#include <boost/bind/bind.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/ip/basic_endpoint.hpp>

#include "session.h"


// https://think-async.com/Asio/boost_asio_1_30_2/doc/html/boost_asio/example/cpp20/coroutines/chat_server.cpp

class CServer
{
private:
    struct listener_ctx
    {
        boost::asio::ip::tcp::acceptor acceptor_;
        std::shared_ptr<CSession> session_;
    };

public:
    CServer(CBooking &booking); //default constructor
    ~CServer(); //standard destructor

    /*
    https://think-async.com/Asio/boost_asio_1_30_2/doc/html/boost_asio/reference/ip__basic_endpoint_lt__InternetProtocol__gt___gt_.html
    */
    template <typename InternetProtocol>
    int32_t add_listener(boost::asio::io_context &io_context, const InternetProtocol& internet_protocol, uint16_t port)
    {
        boost::asio::ip::tcp::endpoint const endpoint(internet_protocol, port);

        return add_listener(io_context, endpoint);
    }

    /*
    https://beta.boost.org/doc/libs/1_64_0/doc/html/boost_asio/reference/ip__address/address.html
    */
    int32_t add_listener(boost::asio::io_context &io_context, const boost::asio::ip::address &ip_address, uint16_t port)
    {
        boost::asio::ip::tcp::endpoint const endpoint(ip_address, port);

        return add_listener(io_context, endpoint);
    }

private:
    int32_t add_listener (boost::asio::io_context &io_context, boost::asio::ip::tcp::endpoint const &endpoint);
    boost::asio::awaitable<void> listener(std::shared_ptr<listener_ctx> ctx_ptr);

private:
    CBooking &m_booking;
    std::vector<std::shared_ptr<listener_ctx>> m_listener_ctx_vector;
};


