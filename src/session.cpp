#include "session.h"

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/redirect_error.hpp>


CSession::CSession(boost::asio::ip::tcp::socket socket, CBooking &booking) :\
    m_socket(std::move(socket)), m_timer(m_socket.get_executor()), m_booking(booking)
{
    m_timer.expires_at(std::chrono::steady_clock::time_point::max());
}

CSession::~CSession()
{
    on_close();
}

void CSession::start(void)
{
    m_booking.join_booker(shared_from_this());

    boost::asio::co_spawn(m_socket.get_executor(),\
        [self = shared_from_this()]{ return self->on_recv(); },\
        boost::asio::detached);

    boost::asio::co_spawn(m_socket.get_executor(),\
        [self = shared_from_this()]{ return self->on_send(); },\
        boost::asio::detached);
}

void CSession::on_close(void)
{
    int32_t rc;

    rc = m_booking.leave_booker(shared_from_this());
    assert(rc >= 0);

    if (m_socket.is_open())
        m_socket.close();
    
    m_timer.cancel();    
}

boost::asio::awaitable<void> CSession::on_recv(void)
{
    try
    {
        for (std::string read_msg;;) {
            std::size_t n = co_await boost::asio::async_read_until(m_socket,\
                boost::asio::dynamic_buffer(read_msg, 1024), "\n", boost::asio::use_awaitable);

            read_msg.erase(0, n);
        }
    }
    catch(const std::exception& e)
    {
        on_close();
    }
    
}

boost::asio::awaitable<void> CSession::on_send(void)
{
    try
    {
        while (m_socket.is_open()) {
            if (m_send_msgs_deque.empty()) {
                boost::system::error_code ec;
                co_await m_timer.async_wait(redirect_error(boost::asio::use_awaitable, ec));
            }
            else {
                co_await boost::asio::async_write(m_socket,\
                    boost::asio::buffer(m_send_msgs_deque.front()), boost::asio::use_awaitable);
                m_send_msgs_deque.pop_front();
            }
        }
    }
    catch(const std::exception& e)
    {
        //std::cerr << e.what() << '\n';
        on_close();
    }
    
}
