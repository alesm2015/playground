#include <iostream>
#include <sstream>


#include "server.h"




int main(int argc, char* argv[])
{
    int threads;
    CBooking booking;
    CServer server(booking);
    boost::property_tree::ptree pt;

    threads = 1;
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
        "}";

    std::stringstream ss;
    ss << data;
    boost::property_tree::read_json(ss, pt);

    std::ostringstream oss;
    boost::property_tree::write_json(oss, pt);
    std::cout << oss.str();

    booking.load_data(pt);

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

