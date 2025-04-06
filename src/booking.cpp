#include <cassert>

#include <boost/optional/optional.hpp>

#include "booking.h"



CBooking::CBooking()
{

}


int32_t CBooking::join_booker(CBooker::booker_ptr booker)
{
    if (booker == nullptr)
        return -EINVAL;

    auto it = m_active_bookers_set.insert(booker);
    if (it.second != true)
        return -EEXIST;

    return m_active_bookers_set.size();
}

int32_t CBooking::leave_booker(CBooker::booker_ptr booker)
{
    if (booker == nullptr)
        return -EINVAL;

    std::size_t size = m_active_bookers_set.erase(booker);
    return size;
}

int32_t CBooking::prepare_reservation (theatre_reservation &reservations)
{
    for (uint32_t seat = 0; seat < m_max_seats_capacity; ++seat) {
        auto rc = reservations.free_reservations_set_.insert(seat);
        if (rc.second != true) {
            return -ENOMEM;
        }
    }

    return static_cast<int32_t>(m_max_seats_capacity);
}

int32_t CBooking::load_data(const boost::property_tree::ptree &pt)
{
    int32_t rc;

    auto movies = pt.find("movies");
    if (movies == pt.not_found()) {
        return -EBADMSG;
    }

    for (auto it = movies->second.begin(); it != movies->second.end(); ++it) {
        auto new_movie = std::make_unique<movie>();
        if (new_movie == nullptr) {
            return -ENOMEM;
        }

        auto movie_pt = it->second.find("movie");
        if (movie_pt == it->second.not_found()) {
            return -EBADMSG;
        }

        auto theatres = it->second.find("theatres");
        if (theatres == it->second.not_found()) {
            return -EBADMSG;
        }

        for (auto it2 = theatres->second.begin(); it2 != theatres->second.end(); ++it2) {
            theatre_reservation new_theatre;

            std::string theatre = it2->second.get_value<std::string>();

            rc = prepare_reservation(new_theatre);
            if (rc < 0) {
                return rc;
            }

            if (theatre.empty()) {
                return -EBADMSG;
            }

            auto map_rc = new_movie->theatre_reservations_map_.insert(std::pair<std::string, theatre_reservation>(std::move(theatre), std::move(new_theatre)));
            if (map_rc.second != true) {
                return -EEXIST;
            }
        }

        if (movie_pt->first.empty()) {
            return -EBADMSG;
        }

        std::string movie_name = movie_pt->second.get_value<std::string>();

        auto map_rc = m_movies_map.insert(std::pair<std::string, std::unique_ptr<movie>>(std::move(movie_name), std::move(new_movie)));
        if (map_rc.second != true) {
            return -EEXIST;
        }
    }

    return EXIT_SUCCESS;
}

int32_t CBooking::book_seats 
(
    CBooker::booker_ptr booker, 
    const std::string &movie,
    const std::string &theatre,
    std::vector<uint32_t> &seats,
    std::vector<uint32_t> &unavalable_seats,
    bool best_effort
)
{
    assert(booker != nullptr);

    auto it_movie = m_movies_map.find(movie);
    if (it_movie == m_movies_map.end()) {
        return -EEXIST;
    }

    std::lock_guard<std::mutex> lck(it_movie->second->mutex_);

    return book_seats(booker, it_movie->second.get(), theatre, seats, unavalable_seats, best_effort);
}

int32_t CBooking::book_seats 
(
    CBooker::booker_ptr booker,
    movie *p_movie,
    const std::string &theatre,
    std::vector<uint32_t> &seats,
    std::vector<uint32_t> &unavalable_seats,
    bool best_effort
)
{
    assert(p_movie != nullptr);
    assert(p_movie->mutex_.try_lock() != true);

    auto it_theatre = p_movie->theatre_reservations_map_.find(theatre);
    if (it_theatre == p_movie->theatre_reservations_map_.end()) {
        return -EEXIST;
    }

    return book_seats(booker, it_theatre->second, seats, unavalable_seats, best_effort);
}

int32_t CBooking::book_seats 
(
    CBooker::booker_ptr booker, 
    theatre_reservation &reservation,
    std::vector<uint32_t> &seats,
    std::vector<uint32_t> &unavalable_seats,
    bool best_effort
)
{
    int32_t rc;
    bool new_booker;
    std::set<uint32_t> new_custom_used_seats_set;
    std::set<uint32_t> *p_custom_used_seats_set;

    new_booker = false;
    auto booker_it = reservation.reserved_map_.find(booker->get_booker_uid());
    if (booker_it == reservation.reserved_map_.end()) {
        new_booker = true;
        p_custom_used_seats_set = &new_custom_used_seats_set;
    }
    else {
        p_custom_used_seats_set = &booker_it->second;
    }

    rc = book_seats(reservation.free_reservations_set_, *p_custom_used_seats_set, seats, unavalable_seats, best_effort);
    if (rc < 0) {
        return rc;
    }

    if (p_custom_used_seats_set->empty()) {
        return EXIT_SUCCESS;
    }

    if (new_booker) {
        auto it = reservation.reserved_map_.insert(std::pair<std::string, std::set<uint32_t>>(booker->get_booker_uid(), std::move(new_custom_used_seats_set)));
        if (it.second != true) {
            return -ENOMEM;
        }
    }

    return rc;
}

