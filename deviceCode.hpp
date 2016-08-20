#pragma once

#include<string>
#include<iostream>

namespace core
{
    class DeviceCode
    {
    public:
        const std::string& codeGet() const
        {
            return code_;
        };

        static DeviceCode& instance();

    protected:
        std::string code_;
    };
}

