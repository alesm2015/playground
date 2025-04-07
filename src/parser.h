#pragma once

#include <set>
#include <string>
#include <vector>
#include <cstdint>

/// @brief Convert text to array of numbers
/// @param seats [in] array in string
/// @return array of numbers
std::set<uint32_t> get_seats(const std::string &seats);

/// @brief Convert array back to string
/// @param seats [out] string
/// @param seats_set [in] array
void seats_to_string(std::string &seats, const std::set<uint32_t> &seats_set);

/// @brief Convert array back to string
/// @param seats [out] string
/// @param seats_vector [in] array
void seats_to_string(std::string &seats, const std::vector<uint32_t> &seats_vector);


