//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  pattern.hpp
//
//   Description:  通用解析装置
//
//       Version:  1.0
//       Created:  2015年01月09日 10时17分31秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#pragma once

#include<stack>
#include<boost/variant.hpp>
#include<boost/xpressive/xpressive.hpp>
#include<boost/algorithm/string/trim.hpp>
#include<boost/algorithm/string/split.hpp>

#include"error.hpp"

namespace core
{

namespace xpr
{
    using namespace boost::xpressive;
    
    //变量名或宏名
    extern sregex gsVarName;
    extern sregex gsMacroName;
}

class ReplacePattern
{
public:
    enum Type
    {
        eList,
        eValue,
        eLiteral,
    };

    struct Param
    {
        Type type;
        std::string value;
    };

    struct Operator
    {
        std::string name;
        std::vector<Param> params;
    };

    void init(const std::vector<std::string>& what);

    const std::vector<Operator>& operatorsGet() const
    {
        return operators_;
    }

    bool needSplit() const
    {
        return needSplit_;
    }

    void reset()
    {
        needSplit_=false;
        operators_.clear();
    }

    void needSplitSet(bool v=true)
    {
        needSplit_=v;
    }

private:
    bool needSplit_=false;
    std::vector<Operator> operators_;
};

class ReplaceNode
{
public:
    enum Type
    {
        eNode,
        eList,
        eValue,
        eLiteral,
    };

    typedef boost::variant<std::string, std::shared_ptr<ReplaceNode>> ParamValue;
    struct Param
    {
        Type type;
        ParamValue value;
    };

    template<typename Itr>
    void complie(ErrorCode& ec, Itr b, Itr const e)
    {
        assert(*b=='$' && *(b+1)=='{');

        //${~@opt}
        for(;;)
        {
            switch(*b)
            {

            }
        }
    }

    struct Operator
    {
        std::string name;
        std::vector<Param> params;
    };

    const std::vector<Operator>& operatorsGet() const
    {
        return operators_;
    }

    void reset()
    {
        modifier_=0;
        operators_.clear();
    }

    int modifierGet() const
    {
        return modifier_;
    }

private:
    int modifier_=0;
    std::vector<Operator> operators_;
};

class ReplaceBlock
{
    class Unit: boost::variant<std::string, ReplaceNode>
    {
        typedef boost::variant<std::string, ReplaceNode> BaseThis;
    public:
        template<typename... Args>
        Unit(Args&&... args)
            :BaseThis(std::forward<Args&&>(args)...)
        {}
    };
public:

private:
    std::vector<Unit> units_;
};

//实现一个递归的模式替换过程
class PatternReplace
{
    enum
    {
        eMaxPatternLength=4*1024,
        eMaxRecursiveLevel=1024,
    };

    struct Unit
    {
        std::string source;
        ReplacePattern pattern;
    };
public:
    template<typename OutItr, typename InItr, typename Format>
    OutItr replace(ErrorCode& ec, OutItr out, InItr b, InItr const e, Format&& format)
    {
        for(;;)
        {
            if(b==e)
                return out;

            switch(*b)
            {
                case '!': //只在 !! 或 !$ 解析转义
                {
                    ++b;
                    if(b==e)
                    {
                        CoreError::ecMake(CoreError::ePatternReplaceFailed);
                        return out;
                    }

                    const auto n=*b;
                    if(n=='!' || n=='$')
                    {
                        *out++=n;
                    } else {
                        *out++='!';
                        *out++=n;
                    }

                    ++b;
                    continue;
                }
                case '$': //模式开头 ${
                {
                    dollarStart(ec, out, b, e, format);
                    if(ec.bad())
                        return out;
                    break;
                }
                default:
                {
                    *out++ = *b++;
                    break;
                }
            }
        }

        return out;
    }

private:
    template<typename OutItr, typename InItr, typename Format>
    void dollarStart(ErrorCode& ec, OutItr& out, InItr& b, InItr const& e, Format& format)
    {
        ++b;
        if(b==e)
        {
            CoreError::ecMake(CoreError::ePatternReplaceFailed);
            return;
        }

        const auto n=*b;
        if(n!='{')
        {
            *out++='$';
            *out++=n;
            ++b;
            return;
        }

        stack_.push(Unit());
        auto& u=stack_.top();

        if(stack_.size()>=eMaxRecursiveLevel)
        {
            CoreError::ecMake(CoreError::ePatternReplaceFailed);
            return;
        }

        u.source.clear();
        u.source.push_back('$');
        for(;;)
        { //找到最后的 }
            if(b==e || u.source.size()>=eMaxPatternLength)
            {
                CoreError::ecMake(CoreError::ePatternReplaceFailed);
                return;
            }
            
            switch(*b)
            {
                case '$': //模式开头 ${
                {
                    auto o=std::back_inserter(u.source);
                    dollarStart(ec, o, b, e, format);
                    if(ec.bad())
                        return;
                    break;
                }
                case '}':
                {
                    u.source.push_back(*b);

                    ++b;
                    if(b!=e && *b=='~')
                    {
                        u.source.push_back(*b);
                        ++b;
                    }

                    split(ec, u.source, u.pattern);
                    if(ec.bad())
                        return;

                    out=format(ec, out, u.pattern);
                    if(ec.bad())
                        return;

                    //当前模式完成
                    stack_.pop();
                    return;
                }
                default:
                {
                    u.source.push_back(*b++);
                    break;
                }
            }

        }
    }

    void split(ErrorCode& ec, const std::string& source, ReplacePattern& dest);

private:
    std::stack<Unit> stack_;
};

template<typename Out>
static inline void simpleSplit(const std::string& str, Out&& out, char del='=')
{
    std::vector<std::string> tmp;
    boost::algorithm::split(tmp, str, [del](char c){return c==del; });
    for(auto& t: tmp)
        out(std::move(t));
}

static inline std::pair<std::string, std::string> simpleSplit(const std::string& str, char del='=')
{
    std::string key, val;
    auto const n=str.find(del);
    if(n==std::string::npos)
    {
        key=str;
    } else {
        key=str.substr(0, n);
        val=str.substr(n+1);
    }

    boost::algorithm::trim(key);
    boost::algorithm::trim(val);

    return std::make_pair(key, val);
}

namespace test
{
    void patternReplaceTest();
}

}

