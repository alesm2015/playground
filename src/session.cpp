#include "session.h"

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/redirect_error.hpp>


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







CSession::CSession(boost::asio::ip::tcp::socket socket, CBooking &booking) :\
    m_socket(std::move(socket)), m_timer(m_socket.get_executor()), m_booking(booking)
{
    m_b_exit_ready = false;
    m_timer.expires_at(std::chrono::steady_clock::time_point::max());

    m_p_telnet = telnet_init(m_my_telopts, ::telnet_event_handler_cb, 0, this);
    assert(m_p_telnet != nullptr);

    init_cli();
}

CSession::~CSession()
{
    on_close();

    telnet_free(m_p_telnet);
}

void CSession::start(boost::asio::ip::tcp::tcp::tcp::acceptor::endpoint_type &peer_endpoint)
{
    int32_t rc;
    std::string pretty_peer_uid;

    m_peer_endpoint = peer_endpoint;
    pretty_peer_uid  = m_peer_endpoint.address().to_string();
    pretty_peer_uid += ":";
    pretty_peer_uid += std::to_string(m_peer_endpoint.port());

    rc = m_booking.join_booker(shared_from_this());
    if (rc < EXIT_SUCCESS) {
        on_close();
        return;
    }
    pretty_peer_uid += "@";
    pretty_peer_uid += std::to_string(rc);
    set_uid(pretty_peer_uid);

    boost::asio::co_spawn(m_socket.get_executor(),\
        [self = shared_from_this()]{ return self->on_recv(); },\
        boost::asio::detached);

    boost::asio::co_spawn(m_socket.get_executor(),\
        [self = shared_from_this()]{ return self->on_send(); },\
        boost::asio::detached);

    /*send init commands*/
    telnet_negotiate(m_p_telnet, TELNET_WILL, TELNET_TELOPT_ECHO);
    telnet_negotiate(m_p_telnet, TELNET_WILL, TELNET_TELOPT_SGA);
    m_cli_session_ptr->OnConnect();
}

