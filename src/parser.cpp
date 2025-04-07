#include <set>
#include <string>
#include <sstream>
#include <cctype>

#include "parser.h"
#include "booking.h"


/// @brief trim the string
/// @param str [in] string needs to be trimmed
/// @return trimmed string
static std::string trim(const std::string& str) 
{
    size_t first = str.find_first_not_of(' ');
    if (first == std::string::npos) 
        return "";

    size_t last = str.find_last_not_of(' ');
    return str.substr(first, last - first + 1);
}

/// @brief Convert text to array of numbers
///         "1, 2, 5"  -> [1, 2, 5]
///         "1, 2, 7 - 9"  -> [1, 2, 7, 8, 9]
/// @param str [in] array in string
/// @param max_value [in] max allowed number
/// @return array of numbers
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

/// @brief Convert text to array of numbers
/// @param seats [in] array in string
/// @return array of numbers
std::set<uint32_t> get_seats(const std::string &seats)
{
    return str_to_seats(seats, CBooking::get_max_seats());
}

/// @brief Convert array back to string
/// @param seats [out] string
/// @param seats_set [in] array
void seats_to_string(std::string &seats, const std::set<uint32_t> &seats_set)
{
    bool is_first;

    is_first = true;
    for(auto seat: seats_set) {
        if (is_first != true)
            seats += ", ";

        seats += std::to_string(seat);
        is_first = false;
    }
}

/// @brief Convert array back to string
/// @param seats [out] string
/// @param seats_vector [in] array
void seats_to_string(std::string &seats, const std::vector<uint32_t> &seats_vector)
{
    bool is_first;

    is_first = true;
    for(auto seat: seats_vector) {
        if (is_first != true)
            seats += ", ";

        seats += std::to_string(seat);
        is_first = false;
    }    
}

