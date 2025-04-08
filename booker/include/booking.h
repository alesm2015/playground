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


/*! \brief CBooking class.
 *         Control bookings per movie, per theatre, per user
 *
 *  Main class which controls the entire booking functions
 *  Also hold the list of all the movies and list of all the theatres
 *  were each movie is being played.
 */
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
    /// @brief Standard constructor
    CBooking();

    /// @brief Load dynamic configuration
    /// @param pt [in] configuration tree
    /// @return Negative on error, >=0 on success
    int32_t load_data(const boost::property_tree::ptree &pt);

    /// @brief Join new session as booker
    /// @param booker [in] new bookr
    /// @return Negative on error, >=0 on success
    int32_t join_booker(CBooker::booker_ptr booker);

    /// @brief Leave us(booker) from booking. Session was closed
    /// @param booker [in] Removing booker
    /// @return Negative on error, >=0 on success
    int32_t leave_booker(CBooker::booker_ptr booker);

    /// @brief get current cinema configuration
    /// @return configuration itself
    const movies_map_t &get_configuration(void) const {return m_movies_map;};

    /// @brief Book the list of seats
    /// @param booker [in] booker uid
    /// @param movie [in] movie, which gets booked
    /// @param theatre [in] theatre where movie is played
    /// @param seats [in] array of booking seats
    /// @param unavalable_seats [out] list of seats, which are already taken.
    ///                     But were in our request
    /// @param best_effort [in] true, to skip already booked seats
    /// @return Negative on error, >=0 on success
    int32_t book_seats (
        CBooker::booker_ptr booker, 
        const std::string &movie,
        const std::string &theatre,
        std::set<uint32_t> &seats,
        std::vector<uint32_t> &unavalable_seats,
        bool best_effort = false);

    /// @brief Release already taken seats
    /// @param booker [in] booker uid
    /// @param movie [in] movie, which gets booked
    /// @param theatre [in] theatre where movie is played
    /// @param seats [in] array of booking seats
    /// @param invalid_seats [out] list of seats, which are not tkaen taken by us
    /// @return Negative on error, >=0 on success
    int32_t unbook_seats (
        CBooker::booker_ptr booker, 
        const std::string &movie,
        const std::string &theatre,
        std::set<uint32_t> &seats,
        std::vector<uint32_t> &invalid_seats);

    /// @brief Get the list of booked seats per booker
    /// @param booker [in] booker uid
    /// @param movie [in] movie
    /// @param theatre [in] theatre where movie is played
    /// @param seats [out] array of booked seats
    /// @return Negative on error, >=0 on success
    int32_t get_booked_seats (
        CBooker::booker_ptr booker, 
        const std::string &movie,
        const std::string &theatre,
        std::set<uint32_t> &seats);

    /// @brief Get the list of free seats
    /// @param booker [in] booker uid
    /// @param movie [in] movie
    /// @param theatre [in] theatre
    /// @param free_seats [out] list of free seats
    /// @return Negative on error, >=0 on success
    int32_t get_free_seats (
        const std::string &movie,
        const std::string &theatre,
        std::set<uint32_t> &free_seats);

    /// @brief Show current status within movies within theatres
    /// @param buffer [out] Buffer where the status is stored in
    ///         human readable form
    void dump_status (std::string &buffer) const;
    
    /// @brief Get the maximum number of seats in theatre
    /// @return value of seats
    static uint32_t get_max_seats(void) {return m_max_seats_capacity;};

private:
    /// @brief Create list of empty seats
    /// @param reservations [out] Location, where list needs to be stored
    /// @return Negative on error, >=0 on success
    int32_t prepare_reservation (theatre_reservation &reservations);

    /// @brief Book the list of seats
    /// @param booker [in] booker uid
    /// @param movie [in] ptr to movie ctx
    /// @param theatre [in] theatre where movie is played
    /// @param seats [in] array of booking seats
    /// @param unavalable_seats [out] list of seats, which are already taken.
    ///                     But were in our request
    /// @param best_effort [in] true, to skip already booked seats
    /// @return Negative on error, >=0 on success
    int32_t book_seats (
        CBooker::booker_ptr booker, 
        movie *p_movie,
        const std::string &theatre,
        std::set<uint32_t> &seats,
        std::vector<uint32_t> &unavalable_seats,
        bool best_effort);

    /// @brief Book the list of seats
    /// @param booker [in] booker uid
    /// @param reservation [in] ptr to reservation ctx
    /// @param seats [in] array of booking seats
    /// @param unavalable_seats [out] list of seats, which are already taken.
    ///                     But were in our request
    /// @param best_effort [in] true, to skip already booked seats
    /// @return Negative on error, >=0 on success
    int32_t book_seats (
        CBooker::booker_ptr booker, 
        theatre_reservation &reservation,
        std::set<uint32_t> &seats,
        std::vector<uint32_t> &unavalable_seats,
        bool best_effort);

    /// @brief Book the list of seats
    /// @param free_reservations_set [in] list of free seats
    /// @param custom_reserved_set [in] list of required seats to be booked
    /// @param seats [in] array of booking seats
    /// @param unavalable_seats [out] list of seats, which are already taken.
    ///                     But were in our request
    /// @param best_effort [in] true, to skip already booked seats
    /// @return Negative on error, >=0 on success
    int32_t book_seats (
        std::set<uint32_t> &free_reservations_set,
        std::set<uint32_t> &custom_reserved_set,
        std::set<uint32_t> &seats,
        std::vector<uint32_t> &unavalable_seats,
        bool best_effort);

    /// @brief Release already taken seats
    /// @param booker [in] booker uid
    /// @param p_movie [in] ptr to movie
    /// @param theatre [in] theatre where movie is played
    /// @param seats [in] array of booking seats
    /// @param invalid_seats [out] list of seats, which are not tkaen taken by us
    /// @return Negative on error, >=0 on success
    int32_t unbook_seats (
        CBooker::booker_ptr booker, 
        movie *p_movie,
        const std::string &theatre,
        std::set<uint32_t> &seats,
        std::vector<uint32_t> &invalid_seats);

    /// @brief Release already taken seats
    /// @param booker [in] booker uid
    /// @param reservation [in] ptr to reservation ctx
    /// @param seats [in] array of booking seats
    /// @param invalid_seats [out] list of seats, which are not tkaen taken by us
    /// @return Negative on error, >=0 on success
    int32_t unbook_seats (
        CBooker::booker_ptr booker, 
        theatre_reservation &reservation,
        std::set<uint32_t> &seats,
        std::vector<uint32_t> &invalid_seats);
        
private:
    std::atomic<uint32_t> m_connections_ctx;
    std::set<CBooker::booker_ptr> m_active_bookers_set; /*!< list of currently active sessions - bookers*/

    movies_map_t m_movies_map; /*!< configuration movies & theatres and ocupation*/

private:
    static constexpr uint32_t m_max_seats_capacity = 20;
};

