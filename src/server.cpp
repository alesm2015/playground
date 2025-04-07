
#include "server.h"






/// @brief Standard constructr
/// @param booking [in] Refrence to booking ctx
CServer::CServer(CBooking &booking) :\
    m_booking(booking)
{

}

/// @brief Standard destructor
CServer::~CServer()
{
    //if (m_acceptor) {
    //    m_acceptor->close();
    //}
}

/// @brief Add new TCP listening port
/// @param io_context [in] Refrence to asio contex
/// @param endpoint [in] Reference to the listening enpoint
/// @return Negative on error, positive on success
int32_t CServer::add_listener(boost::asio::io_context &io_context, boost::asio::ip::tcp::endpoint const &endpoint)
{
    std::shared_ptr<listener_ctx> new_ctx_ptr =\
        std::make_shared<listener_ctx>(listener_ctx(boost::asio::ip::tcp::acceptor(io_context, endpoint)));

    if (new_ctx_ptr == nullptr)
        return -ENOMEM;

    /*enable address reuse. Good if port are not yet full freed by OS*/
    new_ctx_ptr->acceptor_.set_option(boost::asio::ip::tcp::tcp::acceptor::reuse_address(true));

    /*spawn new coroutine*/
    boost::asio::co_spawn(io_context,
        [this, new_ctx_ptr]() mutable -> boost::asio::awaitable<void> {
            co_await listener(new_ctx_ptr);
        }, boost::asio::detached);

    m_listener_ctx_vector.push_back(new_ctx_ptr);
    return static_cast<int32_t>(m_listener_ctx_vector.size());
}

/// @brief Listener coroutine
/// @param ctx_ptr [in] Listening control routine
/// @return Coorutine return, so that the code can continue
boost::asio::awaitable<void> CServer::listener(std::shared_ptr<listener_ctx> ctx_ptr)
{
    std::shared_ptr<CSession> session;
    do {
        boost::asio::ip::tcp::tcp::tcp::acceptor::endpoint_type peer_endpoint; /*!< client IP address */
        session = std::make_shared<CSession>(co_await ctx_ptr->acceptor_.async_accept(peer_endpoint, boost::asio::use_awaitable), m_booking);
        if (session) {
            session->start(peer_endpoint);
        }
    } while (session != nullptr);
}