void CSession::set_uid(const std::string &new_uid)
{
    CBooker::set_uid(new_uid);
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
#if 0
        std::array<uint8_t, 1> recv_data;
        do {
            std::size_t n = co_await boost::asio::async_read(m_socket, boost::asio::buffer(recv_data),\
                boost::asio::use_awaitable);
            if (n == 0) {
                on_close();
                co_return;
            }
            telnet_recv(m_p_telnet, reinterpret_cast<const char *>(recv_data.data()), n);
        } while(true);
#elif 1
        std::vector<uint8_t> read_msg;
        do {
            std::size_t n = co_await boost::asio::async_read(m_socket,\
                boost::asio::dynamic_buffer(read_msg, 1024),\
                boost::asio::transfer_at_least(1), \
                boost::asio::use_awaitable);
            if (n == 0) {
                on_close();
                co_return;
            }
            telnet_recv(m_p_telnet, reinterpret_cast<const char *>(read_msg.data()), n);
            read_msg.clear();
        } while(true);
#else
        for (std::vector<uint8_t> read_msg;;) {
            std::size_t n = co_await boost::asio::async_read_until(m_socket,\
                boost::asio::dynamic_buffer(read_msg, 1024), "\n", boost::asio::use_awaitable);
            if (n == 0) {
                on_close();
                co_return;
            }
            telnet_recv(m_p_telnet, reinterpret_cast<const char *>(read_msg.data()), n);
            //m_timer.cancel_one();
            read_msg.erase(read_msg.begin(), read_msg.begin() + n);
        }
#endif
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
                if (m_b_exit_ready) {
                    on_close();
                    co_return;
                }
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

void CSession::send_raw_msg(const char *message)
{
    std::string msg(message);
    send_raw_msg(msg);
}

void CSession::send_raw_msg(const std::string &message)
{
    std::vector<uint8_t> msg(message.begin(), message.end());
    send_raw_msg(msg);
}

void CSession::send_raw_msg(std::vector<uint8_t> &message)
{
    m_send_msgs_deque.push_back(std::move(message));
    m_timer.cancel_one();
}

void CSession::send_msg(const std::string &message)
{
    std::vector<uint8_t> msg(message.begin(), message.end());
    send_msg(msg);
}

void CSession::send_msg(std::vector<uint8_t> &message)
{
    telnet_send(m_p_telnet, reinterpret_cast<const char *>(message.data()), message.size());
}

void CSession::send_text_msg(const std::string &message)
{
    telnet_send_text(m_p_telnet, message.c_str(), message.size());
}

void CSession::cli_enter(std::ostream &out)
{
    out << "Hello: " + CBooker::get_booker_uid() + "\n";
}
void CSession::cli_exit(std::ostream &out)
{
    out << "Bye ...\n";
    m_b_exit_ready = true;
}

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

void CSession::init_cli(void)
{
    std::size_t pos;
    const CBooking::movies_map_t &movies_map = m_booking.get_configuration();

    auto rootMenu = std::make_unique<cli::Menu>("cli");

#if 1   
    int a1 = 1;
    int a2 = 2;

    //auto f1 = std::bind(&CSession::test1, this, std::placeholders::_1, std::placeholders::_2);
    m_test2 = std::bind(&CSession::test2, this, std::placeholders::_1, std::placeholders::_2, a1, a2);

    rootMenu->Insert(
        "status",
        [](std::ostream& out){ out << "Hello, world\n"; },
        "Show current booking status" );
    rootMenu->Insert("test1", m_test1, "test1");
    rootMenu->Insert("test2", m_test2, "test2");
#endif

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

    /**/
    for (auto& movie : movies_map) {
        std::string help = "Movie: " + movie.first;
        auto new_menu_movie = std::make_unique<cli::Menu>(movie.first, help, movie.first);
        if (new_menu_movie == nullptr) {
            return;
        }

        pos = 0;
        for (auto& theatre : movie.second->theatre_reservations_map_) {
            cli_theatre_cmds new_cli_theatre_cmd;
            std::string help2 = "Theatre: " + theatre.first;

            auto new_menu_theatre = std::make_unique<cli::Menu>(theatre.first, help2, theatre.first);
            if (new_menu_theatre == nullptr) {
                return;
            }
            new_cli_theatre_cmd.theatre = theatre.first;

            new_cli_theatre_cmd.seats.cli_cmd_cb = std::bind(
                &CSession::test2,
                this,
                std::placeholders::_1,
                std::placeholders::_2,
                m_movie_cmd_vector.size(),
                pos);
            new_cli_theatre_cmd.seats.cmd_handler = new_menu_theatre->Insert(
                "seats",
                new_cli_theatre_cmd.seats.cli_cmd_cb,
                "Show free seats");

            new_cli_theatre_cmd.book.cmd_handler = new_menu_theatre->Insert(
            "book",
            [&](std::ostream& out)
            {

            },
            "Book selected seats");


            new_cli_theatre_cmd.try_book.cmd_handler = new_menu_theatre->Insert(
            "trybook",
            [&](std::ostream& out)
            {

            },
            "Try to book selected seats");

            new_cli_theatre_cmd.un_book.cmd_handler = new_menu_theatre->Insert(
            "unbook",
            [&](std::ostream& out)
            {

            },
            "Release selected seats");

            new_cli_theatre_cmd.status.cmd_handler = new_menu_theatre->Insert(
                "status",
                [&](std::ostream& out)
                {

                },
                "Show our booking status"); 

                new_cli_theatre_cmd.theatre_menu =\
                    new_menu_movie->Insert(std::move(new_menu_theatre));
        }

    }

    m_cli_ptr = std::make_unique<cli::Cli>(std::move(rootMenu));
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

    m_cli_session_ptr = std::make_unique<cli::CliCustomTerminalSession>
    (
        *m_cli_ptr.get(),
        m_scheduler,
        std::bind(&CSession::cli_enter, this, std::placeholders::_1),
        std::bind(&CSession::cli_exit, this, std::placeholders::_1),
        std::bind(&CSession::send_text_msg, this, std::placeholders::_1)
    );
}

