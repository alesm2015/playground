#include <set>
#include <string>
#include <sstream>
#include <cctype>

#include "parser.h"
#include "booking.h"


static std::string trim(const std::string& str) 
{
    size_t first = str.find_first_not_of(' ');
    if (first == std::string::npos) 
        return "";

    size_t last = str.find_last_not_of(' ');
    return str.substr(first, last - first + 1);
}


std::set<uint32_t> str_to_seats(const std::string& str, uint32_t max_value) 
{
    std::set<uint32_t> numbers_set;
    std::istringstream iss(str);
    std::string sstr;
    
    while (std::getline(iss, sstr, ',')) {
        sstr = trim(sstr);
        if (sstr.empty()) 
            continue;
        
        size_t minus = sstr.find('-');
        if (minus != std::string::npos) {
            /*we are in the range*/
            std::string start = sstr.substr(0, minus);
            std::string end = sstr.substr(minus + 1);
            
            uint32_t istart = (start.empty()) ? 0 : static_cast<uint32_t>(std::stoul(start));
            uint32_t iend = (end.empty()) ? max_value : static_cast<uint32_t>(std::stoul(end));
            if (istart > max_value)
                istart = max_value;
            if (iend > max_value)
            iend = max_value;
            
            for (uint32_t i = istart; i <= iend; ++i) {
                numbers_set.insert(i);
            }
        } else {
            /*it is single number*/
            uint32_t i = static_cast<uint32_t>(std::stoul(sstr));
            if (i > max_value)
                i = max_value;
            numbers_set.insert(i);
        }
    }
    
    return numbers_set;
}


std::set<uint32_t> get_seats(const std::string &seats)
{
    return str_to_seats(seats, CBooking::get_max_seats());
}
