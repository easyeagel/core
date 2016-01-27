
#include"codec.hpp"
#include"message.hpp"

namespace core
{

void MessageBase::encode(std::string& msg) const
{
    core::encode(msg, static_cast<uint16_t>(cmd_));
    core::encode(msg, static_cast<uint32_t>(dict_.size()));
    for(auto& kv: dict_)
        core::encode(msg, static_cast<uint16_t>(kv.first), kv.second);
}

void MessageBase::decode(uint32_t& start, const std::string& msg)
{
    core::decode(start, msg, reinterpret_cast<uint16_t&>(cmd_));
    uint32_t count=0;
    core::decode(start, msg, count);
    while(start<msg.size())
    {
        uint16_t key;
        SimpleValue val;
        core::decode(start, msg, reinterpret_cast<uint16_t&>(key), val);
        dict_[key]=std::move(val);
    }

    assert(count==dict_.size());
}

void MessageBase::reset()
{
    cmd_=0;
    dict_.clear();
}

}

