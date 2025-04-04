#pragma once

#include <set>

#include <cli/cli.h>

#include "booker.h"


class CBooking
{
public:
    CBooking();

    int32_t join_booker(CBooker::booker_ptr booker);
    int32_t leave_booker(CBooker::booker_ptr booker);

private:
    std::set<CBooker::booker_ptr> m_active_bookers_set;
};

