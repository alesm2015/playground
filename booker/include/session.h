#pragma once

#include <deque>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <string>
#include <functional>

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/awaitable.hpp>

#include <libtelnet.h>

#include "booking.h"
#include "customcli.h"


/*predefined class*/
class CSession;

using on_close_cb = std::function<void(std::shared_ptr<class CSession>)>; /*!< callback definition */


/*! \brief Session class.
 *         Controls single Telnet connection + CLI
 *
 *  Each TCP connection to any listening port create one instance of this class
 */
class CSession:\
    public CBooker, public std::enable_shared_from_this<CSession>
{
private:

    /// Function prototype for all booking CLI functions
    using cli_cmd_cb_t = std::function<void(std::ostream& out, const std::string& arg, size_t movie_pos, size_t theatre_pos)>;

    struct cli_cmds {
        cli_cmd_cb_t cli_cmd_cb; /*!< Function pointer to callback */
        cli::CmdHandler cmd_handler; /*!< CLI control class */
    };

    struct cli_theatre_cmds {
        std::string theatre; /*!< Theatre name */
        cli::CmdHandler theatre_menu; /*!< CLI control class */

        /*Booking functions*/
        cli_cmds seats;
        cli_cmds book;
        cli_cmds try_book;
        cli_cmds unbook;
        cli_cmds status;
    };

    struct cli_movie_cmds {
        std::string movie; /*!< Movie name */
        cli::CmdHandler movie_menu; /*!< CLI control class */
        std::vector<cli_theatre_cmds> theatre_cmd_vector; /*!< vector to theatres in movie*/ 
    };


public:
    /// @brief Standard constructr
    /// @param socket [in] session socket
    /// @param booking [in] Refrence to booking ctx
    CSession(boost::asio::ip::tcp::socket socket, CBooking &booking);
    ~CSession();

    /// @brief Start TCP session
    /// @param peer_endpoint [in] Remote IP address
    void start(boost::asio::ip::tcp::tcp::tcp::acceptor::endpoint_type &peer_endpoint);

    /// @brief Close TCP connection
    void on_close(void);

    /// @brief Set up on close callback function, to inform server class
    /// @param on_close_cb [in] callback function
    void set_on_close_cb(on_close_cb on_close_cb) {m_on_close_cb = on_close_cb;};

public:
    /// @brief Telent protocol callback function
    /// @param telnet [in] Telent ctx
    /// @param event [in] Event type
    void telnet_event_handler_cb (telnet_t *telnet, telnet_event_t *event);

private:
    /// @brief Inicialise CLI contex
    /// @param  none
    void init_cli(void);

    /// @brief Send message directly to socket
    /// @param message [in] message to be send
    void send_raw_msg(const char *message);

    /// @brief Send message directly to socket
    /// @param message [in] message to be send
    void send_raw_msg(const std::string &message);

    /// @brief Send message directly to socket
    /// @param message [in] message to be send
    void send_raw_msg(std::vector<uint8_t> &message);

    /// @brief Send message troug telnet protocol library
    /// @param message [in] message to be send    
    void send_msg(const std::string &message);

    /// @brief Send message troug telnet protocol library
    /// @param message [in] message to be send
    void send_msg(std::vector<uint8_t> &message);

    /// @brief Hello mesaage to be send to the CLI console
    /// @param out [out] Stream to send message
    void cli_enter_cb(std::ostream &out);

    /// @brief Exit message to be send to the CLI console
    /// @param out [out] Stream to send message
    void cli_exit_cb(std::ostream &out);

    /// @brief Callback function called by CLI, to send text message to tlnet library
    /// @param message [in] message to be send
    void cli_send_text_msg_cb(const std::string &message);

    /// @brief Set unique session id
    /// @param new_uid New uid session identificator
    void set_uid(const std::string &new_uid);

    /// @brief RX coroutine
    /// @param  none
    /// @return Coorutine return, so that the code can continue
    boost::asio::awaitable<void> on_recv(void);

    /// @brief TX coroutine
    /// @param  none
    /// @return Coorutine return, so that the code can continue
    boost::asio::awaitable<void> on_send(void);

private:
    /// @brief Support function to get all the reuierd names
    /// @param movie [out] Name of the movie
    /// @param theatre [out] Name of the theatre
    /// @param movie_pos [in] Position of the movie
    /// @param theatre_pos [in] Position of the theatre
    /// @return Negative on error, >=0 on success
    int32_t get_names (std::string &movie, std::string &theatre, size_t movie_pos, size_t theatre_pos);

    /// @brief Default error function, which also terminates the session
    /// @param out [out] leaving message stream
    /// @param msg [in] leaving message
    void cli_sys_err(std::ostream& out, const std::string &msg = "");

    //CLI callbacks
    /// @brief Callback function from CLI which reports current seats status in theatres in all 
    ///         the movies
    /// @param out [out] output stream
    /// @param arg [in] unused
    void status_cb (std::ostream& out, const std::string& arg);

    /// @brief Callback function to list free seats
    /// @param out [out] output stream
    /// @param arg [in] unused
    /// @param movie_pos [in] movie position
    /// @param theatre_pos [in] theatre position
    void free_seats_cb (std::ostream& out, const std::string& arg, size_t movie_pos, size_t theatre_pos);

    /// @brief Callback function to book the seats
    ///     If any seat from the list is already taken, 
    ///     none of the seats will be taken
    /// @param out [out] status output stream
    /// @param arg [in] booking parameters [list of seats]
    /// @param movie_pos [in] movie position
    /// @param theatre_pos [in] theatre position
    void book_seats_cb (std::ostream& out, const std::string& arg, size_t movie_pos, size_t theatre_pos);

    /// @brief Callback function to try to book the seats
    ///     If any seat from the list is already taken, 
    ///     that seat will be skipped, while funtion will continue
    //      with oter seats in the selection
    /// @param out [out] status output stream
    /// @param arg [in] booking parameters [list of seats]
    /// @param movie_pos [in] movie position
    /// @param theatre_pos [in] theatre position
    void trybook_seats_cb (std::ostream& out, const std::string& arg, size_t movie_pos, size_t theatre_pos);

    /// @brief Release already booked selected seats
    /// @param out [out] status output stream
    /// @param arg [in] booking parameters [list of seats]
    /// @param movie_pos [in] movie position
    /// @param theatre_pos [in] theatre position
    void unbook_seats_cb (std::ostream& out, const std::string& arg, size_t movie_pos, size_t theatre_pos);
    void book_status_cb (std::ostream& out, const std::string& arg, size_t movie_pos, size_t theatre_pos);

private:
    bool m_b_exit_done; /*!< variable set, if session was properly unregistered */
    bool m_b_exit_ready; /*!< variable to exit the session, when all messages are send*/
    telnet_t *m_p_telnet; /*!< telnet control structure*/
    on_close_cb m_on_close_cb; /*!< on close callback to inform server class, that we died*/

    /*cli*/
    cli::CmdHandler m_colorCmd;
    cli::CmdHandler m_nocolorCmd;

    /*CLI specific variables*/
    std::unique_ptr<cli::Cli> m_cli_ptr;
    std::unique_ptr<cli::CliCustomTerminalSession> m_cli_session_ptr;
    cli::CliCustomLoopScheduler m_scheduler;

    /*Socet specific variables*/
    boost::asio::ip::tcp::socket m_socket;
    boost::asio::steady_timer m_timer;
    boost::asio::ip::tcp::tcp::tcp::acceptor::endpoint_type m_peer_endpoint;
    std::deque<std::vector<uint8_t>> m_send_msgs_deque;

    CBooking &m_booking;

    /*dynamic CLI configuration, based on movies and theatres*/
    cli_cmd_cb_t m_cli_status_cmd_cb;
    std::vector<cli_movie_cmds> m_movie_cmd_vector;

private:
    /*default telnet options*/
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

