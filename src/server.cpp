
#include "server.h"







CServer::CServer(CBooking &booking) :\
    m_booking(booking)
{

}

CServer::~CServer()
{
    //if (m_acceptor) {
    //    m_acceptor->close();
    //}
}

int32_t CServer::add_listener(boost::asio::io_context &io_context, boost::asio::ip::tcp::endpoint const &endpoint)
{
    std::shared_ptr<listener_ctx> new_ctx_ptr =\
        std::make_shared<listener_ctx>(listener_ctx(boost::asio::ip::tcp::acceptor(io_context, endpoint), nullptr));

    if (new_ctx_ptr == nullptr)
        return -ENOMEM;

    new_ctx_ptr->acceptor_.set_option(boost::asio::ip::tcp::tcp::acceptor::reuse_address(true));
    boost::asio::co_spawn(io_context,
        [this, new_ctx_ptr]() mutable -> boost::asio::awaitable<void> {
            co_await listener(new_ctx_ptr);
        }, boost::asio::detached);

    m_listener_ctx_vector.push_back(new_ctx_ptr);
    return static_cast<int32_t>(m_listener_ctx_vector.size());
}

boost::asio::awaitable<void> CServer::listener(std::shared_ptr<listener_ctx> ctx_ptr)
{

    do {
        ctx_ptr->session_ = std::make_shared<CSession>(co_await ctx_ptr->acceptor_.async_accept(boost::asio::use_awaitable), m_booking);
        if (ctx_ptr->session_ == nullptr) {
            co_return;
        }
        ctx_ptr->session_->start();
    } while (ctx_ptr->session_ != nullptr);

    //co_return;
}

