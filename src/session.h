#pragma once

#include <deque>
#include <memory>
#include <string>

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/awaitable.hpp>

#include "booking.h"


class CSession:\
    public CBooker, public std::enable_shared_from_this<CSession>
{
public:
    CSession(boost::asio::ip::tcp::socket socket, CBooking &booking);
    ~CSession();

    void start(void);
    void on_close(void);

private:
    boost::asio::awaitable<void> on_recv(void);
    boost::asio::awaitable<void> on_send(void);

private:
    CBooking &m_booking;
    boost::asio::ip::tcp::socket m_socket;
    boost::asio::steady_timer m_timer;
    std::deque<std::string> m_send_msgs_deque;
};

