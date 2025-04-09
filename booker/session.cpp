#include <iostream>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/redirect_error.hpp>

#include "session.h"
#include "parser.h"


/// @brief support telnet callback function 
/// @param telnet [in] pointer to telnet ctx
/// @param event [out] event type
/// @param user_data [in] pointer to session class
static void telnet_event_handler_cb
(
    telnet_t *telnet,
    telnet_event_t *event,
    void *user_data
)
{
    CSession *p_CSession;

    assert(telnet != nullptr);
    assert(event != nullptr);
    assert(user_data != nullptr);

    p_CSession = static_cast<CSession *>(user_data);
    assert(p_CSession != nullptr);

    p_CSession->telnet_event_handler_cb(telnet, event);
}






/// @brief Standard constructr
/// @param socket [in] session socket
/// @param booking [in] Refrence to booking ctx
CSession::CSession(boost::asio::ip::tcp::socket socket, CBooking &booking) :\
    m_socket(std::move(socket)), m_timer(m_socket.get_executor()), m_booking(booking)
{
    m_b_exit_ready = false;
    m_b_exit_done = false;
    m_timer.expires_at(std::chrono::steady_clock::time_point::max());

    m_p_telnet = telnet_init(m_my_telopts, ::telnet_event_handler_cb, 0, this);
    assert(m_p_telnet != nullptr);

    init_cli();
}

/// @brief Standard destructor
CSession::~CSession()
{
    assert(m_b_exit_done == true);

    telnet_free(m_p_telnet);
}

/// @brief Start TCP session
/// @param peer_endpoint [in] Remote IP address
void CSession::start(boost::asio::ip::tcp::tcp::tcp::acceptor::endpoint_type &peer_endpoint)
{
    int32_t rc;
    std::string pretty_peer_uid;

    assert(m_on_close_cb != nullptr);

    /*create session pretty name, for booking identificator*/
    m_peer_endpoint = peer_endpoint;
    pretty_peer_uid  = m_peer_endpoint.address().to_string();
    pretty_peer_uid += ":";
    pretty_peer_uid += std::to_string(m_peer_endpoint.port());

    /*join us to booker*/
    rc = m_booking.join_booker(shared_from_this());
    if (rc < EXIT_SUCCESS) {
        on_close();
        return;
    }
    pretty_peer_uid += "@";
    pretty_peer_uid += std::to_string(rc);
    set_uid(pretty_peer_uid);

    /*spawn TX & RX parts*/
    boost::asio::co_spawn(m_socket.get_executor(),\
        [self = shared_from_this()]{ return self->on_recv(); },\
        boost::asio::detached);

    boost::asio::co_spawn(m_socket.get_executor(),\
        [self = shared_from_this()]{ return self->on_send(); },\
        boost::asio::detached);

    /*Init Telnet protocol and CLI*/
    telnet_negotiate(m_p_telnet, TELNET_WILL, TELNET_TELOPT_ECHO);
    telnet_negotiate(m_p_telnet, TELNET_WILL, TELNET_TELOPT_SGA);
    m_cli_session_ptr->OnConnect();
}

/// @brief Set unique session id
/// @param new_uid New uid session identificator
void CSession::set_uid(const std::string &new_uid)
{
    CBooker::set_uid(new_uid);
}

/// @brief Close TCP connection
void CSession::on_close(void)
{
    /*Unregister us from booker*/
    m_booking.leave_booker(shared_from_this());

    if (m_on_close_cb)
        (m_on_close_cb)(shared_from_this());

    /*Close socket*/
    if (m_socket.is_open())
        m_socket.close();
    
    m_timer.cancel(); 
    m_b_exit_done = true;
}

/// @brief RX coroutine
/// @param  none
/// @return Coorutine return, so that the code can continue
boost::asio::awaitable<void> CSession::on_recv(void)
{
    try
    {
        std::vector<uint8_t> read_msg;
        do {
            /*loop untill all bytes are not received, than go to sleep*/
            std::size_t n = co_await boost::asio::async_read(m_socket,\
                boost::asio::dynamic_buffer(read_msg, 1024),\
                boost::asio::transfer_at_least(1), \
                boost::asio::use_awaitable);
            if (n == 0) {
                on_close();
                co_return;
            }

            /*send received bytes to telnet control library*/
            telnet_recv(m_p_telnet, reinterpret_cast<const char *>(read_msg.data()), n);
            read_msg.clear();
        } while(true);
    }
    catch(const std::exception& e)
    {
        on_close();
    }
}

