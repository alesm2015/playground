#pragma once

#include <memory>

class CBooker
{
public:
    using booker_ptr = std::shared_ptr<CBooker>;

public:
    virtual ~CBooker() {};

    const std::string &get_booker_uid(void) const {return m_uid;};
    void set_uid(const std::string &uid) {m_uid = uid;};

private:
    std::string m_uid;
};


