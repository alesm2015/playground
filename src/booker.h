#pragma once

#include <memory>

class CBooker
{
public:
    using booker_ptr = std::shared_ptr<CBooker>;

public:
    virtual ~CBooker() {};

};


