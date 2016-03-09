//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  mutilpart.cpp
//
//   Description:  
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

#include<algorithm>
#include<boost/algorithm/string/trim.hpp>
#include<boost/algorithm/string/split.hpp>

#include<core/http/http.hpp>
#include<core/http/mutilpart.hpp>

namespace core
{

void MutilpartData::parse(ErrorCode& , const Byte* bt, size_t size)
{
    for(auto const end=bt+size; bt<end;)
    {
        switch(pstatus_)
        {
            case ePStatusNewLine1:
            {
                boundaryCache_.push_back(*bt);
                if(*bt=='\r')
                    pstatus_=ePStatusNewLine2;
                else
                    pstatus_=ePStatusMatchFailed;
                ++bt;
                break;
            }
            case ePStatusNewLine2:
            {
                if(*bt=='\n')
                {
                    boundaryCache_.push_back(*bt);
                    pstatus_=ePStatusCache;
                    ++bt;
                }
                else
                    pstatus_=ePStatusMatchFailed;
                break;
            }
            case ePStatusCache:
            {
                boundaryCache_.push_back(*bt);
                if(boundaryCache_.size()==boundary_.size())
                {
                    if(boundary_.back()==boundaryCache_.back())
                    {
                        pstatus_=ePStatusMatchPrefix1;
                        ++bt;
                    } else {
                        boundaryCache_.pop_back();
                        pstatus_=ePStatusMatchFailed;
                    }
                } else {
                    if(boundary_[boundaryCache_.size()-1]!=boundaryCache_.back())
                    {
                        boundaryCache_.pop_back();
                        pstatus_=ePStatusMatchFailed;
                    } else {
                        ++bt;
                    }
                }

                break;
            }
            case ePStatusMatchPrefix1:
            {
                boundaryCache_.push_back(*bt);
                if(*bt=='\r' || *bt=='-')
                {
                    ++bt;
                    pstatus_=ePStatusMatchPrefix2;
                } else {
                    pstatus_=ePStatusMatchFailed;
                }
                break;
            }
            case ePStatusMatchPrefix2:
            {
                if(*bt=='\n')
                {
                    ++bt;
                    boundaryCache_.pop_back();
                    pstatus_=ePStatusMatch;
                    infoLines_.clear();
                    infoLines_.emplace_back(std::string());
                } else if(*bt=='-' && boundaryCache_.back()=='-') {
                    pstatus_=ePStatusComplete;
                } else {
                    pstatus_=ePStatusMatchFailed;
                }
                break;
            }
            case ePStatusMatch:
            {
                auto& line=infoLines_.back();
                if(*bt!='\n')
                {
                    line.push_back(*bt);
                    ++bt;
                } else {
                    if(line.back()=='\r')
                    {
                        ++bt;
                        line.pop_back();
                        if(line.empty()) //匹配完成
                        {
                            infoParse();
                            trait_.stat=eStatusStart;
                            partCall_(trait_, nullptr, 0);

                            boundaryCache_.clear();
                            pstatus_=ePStatusMatchFailed;
                        } else {
                            infoLines_.emplace_back(std::string());
                        }
                    } else {
                        line.push_back(*bt);
                        ++bt;
                    }
                }
                break;
            }
            case ePStatusMatchFailed:
            {
                trait_.stat=eStatusContinue;

                if(!boundaryCache_.empty())
                {
                    partCall_(trait_, reinterpret_cast<const Byte*>(boundaryCache_.data()), boundaryCache_.size());
                    boundaryCache_.clear();
                }

                auto const n=nextLine(bt, end);
                if(n>bt)
                    partCall_(trait_, bt, n-bt);

                bt=n;
                if(bt<end)
                    pstatus_=ePStatusNewLine1;
                break;
            }
            case ePStatusComplete:
            {
                trait_.stat=eStatusCompelete;
                partCall_(trait_, nullptr, 0);
                return;
            }
        }
    }
}

const Byte* MutilpartData::nextLine(const Byte* bt, const Byte* end)
{
    for(;bt<end; ++bt)
    {
        if(*bt=='\r')
            return bt;
    }

    return end;
}

void MutilpartData::infoParse()
{
    for(auto& line: infoLines_)
    {
        std::vector<std::string> dest;
        boost::algorithm::split(dest, line, boost::algorithm::is_any_of(":"));
        if(dest.size()!=2)
            continue;

        boost::algorithm::trim(dest[0]);
        boost::algorithm::trim(dest[1]);
        if(dest[0]=="Content-Disposition")
            contentDispositionParse(dest[1]);
    }
}

void MutilpartData::contentDispositionParse(const std::string& line)
{
    bool in=false, es=false;
    std::string key, val;
    std::string* ptr=&key;
    for(const auto c: line)
    {
        switch(c)
        {
            case ';':
            {
                if(in==true)
                {
                    ptr->push_back(c);
                    break;
                }

                kvCheck(key, val);

                key.clear();
                val.clear();
                ptr=&key;
                break;
            }
            case '"':
            {
                if(in==true)
                {
                    if(es==true)
                    {
                        ptr->push_back(c);
                        break;
                    }

                    in=false;
                    break;
                }

                in=true;
                break;
            }
            case '=':
            {
                if(in==true)
                {
                    ptr->push_back(c);
                    break;
                }

                ptr=&val;
                break;
            }
            case '\\':
            {
                if(es==false)
                    es=true;
                else
                {
                    ptr->push_back(c);
                    es=false;
                }
                break;
            }
            default:
            {
                ptr->push_back(c);
                break;
            }
        }
    }

    kvCheck(key, val);
}

void MutilpartData::kvCheck(std::string& key, std::string& val)
{
    boost::algorithm::trim(key);
    boost::algorithm::trim(val);
    if(key=="filename")
    {
        trait_.filename=val;
    } else if(key=="name") {
        trait_.name=val;
    }
}

std::string MutilpartData::headCheck(ErrorCode& ecRet, const HttpParser& hp)
{
    const auto& h=hp.headGet();
    auto itr=h.find("Content-Type");
    if(itr==h.end())
    {
        ecRet=CoreError::ecMake(CoreError::eNetProtocolError, "http content-type error");
        return std::string();
    }

    const auto& type=itr->second;
    auto const pos=type.find("boundary=");
    if(pos==std::string::npos)
    {
        ecRet=CoreError::ecMake(CoreError::eNetProtocolError, "http content-type error");
        return std::string();
    }

    return type.substr(pos+9);
}

}

