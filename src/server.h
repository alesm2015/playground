#pragma once

#include <set>
#include <vector>

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/ip/basic_endpoint.hpp>

#include "session.h"



/*! \brief Server class.
 *         This class holds all listening ports and handles all the accpetance connections
 *
 *  This is the main class, which holds all the listening ports.
 *  Refrence:
 *      https://think-async.com/Asio/boost_asio_1_30_2/doc/html/boost_asio/example/cpp20/coroutines/chat_server.cpp
 */
class CServer
{
private:

    struct listener_ctx
    { /*!< Structure which hold information for each listning port */
        boost::asio::ip::tcp::acceptor acceptor_; /*!< TCP listenning acceptor ctx */
        //std::set<std::shared_ptr<CSession>> m_sessions_set;
    };

public:
    /// @brief Standard constructr
    /// @param booking [in] Refrence to booking ctx
    CServer(CBooking &booking); 

    /// @brief Standard destructor
    ~CServer();

    /// @brief Add new TCP listening port
    /// @param io_context [in] Refrence to asio contex
    /// @param internet_protocol [in] Type of the internet protocol
    ///        https://think-async.com/Asio/boost_asio_1_30_2/doc/html/boost_asio/reference/ip__basic_endpoint_lt__InternetProtocol__gt___gt_.html
    /// @param port [in] Listening port
    /// @return Negative on error, positive on success
    template <typename InternetProtocol>
    int32_t add_listener(boost::asio::io_context &io_context, const InternetProtocol& internet_protocol, uint16_t port)
    {
        boost::asio::ip::tcp::endpoint const endpoint(internet_protocol, port);

        return add_listener(io_context, endpoint);
    }

    /// @brief Add new TCP listening port
    /// @param io_context [in] Refrence to asio contex
    /// @param ip_address [in] Listenning IP address
    ///        https://beta.boost.org/doc/libs/1_64_0/doc/html/boost_asio/reference/ip__address/address.html
    /// @param port [in] Listening port
    /// @return Negative on error, positive on success
    int32_t add_listener(boost::asio::io_context &io_context, const boost::asio::ip::address &ip_address, uint16_t port)
    {
        boost::asio::ip::tcp::endpoint const endpoint(ip_address, port);

        return add_listener(io_context, endpoint);
    }

private:
    /// @brief Add new TCP listening port
    /// @param io_context [in] Refrence to asio contex
    /// @param endpoint [in] Reference to the listening enpoint
    /// @return Negative on error, positive on success
    int32_t add_listener (boost::asio::io_context &io_context, boost::asio::ip::tcp::endpoint const &endpoint);

    /// @brief Listener coroutine
    /// @param ctx_ptr [in] Listening control routine
    /// @return Coorutine return, so that the code can continue
    boost::asio::awaitable<void> listener(std::shared_ptr<listener_ctx> ctx_ptr);

private:
    CBooking &m_booking; /*!< Refrence to booking class */
    std::vector<std::shared_ptr<listener_ctx>> m_listener_ctx_vector; /*!< vector of all listening ports */
};


