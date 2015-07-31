//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  mutilpart.hpp
//
//   Description:  mutilpart/form-data 分析器与生成器
//
//       Version:  1.0
//       Created:  2015年07月29日 15时09分48秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#pragma once

#include<map>
#include<functional>
#include<core/error.hpp>

namespace core
{

class HttpParser;
class MutilpartData
{
public:
    enum Status
    {
        eStatusStart,
        eStatusContinue,
        eStatusCompelete
    };

    enum PStatus
    {
        ePStatusNewLine1,
        ePStatusNewLine2,
        ePStatusCache,
        ePStatusMatchPrefix1,
        ePStatusMatchPrefix2,
        ePStatusMatch,
        ePStatusMatchFailed,
        ePStatusComplete,
    };

    struct PartTrait
    {
        Status stat;
        std::string name;
        std::string filename;
        std::map<std::string, std::string> infos;
    };

    typedef std::function<void (const PartTrait& trait, const Byte* bt, size_t size)> PartCallBack;

    template<typename Call>
    void partCallSet(Call&& call)
    {
        partCall_=std::move(call);
    }

    void boundarySet(const std::string& b)
    {
        boundary_ =  "\r\n--";
        boundary_ += b;
    }

    void parse(ErrorCode& ec, const Byte* bt, size_t size);

    bool isCompleted() const
    {
        return pstatus_==ePStatusComplete;
    }

    static std::string headCheck(ErrorCode& ecRet, const HttpParser& hp);

private:
    void headParse();
    const Byte* nextLine(const Byte* bt, const Byte* end);
    void infoParse();
    void contentDispositionParse(const std::string& line);
    void kvCheck(std::string& key, std::string& val);

private:
    PStatus pstatus_=ePStatusCache;
    std::string boundaryCache_="\r\n";
    std::string boundary_;

    std::vector<std::string> infoLines_;

    PartTrait trait_;
    PartCallBack partCall_;
};

}

