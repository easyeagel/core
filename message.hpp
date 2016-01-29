
#pragma once

#include<cstdint>

#include<map>
#include<functional>

#include"value.hpp"

namespace core
{

class MessageBase
{
public:
    typedef std::function<const uint8_t*()> AESKeyGet;

    void encode(std::string& msg) const;
    void decode(uint32_t& start, const std::string& msg);
    void reset();

    bool empty() const
    {
        return cmd_==0;
    }

    bool operator==(const MessageBase& o) const
    {
        return cmd_==o.cmd_ && dict_==o.dict_;
    }

protected:
    void cmdSet(const uint16_t& cmd)
    {
        cmd_=cmd;
    }
    
    const uint16_t& cmdGet() const
    {
        return cmd_;
    }

    void set(const uint16_t& key, const SimpleValue& val)
    {
        dict_[key]=val;
    }

    const SimpleValue& get(const uint16_t& key) const
    {
        auto itr=dict_.find(key);
        if(itr==dict_.end())
            return SimpleValue::null();
        return itr->second;
    }

private:
    const uint8_t* keyGet() const;

private:
    uint16_t cmd_=0;
    std::map<uint16_t, SimpleValue> dict_;

    static AESKeyGet keyGet_;
};

template<typename MCmd, typename MKey>
class MessageT: public MessageBase
{
public:
    void cmdSet(const MCmd& cmd)
    {
        MessageBase::cmdSet(static_cast<uint16_t>(cmd));
    }
    
    const MCmd& cmdGet() const
    {
        return static_cast<MCmd>(MessageBase::cmdGet());
    }

    void set(const MKey& key, const SimpleValue& val)
    {
        MessageBase::set(static_cast<uint16_t>(key), val);
    }

    const SimpleValue& get(const MKey& key) const
    {
        return MessageBase::get(static_cast<uint16_t>(key));
    }

    MessageBase& baseGet()
    {
        return *this;
    }

    const MessageBase& baseGet() const
    {
        return *this;
    }
};

}

