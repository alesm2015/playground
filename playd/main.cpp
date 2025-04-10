#include <iostream>
#include <sstream>
#include <string>

#ifndef _WIN32
    #include <unistd.h>
    #include <syslog.h>
    #include <getopt.h>
    #include <signal.h>
    #include <fcntl.h>
    #include <sys/stat.h>
#else
    #include "getopt.h"
#endif /* _WIN32 */

#include <string.h>


#include "server.h"
#include "version.h"


/*Settings for daemon*/
#define BD_NO_CHDIR           0x01 /* Don't chdir ("/") */
#define BD_NO_CLOSE_FILES     0x02 /* Don't close all open files */
#define BD_NO_REOPEN_STD_FDS  0x04 /* Don't reopen stdin, stdout, and stderr to /dev/null */
#define BD_NO_SINGLE_INSTANCE 0x08 /* Don't use single instance*/
#define BD_NO_UMASK0          0x10 /* Don't do a umask(0) */
#define BD_MAX_CLOSE          8192 /* Max file descriptors to close if sysconf(_SC_OPEN_MAX) is indeterminate */

/*Global variables*/
std::string pid_file_path;  /*!< Filepath to pid file */
std::string app_name;  /*!< Our name */
int lpf;  /*!< File descriptor of pid file */

/// @brief Get our process uid
/// @return pid
static uint32_t get_pid(void)
{
#if defined(_WIN32)
	return (uint32_t)GetCurrentProcessId();
#else	
	return (uint32_t)getpid();
#endif
}

