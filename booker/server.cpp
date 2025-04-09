
#include "server.h"






/// @brief Standard constructr
/// @param booking [in] Refrence to booking ctx
CServer::CServer(CBooking &booking) :\
    m_booking(booking)
{
    m_current_connections = 0;

    m_on_close_cb = std::bind(&CServer::on_session_close_cb, this, std::placeholders::_1);
    assert(m_on_close_cb != nullptr);
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

    return static_cast<int32_t>(m_listener_ctx_vector.size());
}

/// @brief Listener coroutine
/// @param ctx_ptr [in] Listening control routine
/// @return Coorutine return, so that the code can continue
boost::asio::awaitable<void> CServer::listener(std::shared_ptr<listener_ctx> ctx_ptr)
{
    bool b_exit;
    bool can_start;
    std::unique_lock<std::mutex> lck(m_mutex, std::defer_lock);

    auto it = m_listener_ctx_vector.insert(m_listener_ctx_vector.end(), ctx_ptr);

    std::shared_ptr<CSession> session;
    do {
        b_exit = false;
        boost::asio::ip::tcp::tcp::tcp::acceptor::endpoint_type peer_endpoint; /*!< client IP address */
        session = std::make_shared<CSession>(co_await ctx_ptr->acceptor_.async_accept(peer_endpoint, boost::asio::use_awaitable), m_booking);
        if (session) {
            lck.lock();
            can_start = (m_current_connections < m_max_active_connections);
            m_current_connections++;
            lck.unlock();

            if (can_start != true) {
                session = nullptr;
                continue;
            }

            auto it = m_active_sessions.insert(\
                std::pair<std::shared_ptr<CSession>, std::shared_ptr<listener_ctx>>(session, ctx_ptr));
            if (it.second != true) {
                session = nullptr;
                m_current_connections--;
                continue;
            }

            session->set_on_close_cb(m_on_close_cb);
            session->start(peer_endpoint);
        }
        else
            b_exit = true;

        session = nullptr;
    } while (b_exit != true);

    lck.lock();
    m_listener_ctx_vector.erase(it);
}


    /// @brief callback function called when session has been terminated
    /// @param session_ptr [in] session
void CServer::on_session_close_cb(std::shared_ptr<class CSession> session_ptr)
{
    if (session_ptr == nullptr)
        return;

    m_active_sessions.erase(session_ptr);
}

/// @brief Close all listening ports
void CServer::close_listening_ports (void)
{
    std::lock_guard<std::mutex> lck(m_mutex);

    for(auto listen_ctx: m_listener_ctx_vector) {
        if (listen_ctx->acceptor_.is_open())
            listen_ctx->acceptor_.close();
    }
}

/// @brief Close all active sessions
/// @return Negative on error, positive on success
int32_t CServer::close_all_sessions(void)
{
    do {
        auto it = m_active_sessions.begin();
        if (it == m_active_sessions.end())
            break;

        it->first->on_close(); //close sessiion
    } while(true);

    return EXIT_SUCCESS;
}

