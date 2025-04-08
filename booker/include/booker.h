#pragma once

#include <memory>


/*! \brief CBooker class.
 *         Simple class to hold booker or session ID
 *
 */
class CBooker
{
public:
    using booker_ptr = std::shared_ptr<CBooker>;

public:
    /// @brief Standard destructor
    virtual ~CBooker() {};

    /// @brief get the booker uid
    const std::string &get_booker_uid(void) const {return m_uid;};

    /// @brief set the booker uid
    /// @param [in] booker uid
    void set_uid(const std::string &uid) {m_uid = uid;};

private:
    std::string m_uid;
};


