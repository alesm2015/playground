#pragma once

#include <deque>
#include <memory>
#include <string>
#include <vector>
#include <mutex>

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/awaitable.hpp>

#include <libtelnet.h>

#include "booking.h"
#include "customcli.h"


class CSession:\
    public CBooker, public std::enable_shared_from_this<CSession>
{
public:
    CSession(boost::asio::ip::tcp::socket socket, CBooking &booking);
    ~CSession();

    void start(void);
    void on_close(void);

public:
    void telnet_event_handler_cb (telnet_t *telnet, telnet_event_t *event);

private:
    void init_cli(void);
    void send_raw_msg(const char *message);
    void send_raw_msg(const std::string &message);
    void send_raw_msg(std::vector<uint8_t> &message);

    void send_msg(const std::string &message);
    void send_msg(std::vector<uint8_t> &message);
    void send_text_msg(const std::string &message);

    void cli_enter(std::ostream &out);
    void cli_exit(std::ostream &out);

    boost::asio::awaitable<void> on_recv(void);
    boost::asio::awaitable<void> on_send(void);

private:
    bool m_b_exit_ready;
    telnet_t *m_p_telnet;

    /*cli*/
    cli::CmdHandler m_colorCmd;
    cli::CmdHandler m_nocolorCmd;
    std::unique_ptr<cli::Cli> m_cli_ptr;
    std::unique_ptr<cli::CliCustomTerminalSession> m_cli_session_ptr;
    cli::CliCustomLoopScheduler m_scheduler;

    boost::asio::ip::tcp::socket m_socket;
    boost::asio::steady_timer m_timer;
    std::deque<std::vector<uint8_t>> m_send_msgs_deque;

    CBooking &m_booking;

private:
    static constexpr telnet_telopt_t m_my_telopts[] = {
        /*          id                  us        remote   */
        { TELNET_TELOPT_ECHO,      TELNET_WILL, TELNET_DONT },
        { TELNET_TELOPT_TTYPE,     TELNET_WILL, TELNET_DONT },
        { TELNET_TELOPT_COMPRESS2, TELNET_WONT, TELNET_DO   },
        { TELNET_TELOPT_ZMP,       TELNET_WONT, TELNET_DO   },
        { TELNET_TELOPT_MSSP,      TELNET_WONT, TELNET_DO   },
        { TELNET_TELOPT_BINARY,    TELNET_WILL, TELNET_DO   },
        { TELNET_TELOPT_NAWS,      TELNET_WILL, TELNET_DONT },
        { -1, 0, 0 }
    };
};

