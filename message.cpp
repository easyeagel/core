
#include<boost/crc.hpp>

#include"codec.hpp"
#include"tinyaes.hpp"
#include"message.hpp"

namespace core
{


MessageBase::AESKeyGet MessageBase::keyGet_;

void MessageBase::encode(std::string& dest) const
{
    std::string msg;
    core::encode(msg, static_cast<uint16_t>(cmd_));
    core::encode(msg, static_cast<uint32_t>(dict_.size()));
    for(auto& kv: dict_)
        core::encode(msg, static_cast<uint16_t>(kv.first), kv.second);

    //加入crc效验
    boost::crc_32_type crc;
    crc.process_bytes(msg.data(), msg.size());
    core::encode(msg, crc.checksum());

    core::TinyAES128 aes;
    const auto csz=dest.size();
    const auto sz=aes.encryptSizeGet(msg.size());
    dest.resize(csz+sz);

    aes.encrypt(keyGet(), msg.data(), msg.size(),
        reinterpret_cast<uint8_t*>(const_cast<char*>(dest.data()))+csz);
}

void MessageBase::decode(uint32_t& start, const std::string& src)
{
    std::string msg;
    core::TinyAES128 aes;
    const auto sz=aes.decryptSizeGet(src.data()+start);
    if(sz>src.size()) //错误
        return;

    msg.resize(sz);

    aes.decrypt(keyGet(),
        src.data()+start,
        const_cast<char*>(msg.data())
    );

    start += sz;

    //crc效验
    uint32_t crcIndex=msg.size()-sizeof(boost::crc_32_type::value_type);
    boost::crc_32_type::value_type crcValue=0;
    core::decode(crcIndex, msg, crcValue);

    boost::crc_32_type crc;
    crc.process_bytes(msg.data(), crcIndex-sizeof(crcValue));
    if(crcValue!=crc.checksum()) //效验失败
        return;

    uint32_t msgIndex=0;
    core::decode(msgIndex, msg, reinterpret_cast<uint16_t&>(cmd_));
    uint32_t count=0;
    core::decode(msgIndex, msg, count);
    for(uint32_t i=0; i<count && msgIndex<msg.size(); ++i)
    {
        uint16_t key;
        SimpleValue val;
        core::decode(msgIndex, msg, reinterpret_cast<uint16_t&>(key), val);
        dict_[key]=std::move(val);
    }

    assert(count==dict_.size());
}

const uint8_t* MessageBase::keyGet() const
{
    if(keyGet_)
        return keyGet_();
    return reinterpret_cast<const uint8_t*>("2w7e9516283ed2a6acf71j8809cf4c3c");
}

void MessageBase::reset()
{
    cmd_=0;
    dict_.clear();
}

}