#ifndef _WIN32
/// @brief Start us as daemon
/// @param flags [in] Configuration
static void daemonize(uint32_t flags)
{
    pid_t pid;
    int maxfd, fd, rc;
    char ch_tmp[260];

    /* The first fork will change our pid
    * but the sid and pgid will be the
    * calling process.
    */
    pid = fork();    

    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);    

     /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);    

    /*
    * Run the process in a new session without a controlling
    * terminal. The process group ID will be the process ID
    * and thus, the process will be the process group leader.
    * After this call the process will be in a new session,
    * and it will be the progress group leader in a new
    * process group.
    */
    if (setsid() < 0) // become leader of new session
        exit(EXIT_FAILURE);               

    /*
    * We will fork again, also known as a
    * double fork. This second fork will orphan
    * our process because the parent will exit.
    * When the parent process exits the child
    * process will be adopted by the init process
    * with process ID 1.
    * The result of this second fork is a process
    * with the parent as the init process with an ID
    * of 1. The process will be in it's own session
    * and process group and will have no controlling
    * terminal. Furthermore, the process will not
    * be the process group leader and thus, cannot
    * have the controlling terminal if there was one.
    */
    pid = fork();     

    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);            

    /* Set new file permissions */
    if(!(flags & BD_NO_UMASK0))
        umask(0);
        
    /* Change the working directory to the root directory */
    /* or another appropriated directory */
    if(!(flags & BD_NO_CHDIR))
        chdir("/");

    if(!(flags & BD_NO_CLOSE_FILES))  // close all open files
    {
        maxfd = sysconf(_SC_OPEN_MAX);
        if(maxfd == -1)
            maxfd = BD_MAX_CLOSE;  
        for(fd = 0; fd < maxfd; fd++)
            close(fd);
    }

    if(!(flags & BD_NO_REOPEN_STD_FDS))
    { //set stnadard io to /dev/null
        close(STDIN_FILENO);

        fd = open("/dev/null", O_RDWR);
        if(fd != STDIN_FILENO)
            exit(EXIT_FAILURE);

        if(dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
            exit(EXIT_FAILURE);
        if(dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
            exit(EXIT_FAILURE);          
    }

    /* Open lock file, so we make sure, that single instance is running only*/
    if ((pid_file_path.empty() == false)&&(!(flags & BD_NO_SINGLE_INSTANCE)))
    {     
        rc = snprintf((char *)&ch_tmp, sizeof(ch_tmp), "%s/%s",\
        pid_file_path.c_str(), app_name.c_str());
        std::cout << ch_tmp << "\r\n";
        if ((rc < 0)||((size_t)rc > sizeof(ch_tmp)))
            exit(EXIT_FAILURE);

        lpf = open(ch_tmp, O_RDWR | O_CREAT | O_TRUNC, 0640);
        if (lpf < 0)
            exit(EXIT_FAILURE); 
        if (lockf(lpf, F_TLOCK, 0) < 0)
            exit(EXIT_FAILURE); 
        snprintf((char *)&ch_tmp, sizeof(ch_tmp), "%u\n", get_pid());
        if (write(lpf, ch_tmp, strlen(ch_tmp)) < 0)
            exit(EXIT_FAILURE); 
    }

    /* Open the log file */
    openlog (app_name.c_str(), LOG_PID, LOG_DAEMON);        
}
#else
static void daemonize(uint32_t flags)
{
    (void)(flags);
    return;
}
#endif /*_WIN32*/



/// @brief signal event handler callback
/// @param timer [io] refrence to timer
/// @param error [in] Result of operation.
/// @param signal_number [in] Indicates which signal occurred.
static void signal_handler_cb
(
    boost::asio::steady_timer &timer,
    const boost::system::error_code& error,
    int signal_number
) 
{
    (void)(signal_number);

    if (!error) {
        timer.cancel_one();
    }
    else {
        //error, just crash the program
        exit(-1);
    }
}




/// @brief Shut down coroutine
/// @param server [io] server reference
/// @param io_context [io] boost io context
/// @param timer [io] timer reference
/// @return none
static boost::asio::awaitable<void> on_shut_down
(
    CServer& server,
    boost::asio::io_context& io_context,
    boost::asio::steady_timer &timer
)
{
    boost::system::error_code ec;

    /*wait for timer event*/
    co_await timer.async_wait(redirect_error(boost::asio::use_awaitable, ec));

    //start with shut down process
    server.close_listening_ports(); //close all listening ports
    timer.expires_after(std::chrono::milliseconds(100));
    co_await timer.async_wait(redirect_error(boost::asio::use_awaitable, ec));

    server.close_all_sessions(); //close all sessions
    timer.expires_after(std::chrono::milliseconds(100));
    co_await timer.async_wait(redirect_error(boost::asio::use_awaitable, ec));

    io_context.stop(); //stop io_context
    co_return;
}





/// @brief Main entry
/// @param argc [in] number of parameters
/// @param argv [in] parameter value
/// @return Program compltition
int main(int argc, char* argv[])
{
    int rc;
    int threads;
    bool bdaemonize;
    CBooking booking;
    CServer server(booking);
    boost::property_tree::ptree pt;

    (void)(argc);
    (void)(argv);

    lpf = -1;
    app_name = "playd"; /*!< App name */
    pid_file_path = "/tmp"; /*!< Location of lock file */

    std::cout << app_name << " version " << playd_VERSION_MAJOR << "." << playd_VERSION_MINOR << "." << playd_VERSION_PATCH << " started\n";

    #ifdef BUILD_DAEMON
        bdaemonize = true;
    #else
        bdaemonize = false;
    #endif
    if (bdaemonize)
        daemonize(0x00);

    /*default configuration*/
    threads = 2; /*!< number of threads */
    std::string data =\
        "{"\
        "\"movies\": ["\
        "               {"\
        "                   \"movie\": \"GodFather\","
        "                   \"theatres\": ["\
        "                           \"Tokyo\","\
        "                           \"Delhi\","\
        "                           \"Shanghai\","\
        "                           \"SaoPaulo\","\
        "                           \"MexicoCity\""\
        "                               ]"\
        "               },"\
        "               {"\
        "                   \"movie\": \"Matrix\","
        "                   \"theatres\": ["\
        "                           \"Tokyo\","\
        "                           \"MexicoCity\""\
        "                               ]"\
        "               },"\
        "               {"\
        "                   \"movie\": \"Inception\","
        "                   \"theatres\": ["\
        "                           \"Shanghai\","\
        "                           \"SaoPaulo\","\
        "                           \"MexicoCity\""\
        "                               ]"\
        "               }"\
        "            ]"\
        "}"; /*!< Dummy configuration data */

    std::stringstream ss;
    ss << data;
    boost::property_tree::read_json(ss, pt);

#if 0
    /*for debugging purposes only*/
    std::ostringstream oss;
    boost::property_tree::write_json(oss, pt);
    std::cout << oss.str();
#endif

    rc = booking.load_data(pt);
    if (rc < EXIT_SUCCESS) {
        return rc;
    }

    try
    {
        boost::asio::io_context io_context(threads);
        boost::asio::steady_timer timer(io_context, std::chrono::steady_clock::time_point::max());
        
        /*spawn new coroutine*/
        boost::asio::co_spawn(io_context,
            [&server, &io_context, &timer]() mutable -> boost::asio::awaitable<void> {
                co_await on_shut_down(server, io_context, timer);
            }, boost::asio::detached);

        /*add listenning ports*/
        server.add_listener(io_context, boost::asio::ip::tcp::v4(), 50000);

        /*signal control*/
        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM, SIGQUIT);
        signals.async_wait([&timer](const boost::system::error_code& error, int signal_number) {
            signal_handler_cb(timer, error, signal_number);
        });

        /*run io contex*/
        io_context.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return EXIT_SUCCESS;
}

