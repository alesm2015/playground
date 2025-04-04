#include <cassert>

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

