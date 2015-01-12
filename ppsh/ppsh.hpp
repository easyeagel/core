//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  ppsh.hpp
//
//   Description:  C++ 嵌入的 类Shell 语言
//
//       Version:  1.0
//       Created:  2013年11月06日 21时26分32秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================


#ifndef EZBTY_ORG_PPSH_HPP
#define EZBTY_ORG_PPSH_HPP

#include<fcntl.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<sys/types.h>


#include<cassert>

#include<map>
#include<vector>
#include<memory>
#include<iostream>
#include<functional>

#include<boost/filesystem.hpp>
#include<boost/algorithm/string/predicate.hpp>

#include"ppsh/proc.hpp"

namespace ppsh
{

class OSPipe
{
    enum {eInvalid=-1};
public:
    OSPipe()
        :fds_{eInvalid, eInvalid}
    {
        ::pipe(fds_);
    }

    void readClose()
    {
        ::close(readGet());
        fds_[0]=eInvalid;
    }

    void writeClose()
    {
        ::close(writeGet());
        fds_[1]=eInvalid;
    }

    void close()
    {
        readClose();
        writeClose();
    }

    bool good() const
    {
        return fds_[0]!=eInvalid && fds_[1]!=eInvalid;
    }

    int readGet() const
    {
        return fds_[0];
    }

