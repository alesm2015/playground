#pragma once

#include <set>
#include <map>
#include <string>
#include <mutex>
#include <atomic>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <cli/cli.h>

#include "booker.h"


class CBooking
{
public:
    struct theatre_reservation
    {
        using reserved_map_t = std::map<std::string, std::set<uint32_t>>;

        std::set<uint32_t> free_reservations_set_;
        reserved_map_t reserved_map_;
    };

    struct movie
    {
        using theatres_map_t = std::map<std::string, theatre_reservation>;

        std::mutex mutex_;
        theatres_map_t theatre_reservations_map_;
    };

    using movies_map_t = std::map<std::string, std::unique_ptr<movie>>;
    using movies_map_it_t = movies_map_t::iterator;

public:
    CBooking();

    int32_t load_data(const boost::property_tree::ptree &pt);

    int32_t join_booker(CBooker::booker_ptr booker);
    int32_t leave_booker(CBooker::booker_ptr booker);

    const movies_map_t &get_configuration(void) const {return m_movies_map;};

    int32_t book_seats (
        CBooker::booker_ptr booker, 
        const std::string &movie,
        const std::string &theatre,
        std::set<uint32_t> &seats,
        std::vector<uint32_t> &unavalable_seats,
        bool best_effort = false);

    int32_t unbook_seats (
        CBooker::booker_ptr booker, 
        const std::string &movie,
        const std::string &theatre,
        std::set<uint32_t> &seats,
        std::vector<uint32_t> &invalid_seats);

    int32_t get_free_seats (
        const std::string &movie,
        const std::string &theatre,
        std::set<uint32_t> &free_seats);

    void dump_status (std::string &buffer) const;
    
    static uint32_t get_max_seats(void) {return m_max_seats_capacity;};

private:
    int32_t prepare_reservation (theatre_reservation &reservations);
    int32_t book_seats (
        CBooker::booker_ptr booker, 
        movie *p_movie,
        const std::string &theatre,
        std::set<uint32_t> &seats,
        std::vector<uint32_t> &unavalable_seats,
        bool best_effort);

    int32_t book_seats (
        CBooker::booker_ptr booker, 
        theatre_reservation &reservation,
        std::set<uint32_t> &seats,
        std::vector<uint32_t> &unavalable_seats,
        bool best_effort);

    int32_t book_seats (
        std::set<uint32_t> &free_reservations_set,
        std::set<uint32_t> &custom_reserved_set,
        std::set<uint32_t> &seats,
        std::vector<uint32_t> &unavalable_seats,
        bool best_effort);

    int32_t unbook_seats (
        CBooker::booker_ptr booker, 
        movie *p_movie,
        const std::string &theatre,
        std::set<uint32_t> &seats,
        std::vector<uint32_t> &invalid_seats);

    int32_t unbook_seats (
        CBooker::booker_ptr booker, 
        theatre_reservation &reservation,
        std::set<uint32_t> &seats,
        std::vector<uint32_t> &invalid_seats);

    void dump_set (std::string buffer, const std::set<uint32_t> &set) const;
        
private:
    std::atomic<uint32_t> m_connections_ctx;
    std::set<CBooker::booker_ptr> m_active_bookers_set;

    movies_map_t m_movies_map;

private:
    static constexpr uint32_t m_max_seats_capacity = 20;
};