/// @brief TX coroutine
/// @param  none
/// @return Coorutine return, so that the code can continue
boost::asio::awaitable<void> CSession::on_send(void)
{
    try
    {
        while (m_socket.is_open()) {
            /*loop untill all messages are not sens*/
            if (m_send_msgs_deque.empty()) {
                boost::system::error_code ec;
                if (m_b_exit_ready) {
                    on_close();
                    co_return;
                }
                /*if, there is nothing to be send, we wait in this timer, otherwise we will loop forever*/
                co_await m_timer.async_wait(redirect_error(boost::asio::use_awaitable, ec));
            }
            else {
                /*send message*/
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

/// @brief Send message directly to socket
/// @param message [in] message to be send
void CSession::send_raw_msg(const char *message)
{
    std::string msg(message);
    send_raw_msg(msg);
}

/// @brief Send message directly to socket
/// @param message [in] message to be send
void CSession::send_raw_msg(const std::string &message)
{
    std::vector<uint8_t> msg(message.begin(), message.end());
    send_raw_msg(msg);
}

/// @brief Send message directly to socket
/// @param message [in] message to be send
void CSession::send_raw_msg(std::vector<uint8_t> &message)
{
    m_send_msgs_deque.push_back(std::move(message));
    m_timer.cancel_one();
}

/// @brief Send message troug telnet protocol library
/// @param message [in] message to be send
void CSession::send_msg(const std::string &message)
{
    std::vector<uint8_t> msg(message.begin(), message.end());
    send_msg(msg);
}

/// @brief Send message troug telnet protocol library
/// @param message [in] message to be send
void CSession::send_msg(std::vector<uint8_t> &message)
{
    telnet_send(m_p_telnet, reinterpret_cast<const char *>(message.data()), message.size());
}

/// @brief Callback function called by CLI, to send text message to tlnet library
/// @param message [in] message to be send
void CSession::cli_send_text_msg_cb(const std::string &message)
{
    telnet_send_text(m_p_telnet, message.c_str(), message.size());
}

/// @brief Hello message to be send to the CLI console
/// @param out [out] Stream to send message
void CSession::cli_enter_cb(std::ostream &out)
{
    out << "Hello: " + CBooker::get_booker_uid() + "\n";
}

/// @brief Exit message to be send to the CLI console
/// @param out [out] Stream to send message
void CSession::cli_exit_cb(std::ostream &out)
{
    out << "Bye ...\n";
    m_b_exit_ready = true;
}

/// @brief Telent protocol callback function
/// @param telnet [in] Telent ctx
/// @param event [in] Event type
void CSession::telnet_event_handler_cb (telnet_t *telnet, telnet_event_t *event)
{
    std::vector<uint8_t> new_send_msg;
    std::vector<uint8_t> new_recv_msg;

    assert(telnet == m_p_telnet);

    switch (event->type) 
    {
    case TELNET_EV_DATA:
        /* The DATA event is triggered whenever regular data (not part of any special TELNET command) is received. */
        /* this will be input typed by the user.*/
        new_recv_msg.resize(event->data.size);
        std::copy(event->data.buffer, event->data.buffer + event->data.size, new_recv_msg.begin());
        m_cli_session_ptr->Read(new_recv_msg);
        break;
    case TELNET_EV_SEND:
        /* This event is sent whenever libtelnet has generated data that must be sent over the wire to the remove end. */
        /* Generally that means calling send() or adding the data to your application's output buffer. */
        new_send_msg.resize(event->data.size);
        std::copy(event->data.buffer, event->data.buffer + event->data.size, new_send_msg.begin());
        send_raw_msg(new_send_msg);
        break;
    case TELNET_EV_ERROR:
        // event->error.msg
        on_close();
        break;
    default:
        /*don't process any other messages*/
        break;
    }
}

/// @brief Inicialise CLI contex
/// @param  none
void CSession::init_cli(void)
{
    std::size_t pos;
    const CBooking::movies_map_t &movies_map = m_booking.get_configuration();

    /*Create root menu holder*/
    auto rootMenu = std::make_unique<cli::Menu>("cli");
    assert(rootMenu != nullptr);

    /*Status command*/
    m_cli_status_cmd_cb = std::bind(&CSession::status_cb, this, std::placeholders::_1, std::placeholders::_2);
    assert(m_cli_status_cmd_cb != nullptr);
    rootMenu->Insert(
        "status",
        m_cli_status_cmd_cb,
        "Show current booking status" );

    /*turn on colors command*/
    m_colorCmd = rootMenu->Insert(
        "color",
        [&](std::ostream& out)
        {
            out << "Colors ON\n";
            cli::SetColor();
            m_colorCmd.Disable();
            m_nocolorCmd.Enable();
        },
        "Enable colors in the cli" );

    /*turn off colors command*/
    m_nocolorCmd = rootMenu->Insert(
        "nocolor",
        [&](std::ostream& out)
        {
            out << "Colors OFF\n";
            cli::SetNoColor();
            m_colorCmd.Enable();
            m_nocolorCmd.Disable();
        },
        "Disable colors in the cli" );

    /*Build commands based from the configuration*/
    for (auto& movie : movies_map) {
        cli_movie_cmds new_cli_movie;
        std::string help = "Movie: " + movie.first;
        /*movie control holder*/
        auto new_menu_movie = std::make_unique<cli::Menu>(movie.first, help, movie.first);
        assert(new_menu_movie != nullptr);

        new_cli_movie.movie = movie.first;

        pos = 0;
        for (auto& theatre : movie.second->theatre_reservations_map_) {
            cli_theatre_cmds new_cli_theatre_cmd;
            std::string help2 = "Theatre: " + theatre.first;

            /*theatre control holder*/
            auto new_menu_theatre = std::make_unique<cli::Menu>(theatre.first, help2, theatre.first);
            assert(new_menu_theatre != nullptr);

            new_cli_theatre_cmd.theatre = theatre.first;

            /*seats*/
            new_cli_theatre_cmd.seats.cli_cmd_cb = std::bind(
                &CSession::free_seats_cb,
                this,
                std::placeholders::_1,
                std::placeholders::_2,
                m_movie_cmd_vector.size(),
                pos);
            assert(new_cli_theatre_cmd.seats.cli_cmd_cb != nullptr);
            new_cli_theatre_cmd.seats.cmd_handler = new_menu_theatre->Insert(
                "seats",
                new_cli_theatre_cmd.seats.cli_cmd_cb,
                "Show free seats");

            /*book*/
            new_cli_theatre_cmd.book.cli_cmd_cb = std::bind(
                &CSession::book_seats_cb,
                this,
                std::placeholders::_1,
                std::placeholders::_2,
                m_movie_cmd_vector.size(),
                pos);
            assert(new_cli_theatre_cmd.book.cli_cmd_cb != nullptr);
            new_cli_theatre_cmd.book.cmd_handler = new_menu_theatre->Insert(
            "book",
            new_cli_theatre_cmd.book.cli_cmd_cb,
            "Book selected seats");

            /*trybook*/
            new_cli_theatre_cmd.try_book.cli_cmd_cb = std::bind(
                &CSession::trybook_seats_cb,
                this,
                std::placeholders::_1,
                std::placeholders::_2,
                m_movie_cmd_vector.size(),
                pos);
            assert(new_cli_theatre_cmd.try_book.cli_cmd_cb != nullptr);
            new_cli_theatre_cmd.try_book.cmd_handler = new_menu_theatre->Insert(
            "trybook",
            new_cli_theatre_cmd.try_book.cli_cmd_cb,
            "Try to book selected seats");

            /*unbook*/
            new_cli_theatre_cmd.unbook.cli_cmd_cb = std::bind(
                &CSession::unbook_seats_cb,
                this,
                std::placeholders::_1,
                std::placeholders::_2,
                m_movie_cmd_vector.size(),
                pos);
            assert(new_cli_theatre_cmd.unbook.cli_cmd_cb != nullptr);
            new_cli_theatre_cmd.unbook.cmd_handler = new_menu_theatre->Insert(
            "unbook",
            new_cli_theatre_cmd.unbook.cli_cmd_cb,
            "Release selected seats");

            /*status*/
            new_cli_theatre_cmd.status.cli_cmd_cb = std::bind(
                &CSession::book_status_cb,
                this,
                std::placeholders::_1,
                std::placeholders::_2,
                m_movie_cmd_vector.size(),
                pos);
            assert(new_cli_theatre_cmd.status.cli_cmd_cb != nullptr);
            new_cli_theatre_cmd.status.cmd_handler = new_menu_theatre->Insert(
                "status",
                new_cli_theatre_cmd.status.cli_cmd_cb,
                "Show our booking status"); 

            new_cli_theatre_cmd.theatre_menu =\
                new_menu_movie->Insert(std::move(new_menu_theatre));
            new_cli_movie.theatre_cmd_vector.push_back(std::move(new_cli_theatre_cmd));
            pos++;
        }

        if (new_cli_movie.theatre_cmd_vector.size()) {
            new_cli_movie.movie_menu =\
                rootMenu->Insert(std::move(new_menu_movie));

            m_movie_cmd_vector.push_back(std::move(new_cli_movie));
        }
    }

    /*initialise main cli engine*/
    m_cli_ptr = std::make_unique<cli::Cli>(std::move(rootMenu));
    assert(m_cli_ptr != nullptr);
    m_cli_ptr->StdExceptionHandler(
        [](std::ostream& out, const std::string& cmd, const std::exception& e)
        {
            out << cli::beforeError 
                << "Exception caught in CLI handler: "
                << e.what()
                << " while handling command: "
                << cmd
                << "."
                << cli::afterError
                << "\n";
        }
    );

    // custom handler for unknown commands
    m_cli_ptr->WrongCommandHandler(
        [](std::ostream& out, const std::string& cmd)
        {
            out << cli::beforeError 
                << "Unknown command or incorrect parameters: "
                << cmd
                << "."
                << cli::afterError
                << "\n";
        }
    );
    m_colorCmd.Disable(); // start with colors

    /*terminal session*/
    m_cli_session_ptr = std::make_unique<cli::CliCustomTerminalSession>(
        *m_cli_ptr.get(),
        m_scheduler,
        std::bind(&CSession::cli_enter_cb, this, std::placeholders::_1),
        std::bind(&CSession::cli_exit_cb, this, std::placeholders::_1),
        std::bind(&CSession::cli_send_text_msg_cb, this, std::placeholders::_1)
    );
    assert(m_cli_session_ptr != nullptr);
}

/// @brief Support function to get all the reuierd names
/// @param movie [out] Name of the movie
/// @param theatre [out] Name of the theatre
/// @param movie_pos [in] Position of the movie
/// @param theatre_pos [in] Position of the theatre
/// @return Negative on error, >=0 on success
int32_t CSession::get_names (std::string &movie, std::string &theatre, size_t movie_pos, size_t theatre_pos)
{
    std::vector<cli_theatre_cmds> *p_theatre_cmd;

    if (movie_pos >= m_movie_cmd_vector.size())
        return -EFAULT;
    movie = m_movie_cmd_vector[movie_pos].movie;

    p_theatre_cmd = &m_movie_cmd_vector[movie_pos].theatre_cmd_vector;
    if (theatre_pos >= p_theatre_cmd->size())
        return -EFAULT;
    theatre = p_theatre_cmd->at(theatre_pos).theatre;

    return EXIT_SUCCESS;
}

/// @brief Default error function, which also terminates the session
/// @param out [out] leaving message stream
/// @param msg [in] leaving message
void CSession::cli_sys_err(std::ostream& out, const std::string &msg)
{
    out << cli::beforeError;
    if (msg.empty())
        out << "System error \n";
    else {
        out << msg;
        out << "\n";
    }
    out << cli::afterError;

    m_b_exit_ready = true;
}

/// @brief Callback function from CLI which reports current seats status in theatres in all 
///         the movies
/// @param out [out] output stream
/// @param arg [in] unused
void CSession::status_cb (std::ostream& out, const std::string& arg)
{
    std::string buffer;

    (void)(arg);

    m_booking.dump_status(buffer);
    buffer += "\n";
    out << buffer;
}

/// @brief Callback function to list free seats
/// @param out [out] output stream
/// @param arg [in] unused
/// @param movie_pos [in] movie position
/// @param theatre_pos [in] theatre position
void CSession::free_seats_cb (std::ostream& out, const std::string& arg, size_t movie_pos, size_t theatre_pos)
{
    int32_t rc;
    std::string movie;
    std::string theatre;
    std::string str;
    std::set<uint32_t> free_seats;

    (void)(arg);

    /*retrive names from positions*/
    rc = get_names(movie, theatre, movie_pos, theatre_pos);
    if (rc < EXIT_SUCCESS) {
        cli_sys_err(out);
        return;
    }

    /*get list of free seats*/
    rc = m_booking.get_free_seats(movie, theatre, free_seats);
    if (rc < EXIT_SUCCESS) {
        cli_sys_err(out);
        return;
    }

    if (free_seats.empty()) {
        out << "There are no seats available\n";
    }
    else {
        seats_to_string(str, free_seats);
        out << "Free available seats: ";
        out << str;
        out << "\n";
    }
}

/// @brief Callback function to book the seats
///     If any seat from the list is already taken, 
///     none of the seats will be taken
/// @param out [out] status output stream
/// @param arg [in] booking parameters [list of seats]
/// @param movie_pos [in] movie position
/// @param theatre_pos [in] theatre position
void CSession::book_seats_cb (std::ostream& out, const std::string& arg, size_t movie_pos, size_t theatre_pos)
{
    int32_t rc;
    std::string tmp;
    std::string movie;
    std::string theatre;
    std::set<uint32_t> req_free_seats;
    std::vector<uint32_t> unavalable_seats;

    /*retrive names from positions*/
    rc = get_names(movie, theatre, movie_pos, theatre_pos);
    if (rc < EXIT_SUCCESS) {
        cli_sys_err(out);
        return;
    }

    /*convert slection text to list of seats*/
    req_free_seats = get_seats(arg);

    /*book the seats*/
    rc = m_booking.book_seats(shared_from_this(), movie, theatre, req_free_seats, unavalable_seats, false);
    if (rc < EXIT_SUCCESS) {
        out << cli::beforeError;
        out << "Failed to process an request\n";
        out << cli::afterError;
        return;
    }

    /*get latest list of currently booked list of the booker*/
    rc = m_booking.get_booked_seats(shared_from_this(), movie, theatre, req_free_seats);
    if (rc < EXIT_SUCCESS) {
        out << cli::beforeError;
        out << "Failed to process an request\n";
        out << cli::afterError;
        return;
    }

    out << cli::beforeOK;
    out << "Currently reserved seats: ";
    seats_to_string(tmp, req_free_seats);
    out << tmp;
    out << cli::afterOK;
    out << "\n";
}

/// @brief Callback function to try to book the seats
///     If any seat from the list is already taken, 
///     that seat will be skipped, while funtion will continue
//      with oter seats in the selection
/// @param out [out] status output stream
/// @param arg [in] booking parameters [list of seats]
/// @param movie_pos [in] movie position
/// @param theatre_pos [in] theatre position
void CSession::trybook_seats_cb (std::ostream& out, const std::string& arg, size_t movie_pos, size_t theatre_pos)
{
    int32_t rc;
    std::string tmp;
    std::string movie;
    std::string theatre;
    std::set<uint32_t> req_free_seats;
    std::vector<uint32_t> unavalable_seats;

    /*retrive names from positions*/
    rc = get_names(movie, theatre, movie_pos, theatre_pos);
    if (rc < EXIT_SUCCESS) {
        cli_sys_err(out);
        return;
    }

    /*convert slection text to list of seats*/
    req_free_seats = get_seats(arg);

    /*book the seats*/
    rc = m_booking.book_seats(shared_from_this(), movie, theatre, req_free_seats, unavalable_seats, true);
    if (rc < EXIT_SUCCESS) {
        out << cli::beforeError;
        out << "Failed to process an request\n";
        out << cli::afterError;
        return;
    }

    /*get latest list of currently booked list of the booker*/
    rc = m_booking.get_booked_seats(shared_from_this(), movie, theatre, req_free_seats);
    if (rc < EXIT_SUCCESS) {
        out << cli::beforeError;
        out << "Failed to process an request\n";
        out << cli::afterError;
        return;
    }

    out << cli::beforeOK;
    out << "Currently reserved seats: ";
    seats_to_string(tmp, req_free_seats);
    out << tmp;
    out << cli::afterOK;
    out << "\n";
    
    if (unavalable_seats.empty() != true) {
        tmp.clear();
        /*list of seats, which were not able to get taken*/
        out << cli::beforeWarn;
        out << "Unavailble seats: ";
        seats_to_string(tmp, unavalable_seats);
        out << tmp;
        out << cli::afterWarn;
        out << "\n";
    }
}

/// @brief Release already booked selected seats
/// @param out [out] status output stream
/// @param arg [in] booking parameters [list of seats]
/// @param movie_pos [in] movie position
/// @param theatre_pos [in] theatre position
void CSession::unbook_seats_cb (std::ostream& out, const std::string& arg, size_t movie_pos, size_t theatre_pos)
{
    int32_t rc;
    std::string tmp;
    std::string movie;
    std::string theatre;
    std::set<uint32_t> req_free_seats;
    std::vector<uint32_t> invalid_seats;

    /*retrive names from positions*/
    rc = get_names(movie, theatre, movie_pos, theatre_pos);
    if (rc < EXIT_SUCCESS) {
        cli_sys_err(out);
        return;
    }

    /*convert slection text to list of seats*/
    req_free_seats = get_seats(arg);

    /*release selected seats*/
    rc = m_booking.unbook_seats(shared_from_this(), movie, theatre, req_free_seats, invalid_seats);
    if (rc < EXIT_SUCCESS) {
        out << cli::beforeError;
        out << "Failed to process an request\n";
        out << cli::afterError;
        return;
    }

    /*get latest list of still currently booked list of the booker*/
    rc = m_booking.get_booked_seats(shared_from_this(), movie, theatre, req_free_seats);
    if (rc < EXIT_SUCCESS) {
        out << cli::beforeError;
        out << "Failed to process an request\n";
        out << cli::afterError;
        return;
    }

    out << cli::beforeOK;
    out << "Currently reserved seats: ";
    seats_to_string(tmp, req_free_seats);
    out << tmp;
    out << cli::afterOK;
    out << "\n";

    if (invalid_seats.empty() != true) {
        tmp.clear();
        /*list of the seats, which were not booked by us, or are free*/
        out << cli::beforeWarn;
        out << "Invalid seats: ";
        seats_to_string(tmp, invalid_seats);
        out << tmp;
        out << cli::afterWarn;
        out << "\n";
    }
}

/// @brief Get current status of the booker boked seats
/// @param out [out] status output stream
/// @param arg [in] booking parameters [list of seats]
/// @param movie_pos [in] movie position
/// @param theatre_pos [in] theatre position
void CSession::book_status_cb (std::ostream& out, const std::string& arg, size_t movie_pos, size_t theatre_pos)
{
    int32_t rc;
    std::string tmp;
    std::string movie;
    std::string theatre;
    std::set<uint32_t> req_seats;

    (void)(arg);

    /*retrive names from positions*/
    rc = get_names(movie, theatre, movie_pos, theatre_pos);
    if (rc < EXIT_SUCCESS) {
        cli_sys_err(out);
        return;
    }

    /*get latest list of still currently booked list of the booker*/
    rc = m_booking.get_booked_seats(shared_from_this(), movie, theatre, req_seats);
    if (rc < EXIT_SUCCESS) {
        out << cli::beforeError;
        out << "Failed to process an request\n";
        out << cli::afterError;
        return;
    }

    out << cli::beforeOK;
    out << "Currently reserved seats: ";
    seats_to_string(tmp, req_seats);
    out << tmp;
    out << cli::afterOK;
    out << "\n";
}