    int writeGet() const
    {
        return fds_[1];
    }

private:
    int fds_[2];
};

static inline void stdinReset(int fd)
{
    int ret=::dup2(fd, STDIN_FILENO);
    assert(ret!=-1);
    static_cast<void>(ret);
}

static inline void stdoutReset(int fd)
{
    int ret=::dup2(fd, STDOUT_FILENO);
    assert(ret!=-1);
    static_cast<void>(ret);
}

static inline void stdinReset(const std::string& file)
{
    if(file.empty())
    {
        std::cerr << "stdinReset: file Must be specify" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    int fd=::open(file.c_str(), O_RDONLY);
    if(fd<0)
    {
        std::perror("stdinReset: ");
        std::exit(EXIT_FAILURE);
    }
    stdinReset(fd);
}

static inline void stdoutReset(const std::string& file)
{
    if(file.empty())
    {
        std::cerr << "stdoutReset: file Must be specify" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    int fd=::open(file.c_str(), O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if(fd<0)
    {
        std::perror("stdoutReset: ");
        std::exit(EXIT_FAILURE);
    }

    stdoutReset(fd);
}

enum class ExecuterEnum
{
    eNone,
    eCommand,
    ePipeLine,
    eSucessAfter,
};

namespace details
{
    template<ExecuterEnum EE>
    struct EEName;

    template<>
    struct EEName<ExecuterEnum::eCommand>
    {
        static constexpr const char* name()
        {
            return "cmd";
        }
    };

    template<>
    struct EEName<ExecuterEnum::ePipeLine>
    {
        static constexpr const char* name()
        {
            return " | ";
        }
    };

    template<>
    struct EEName<ExecuterEnum::eSucessAfter>
    {
        static constexpr const char* name()
        {
            return " && ";
        }
    };
}

typedef std::map<std::string, std::string> OptDict;

struct Option
{
    std::string name;
    std::string value;

    Option()=default;

    Option(const char* n, const char* v)
        :name(n), value(v)
    {}

    Option(const std::string& n, const std::string& v)
        :name(n), value(v)
    {}

    Option(const std::string& v)
        :value(v)
    {}

    Option(const char* v)
        :value(v)
    {}

};

typedef std::reference_wrapper<const Option> OptionRef;

struct ChildNotWait {};
static const ChildNotWait childNoWait={};

template<typename Object>
class ExecuterT
{
    typedef std::function<void(Object& obj)> ForkCall;

protected:
    Object& objGet()
    {
        return static_cast<Object&>(*this);
    }

    const Object& objGet() const
    {
        return static_cast<const Object&>(*this);
    }
public:
    typedef Object ObjectType;

    int operator()()
    {
        auto& obj=objGet();
        obj.execute();
        return obj.childWait();
    }

    int operator()(const OptDict* dict)
    {
        auto& obj=objGet();
        obj.optDictSet(dict);
        obj.execute();
        return obj.childWait();
    }

    int operator()(ChildNotWait)
    {
        auto& obj=objGet();
        return obj.execute();
    }

    int operator()(const OptDict* dict, ChildNotWait)
    {
        auto& obj=objGet();
        obj.optDictSet(dict);
        return obj.execute();
    }

    ~ExecuterT(){}

    ExecuterT()=default;
    ExecuterT(const ExecuterT&)=default;

    ExecuterT(ExecuterT&& exe)
        :childCalls_(std::move(exe.childCalls_))
        ,parentCalls_(std::move(exe.parentCalls_))
    {}

    void optDictSet(const OptDict* dict)
    {
        optDict_=dict;
    }

    void childCallPush(ForkCall&& call)
    {
        childCalls_.emplace_back(std::move(call));
    }

    void parentCallPush(ForkCall&& call)
    {
        parentCalls_.emplace_back(std::move(call));
    }

    const std::string& optValueGet(const Option& opt)
    {
        if(opt.name.empty() || optDict_==nullptr)
            return opt.value;

        auto itr=optDict_->find(opt.name);
        if(itr==optDict_->end())
            return opt.value;

        return itr->second;
    }

protected:
    const OptDict* optDict_=nullptr;
    std::vector<ForkCall> childCalls_;
    std::vector<ForkCall> parentCalls_;
};

class Path
{
    Path()
    {
        const char* const path=std::getenv("PATH");
        if(path==nullptr)
            return;

        const char *s, *e;
        for(s=path, e=path; *e; ++e)
        {
            if(*e!=':')
                continue;
            paths_.push_back(std::string(s, static_cast<size_t>(e-s)));
            s=e+1;
        }
    }

public:
    static Path& instance()
    {
        static Path gs;
        return gs;
    }

    static void push(const std::string& path)
    {
        instance().paths_.push_back(path);
    }

    static std::string find(const std::string& file)
    {
        for(const auto& p: instance().paths_)
        {
            std::string tmp=p+'/'+file;
            if(boost::filesystem::is_regular_file(tmp))
                return std::move(tmp);
        }

        return std::string();
    }

private:
    std::vector<std::string> paths_;
};

static inline void globalInit()
{
    Path::instance();
}

class Command: public ExecuterT<Command>
{
public:
    Command(const std::string& cmd)
        :cmd_(cmd)
    {}

    static std::string name()
    {
        return "cmd";
    }

    static const ExecuterEnum eEnumValue=ExecuterEnum::eCommand;

    int execute()
    {
        auto pid=::fork();
        switch(pid)
        {
            case 0:
            {
                for(auto& call: childCalls_)
                    call(*this);
                this->childCalls_.clear();
                startProgram();
            }
            case -1:
                return 1;
            default:
            {
                child_=pid;
                for(auto& call: parentCalls_)
                    call(*this);
                return 0;
            }
        }
    }

    Command& leftGet()
    {
        return *this;
    }

    Command& rigthGet()
    {
        return *this;
    }

    const Command& leftGet() const
    {
        return *this;
    }

    const Command& rigthGet() const
    {
        return *this;
    }

    int childWait() const
    {
        int st=0;
        ::waitpid(child_, &st, 0);
        return st;
    }

    Command& operator+(const Option& opt)
    {
        cmdIndex_.push_back(static_cast<int>(opts_.size()));
        opts_.push_back(opt);
        return *this;
    }

    Command& operator+(const OptionRef& ref)
    {
        refs_.push_back(ref);
        cmdIndex_.push_back(-static_cast<int>(refs_.size()));
        return *this;
    }

    Command operator>(const OptionRef& ref) const
    {
        Command tmp(*this);

        tmp.childCallPush(
            [ref](Command& obj)
            {
                stdoutReset(obj.optValueGet(ref.get()));
            }
        );

        return std::move(tmp);
    }

    Command operator<(const OptionRef& ref) const
    {
        Command tmp(*this);

        tmp.childCallPush(
            [ref](Command& obj)
            {
                stdinReset(obj.optValueGet(ref.get()));
            }
        );

        return std::move(tmp);
    }

    Command operator>(const Option& opt) const
    {
        Command tmp(*this);

        tmp.childCallPush(
            [opt](Command& obj)
            {
                stdoutReset(obj.optValueGet(opt));
            }
        );

        return std::move(tmp);
    }

    Command operator<(const Option& opt) const
    {
        Command tmp(*this);

        tmp.childCallPush(
            [opt](Command& obj)
            {
                stdinReset(obj.optValueGet(opt));
            }
        );

        return std::move(tmp);
    }

    void reset()
    {
        child_=-1;
    }

    int childGet() const
    {
        return child_;
    }

    const std::string& cmdGet() const
    {
        return cmd_;
    }

private:
    void startProgram()
    {
        auto exe=exeFind();
        auto vect=optPrepare();

        ::execvp(exe.c_str(), vect.data());

        std::perror("execvp");
        ::std::exit(EXIT_FAILURE);
    }

    std::string exeFind()
    {
        std::string file=boost::starts_with(cmd_, "./") ? cmd_ : Path::find(cmd_);
        if(file.empty() || !boost::filesystem::is_regular_file(file))
        {
            std::cerr << "File Not Exist: " << cmd_ << std::endl;
            std::cerr.flush();
            std::exit(EXIT_FAILURE);
        }

        auto fs=boost::filesystem::status(file);
        auto per=fs.permissions();
        if(0==(per & boost::filesystem::owner_exe))
        {
            std::cerr << "Can Not Execute: " << cmd_ << std::endl;
            std::cerr.flush();
            std::exit(EXIT_FAILURE);
        }

        return std::move(file);
    }

    std::vector<char*> optPrepare()
    {
        std::vector<char*> vect;
        auto vpush=[&](const std::string& str)
        {
            vect.push_back(const_cast<char*>(str.data()));
        };

        vpush(cmd_);

        for(auto idx: cmdIndex_)
        {
            const Option& opt=optGet(idx);
            const std::string& value=optValueGet(opt);
            vpush(value);
        }

        vect.push_back(nullptr);

        return std::move(vect);
    }

    const Option& optGet(int idx)
    {
        if(idx>=0)
            return opts_[static_cast<size_t>(idx)];
        const size_t tmp=static_cast<size_t>(-idx-1);
        return refs_[tmp].get();
    }

private:
    int child_=-1;
    std::string cmd_;
    std::vector<OptionRef> refs_;
    std::vector<int> cmdIndex_;
    std::vector<Option> opts_;
};

static inline Command cmd(const char* cmd)
{
    return Command(cmd);
}

static inline Command cmd(const std::string& cmd)
{
    return Command(cmd);
}

namespace details
{

template<typename Object, typename Left, typename Right, ExecuterEnum EE>
class DoubleComandT: public ExecuterT<Object>
{
public:
    static const ExecuterEnum eEnumValue=EE;

    typedef Left LeftType;
    typedef Right RightType;

    static std::string name()
    {
        return '(' + LeftType::name() + details::EEName<EE>::name() + RightType::name() + ')';
    }

    DoubleComandT(LeftType&& left, RightType&& right)
        :left_(std::move(left)), right_(std::move(right))
    {}

    LeftType& leftGet()
    {
        return left_;
    }

    RightType& rightGet()
    {
        return right_;
    }

    const LeftType& leftGet() const
    {
        return left_;
    }

    const RightType& rightGet() const
    {
        return right_;
    }

    void optDictSet(const OptDict* dict)
    {
        left_.optDictSet(dict);
        right_.optDictSet(dict);
    }

    int childWait() const
    {
        this->leftGet().childWait();
        return this->rightGet().childWait();
    }

    void reset()
    {
        leftWait_=false;
        rightWait_=false;
        left_.reset();
        right_.reset();
    }

    Object operator>(const OptionRef& ref) const
    {
        Object tmp(this->objGet());
        auto& right=tmp.rightGet();
        right.childCallPush(
            [ref](Object& obj)
            {
                stdoutReset(obj.optValueGet(ref.get()));
            }
        );
        return std::move(tmp);
    }

    Object operator<(const OptionRef& ref) const
    {
        Object tmp(this->objGet());
        auto& right=tmp.rightGet();
        right.childCallPush(
            [ref](Object& obj)
            {
                stdinReset(obj.optValueGet(ref.get()));
            }
        );
        return std::move(tmp);
    }

    Object operator>(const Option& opt) const
    {
        Object tmp(this->objGet());
        auto& right=tmp.rightGet();
        right.childCallPush(
            [opt](typename RightType::ObjectType& obj)
            {
                stdoutReset(obj.optValueGet(opt));
            }
        );
        return std::move(tmp);
    }

    Object operator<(const Option& opt) const
    {
        Object tmp(this->objGet());
        auto& right=tmp.rightGet();
        right.childCallPush(
            [opt](typename RightType::ObjectType& obj)
            {
                stdinReset(obj.optValueGet(opt));
            }
        );
        return std::move(tmp);
    }

protected:
    bool leftWait_=false;
    bool rightWait_=false;
    LeftType left_;
    RightType right_;
};
}

template<typename Left, typename Right>
class CmdPipeLineT
    : public details::DoubleComandT<CmdPipeLineT<Left, Right>, Left, Right, ExecuterEnum::ePipeLine>
{
    typedef details::DoubleComandT<CmdPipeLineT, Left, Right, ExecuterEnum::ePipeLine> BaseThis;
public:
    using BaseThis::BaseThis;

    int execute()
    {
        OSPipe pipe;
        assert(pipe.good());

        auto& left=this->leftGet();
        auto& right=this->rightGet();

        left.childCallPush(
            [=](typename Left::ObjectType&) mutable
            {
                stdoutReset(pipe.writeGet());
                pipe.close();
            }
        );

        right.childCallPush(
            [=](typename Right::ObjectType&) mutable
            {
                for(auto& call: this->childCalls_)
                    call(*this);
                this->childCalls_.clear();
                stdinReset(pipe.readGet());
                pipe.close();
            }
        );

        auto ret=left.execute();
        if(ret)
        {
            pipe.close();
            return ret;
        }

        ret=right.execute();
        pipe.close();
        return ret;
    }

};

template<typename Left, typename Right>
static inline CmdPipeLineT<Left, Right> operator|(Left left, Right right)
{
    return CmdPipeLineT<Left, Right>(std::move(left), std::move(right));
}

template<typename Left, typename Right>
class CmdSucessAfterT
    : public details::DoubleComandT<CmdSucessAfterT<Left, Right>, Left, Right, ExecuterEnum::eSucessAfter>
{
    typedef details::DoubleComandT<CmdSucessAfterT, Left, Right, ExecuterEnum::eSucessAfter> BaseThis;
public:
    using BaseThis::BaseThis;

    int execute()
    {
        reset();

        auto& left=this->leftGet();
        status_=left.execute();
        if(status_)
            return status_;

        status_=left.childWait();
        if(status_)
            return status_;

        auto& right=this->rightGet();
        status_=right.execute();
        return status_;
    }

    int childWait() const
    {
        if(status_)
            return status_;
        return this->rightGet().childWait();
    }

    void reset()
    {
        status_=0;
        BaseThis::reset();
    }

private:
    int status_=0;
};

template<typename Left, typename Right>
static inline CmdSucessAfterT<Left, Right> operator&&(Left left, Right right)
{
    static_assert(static_cast<bool>(Left::eEnumValue) && static_cast<bool>(Right::eEnumValue)
        , "Left and Right Must be Callable");
    return CmdSucessAfterT<Left, Right>(std::move(left), std::move(right));
}


}

#endif //EZBTY_ORG_PPSH_HPP

