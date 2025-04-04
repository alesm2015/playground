#include <iostream>

#include "server.h"




int main(int argc, char* argv[])
{
    int threads;
    CBooking booking;
    CServer server(booking);

    threads = 1;

    try
    {
        boost::asio::io_context io_context(threads);

        server.add_listener(io_context, boost::asio::ip::tcp::v4(), 50000);

        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&](auto, auto){ io_context.stop(); });

        io_context.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return -1;
}