int32_t CBooking::book_seats
(
    std::set<uint32_t> &free_reservations_set,
    std::set<uint32_t> &custom_reserved_set,
    std::vector<uint32_t> &seats,
    std::vector<uint32_t> &unavalable_seats,
    bool best_effort
)
{
    std::set<uint32_t> new_reserved_seats;

    for (uint32_t seat : seats) {
        auto free_seat_it = free_reservations_set.find(seat);
        if (free_seat_it == free_reservations_set.end()) {
            /*OK, seat is already book, is ours?*/
            auto reserved_seat_it = custom_reserved_set.find(seat);
            if (reserved_seat_it == custom_reserved_set.end()) {
                /*no it is not*/
                unavalable_seats.push_back(seat);
            }
        }
        else {
            auto it = new_reserved_seats.insert(seat);
            if (it.second != true) {
                return -ENOMEM;
            }
            free_reservations_set.erase(free_seat_it);
        }
    }

    if ((unavalable_seats.empty() != true)&&(best_effort == false)) {
        /*we failed to book all the required seats*/
        for (uint32_t seat : new_reserved_seats) {
            auto it = free_reservations_set.insert(seat);
            if (it.second != true) {
                return -ENOMEM;
            }
        }
        return EXIT_SUCCESS;
    }

    custom_reserved_set.insert(new_reserved_seats.begin(), new_reserved_seats.end());
    return static_cast<int32_t>(seats.size());
}

int32_t CBooking::unbook_seats 
(
    CBooker::booker_ptr booker, 
    const std::string &movie,
    const std::string &theatre,
    std::vector<uint32_t> &seats,
    std::vector<uint32_t> &invalid_seats
)
{
    assert(booker != nullptr);

    auto it_movie = m_movies_map.find(movie);
    if (it_movie == m_movies_map.end()) {
        return -EEXIST;
    }

    std::lock_guard<std::mutex> lck(it_movie->second->mutex_);

    return unbook_seats(booker, it_movie->second.get(), theatre, seats, invalid_seats);    
}

int32_t CBooking::unbook_seats 
(
    CBooker::booker_ptr booker, 
    movie *p_movie,
    const std::string &theatre,
    std::vector<uint32_t> &seats,
    std::vector<uint32_t> &invalid_seats
)
{
    assert(p_movie != nullptr);
    assert(p_movie->mutex_.try_lock() != true);

    auto it_theatre = p_movie->theatre_reservations_map_.find(theatre);
    if (it_theatre == p_movie->theatre_reservations_map_.end()) {
        return -EEXIST;
    }

    return unbook_seats(booker, it_theatre->second, seats, invalid_seats);
}

int32_t CBooking::unbook_seats 
(
    CBooker::booker_ptr booker, 
    theatre_reservation &reservation,
    std::vector<uint32_t> &seats,
    std::vector<uint32_t> &invalid_seats
)
{
    int32_t rc;

    auto booker_it = reservation.reserved_map_.find(booker->get_booker_uid());
    if (booker_it == reservation.reserved_map_.end()) {
        invalid_seats = seats;
        return static_cast<int32_t>(invalid_seats.size());
    }

    rc = 0;
    for (uint32_t seat : seats) {
        auto custom_reservation_it = booker_it->second.find(seat);
        if (custom_reservation_it != booker_it->second.end()) {
            rc++;
            auto it = reservation.free_reservations_set_.insert(seat);
            if (it.second != true) {
                return -ENOMEM;
            }
            booker_it->second.erase(custom_reservation_it);
        }
        else {
            invalid_seats.push_back(seat);
        }
    }

    if (booker_it->second.empty()) {
        reservation.reserved_map_.erase(booker_it);
    }

    return rc;
}

int32_t CBooking::get_free_seats (
    const std::string &movie,
    const std::string &theatre,
    std::set<uint32_t> &free_seats
)
{
    auto it_movie = m_movies_map.find(movie);
    if (it_movie == m_movies_map.end()) {
        return -EEXIST;
    }

    std::lock_guard<std::mutex> lck(it_movie->second->mutex_);

    auto it_theatre = it_movie->second->theatre_reservations_map_.find(theatre);
    if (it_theatre == it_movie->second->theatre_reservations_map_.end()) {
        return -EEXIST;
    }

    free_seats = it_theatre->second.free_reservations_set_;
    return EXIT_SUCCESS;
}

void CBooking::dump_set (std::string buffer, const std::set<uint32_t> &set) const
{
    bool add_comma;

    add_comma = false;
    for (uint32_t seat : set) {
        if (add_comma)
            buffer += ",";

        buffer += std::to_string(seat);
        add_comma = true;
    }
}

void CBooking::dump_status (std::string &buffer) const
{
    std::string tmp_buffer;
    const char ch_offset[] = "   ";

    for (auto it = m_movies_map.begin(); it != m_movies_map.end(); ++it) {
        buffer += "Movie: " + it->first;
        buffer += "\n";

        std::lock_guard<std::mutex> lck(it->second->mutex_);

        for (auto it2 = it->second->theatre_reservations_map_.begin(); it2 != it->second->theatre_reservations_map_.end(); ++it2) {
            buffer += ch_offset;
            buffer += "Theater: " + it2->first;
            buffer += "\n";

            buffer += ch_offset;
            buffer += "  ";
            buffer += "Free seats: ";
            tmp_buffer.clear();
            dump_set(tmp_buffer, it2->second.free_reservations_set_);
            buffer += std::move(tmp_buffer);
            buffer += "\n";

            buffer += ch_offset;
            buffer += "  ";
            buffer += "Allocated seats: ";
            buffer += "\n";

            for (auto it3 = it2->second.reserved_map_.begin(); it3 != it2->second.reserved_map_.end(); ++it3) {
                buffer += ch_offset;
                buffer += ch_offset;
                buffer += it3->first;

                std::size_t size = it3->first.size();
                if (size > 20)
                    size = 0;
                else
                    size = 20 - size;
                while(size) {
                    buffer += " ";
                    size--;
                }
                buffer += ": ";
                tmp_buffer.clear();
                dump_set(tmp_buffer, it3->second);
                buffer += std::move(tmp_buffer);
                buffer += "\n";
            }
        }
    }
}

