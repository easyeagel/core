//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  time.cpp
//
//   Description:  时间操作
//
//       Version:  1.0
//       Created:  2013年11月05日 15时45分34秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================


#include<ctime>
#include<atomic>
#include<iomanip>

#include<boost/xpressive/xpressive.hpp>
#include<boost/xpressive/regex_actions.hpp>
#include<boost/date_time/local_time/local_time.hpp>
#include<boost/date_time/posix_time/posix_time.hpp>

#include"time.hpp"
#include"string.hpp"
#include"server.hpp"

namespace bp = boost::posix_time;
namespace bg = boost::gregorian;

namespace core
{

std::time_t LocalTime::offset()
{
    struct Init
    {
        Init() :offset(8*60*60)
        {
            std::time_t t=0;
#ifndef _MSC_VER
            std::tm tm;
            ::gmtime_r(&t, &tm);
#else
            std::tm tm= *std::gmtime(&t);
#endif
            offset=-std::mktime(&tm);
        }

        std::time_t offset;
    };

    static OnceConstructT<Init> gs;
    return gs.get().offset;
}

LocalTime::LocalTime(std::time_t tm)
    :time_(boost::posix_time::from_time_t(tm+offset()))
{}

std::time_t LocalTime::ctimeGet() const
{
    PTime epoch(bg::date(1970,1,1));
    bp::time_duration::sec_type x = (time_-epoch).total_seconds();
    return std::time_t(x)-offset();
}

LocalTime::PTime LocalTime::utcTimeGet() const
{
    const auto off=offset();
    if(off>0)
        return time_-bp::seconds(off);
    return time_ + bp::seconds(-off);
}

LocalTime::SplitedTime LocalTime::splitedTimeGet() const
{
    const bg::date dt(time_.date());
    const bp::time_duration td = time_.time_of_day();

    return SplitedTime
    {
        static_cast<uint32_t>(dt.year()),
        static_cast<uint32_t>(dt.month() - 1),
        static_cast<uint32_t>(dt.day()   - 1),
        static_cast<uint32_t>(td.hours()),
        static_cast<uint32_t>(td.minutes()),
        static_cast<uint32_t>(td.seconds()),
        static_cast<uint32_t>(dt.day_of_week())
    };
}

std::time_t LocalTime::toCTime(const SplitedTime& st)
{
    bp::ptime pt(bg::date(st[0], st[1]+1, st[2]+1), bp::time_duration(st[3], st[4], st[5]));
    LocalTime tmp;
    tmp.time_=pt;
    return tmp.ctimeGet();
}

std::time_t LocalTime::timeMake(int year, int month, int day, int hour, int minute, int second)
{
    bp::ptime pt(bp::ptime(bg::date(year,month,day)
                ,bp::time_duration(hour,minute,second)));
    LocalTime tmp;
    tmp.time_=pt;
    return tmp.ctimeGet();
}

LocalTime::PTime LocalTime::toLocalTime(const PTime& pt)
{
    const auto off=offset();
    if(off>0)
        return pt + bp::seconds(off);
    return pt - bp::seconds(-off);
}


#define MacroNowPath "%Y%m%d%H%M%S"

std::time_t secondNow()
{
    return NowKit::secondGet();
}

std::string nowString()
{
    return timeString(secondNow());
}

std::string timeString(std::time_t t)
{
    //2013-11-04_17:34:35
#ifndef _MSC_VER
    std::tm tm;
    ::localtime_r(&t, &tm);
#else
    std::tm tm = *std::localtime(&t);
#endif

    char buf[24];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d_%H:%M:%S", &tm);

    return buf;
}

std::string nowPathString()
{
    //2013-11-04_17-34-35
    auto t=secondNow();
#ifndef _MSC_VER
    std::tm tm;
    ::localtime_r(&t, &tm);
#else
    std::tm tm = *std::localtime(&t);
#endif

    char buf[24];
    std::strftime(buf, sizeof(buf), MacroNowPath, &tm);

    return buf;
}

std::string nowStringForestall()
{
    return timeString(std::time(nullptr));
}

std::string nowPathStringForestall()
{
    //2013-11-04_17-34-35
    auto t=std::time(nullptr);
#ifndef _MSC_VER
    std::tm tm;
    ::localtime_r(&t, &tm);
#else
    std::tm tm = *std::localtime(&t);
#endif

    char buf[24];
    std::strftime(buf, sizeof(buf), MacroNowPath, &tm);

    return buf;
}

#undef MacroNowPath




//======================================================================
IOTimer::IOTimer(size_t milli)
    : timer_(IOServer::serviceFetchOne().castGet())
    , time_(milli)
{}

IOTimer::IOTimer(IOService& service, size_t milli)
    : timer_(service.castGet())
    , time_(milli)
{}

void IOTimer::timerStart(TimerCallBack&& handle)
{
    call_=std::move(handle);
    stoped_=false;

    IOServer::stopCallPush(
        [this](IOServer::CallFun&& fun)
        {
            stopCall_=std::move(fun);
            timerStop();
        }
    );

    timerStart();
}

void IOTimer::timerCall(const boost::system::error_code& ec)
{
    //如果出错，则必定是主动取消的错误
    assert(ec ? (ec.value()==boost::asio::error::operation_aborted && stoped_) : true);

    {
        Spinlock::ScopedLock lock(mutex_);
        if(stoped_)
            return stopCall_();
    }

    call_(ec ? CoreError::ecMake(CoreError::eTimerStopping) : CoreError::ecMake(CoreError::eGood));
    timerStart();
}

void IOTimer::timerStop()
{
    Spinlock::ScopedLock lock(mutex_);
    stoped_=true;
    timer_.cancel();
}

void IOTimer::timerStart()
{
    timer_.expires_from_now(time_);
    timer_.async_wait(
        [this](const boost::system::error_code& ec)
        {
            timerCall(ec);
        }
    );
}

void WholePointTimer::secondNotify(const ErrorCode& ec, const NowKit::DateTime& dt)
{
    //把任务均匀平衡到多个线程里
    IOServer::post(
        [ec, dt]()
        {
            instance().secondDoit(ec, dt);
        }
    );
}

void WholePointTimer::secondDoit(const ErrorCode& ec, const NowKit::DateTime& tm)
{
    //不可能是整点
    //大多数情况都会从这里出去
    if(tm.second!=0)
        return;

    listDoit(ec, tm, minutes_);
    listDoit(ec, tm, hours_ [tm.minute]);
    listDoit(ec, tm, dates_ [tm.hour][tm.minute]);
    listDoit(ec, tm, weeks_ [tm.week][tm.hour][tm.minute]);
    listDoit(ec, tm, months_[tm.day-1][tm.hour][tm.minute]);
}

void WholePointTimer::listDoit(const ErrorCode& ec, const NowKit::DateTime& tm, MutexCallList& list)
{
    //因为向定时器添加新调用的情形并不是很多，所以可以使用锁
    MutexScopedLock lock(list.mutex);
    for(auto itr=list.list.begin(), end=list.list.end(); itr!=end; )
    {
        const auto notRm=(*itr)(ec, tm);
        if(notRm)
        {
            ++itr;
            continue;
        }
        itr=list.list.erase(itr);
    }
}

void WholePointTimer::timerPush(TimerCallBack&& , const std::string& timerLine)
{
    TimePattern tp;
    bool const ok=tp.init(timerLine);
    if(!ok)
        return;

    ///@todo 加入真正的错误处理与定时处理
}

NowKit::NowKit()
    : secondStart_(std::time(nullptr))
    , second_(MainServer::serviceGet(), 1000)
{
    second_.timerStart(
        [this](const ErrorCode& ec)
        {
            //更新每个线程的缓存
            MainServer::post(
                [ec]()
                {
                    auto& dt=dateTimeThreadGet();
                    dt=DateTime(std::time(nullptr));
                    WholePointTimer::secondNotify(ec, dateTimeGet());
                }
            );

            IOServer::postForeach(
                []()
                {
                    auto& dt=dateTimeThreadGet();
                    dt=DateTime(std::time(nullptr));
                }
            );
        }
    );
}

NowKit::DateTime::DateTime(std::time_t tm)
    :epoch(tm)
{
    const auto& pt=boost::posix_time::second_clock::local_time();
    const auto& dt=pt.date();
    const auto& ymd=dt.year_month_day();
    year=ymd.year;
    month=ymd.month;
    day=ymd.day;

    week=dt.day_of_week();

    const auto& du=pt.time_of_day();
    hour=du.hours();
    minute=du.minutes();
    second=du.seconds();
}


namespace xp
{

using namespace boost::xpressive;

placeholder<TimePattern> phTimePattern;
placeholder<std::vector<std::pair<uint32_t, uint32_t>>> phTimeUnitList;

class TimePush
{
public:
    typedef void result_type;

    template<typename Sequence, typename Tag, typename Value>
    void operator()(Sequence &seq, Tag const& tag, Value &val) const
    {
        seq.push(tag, val);
        val.clear();
    }

    template<typename Sequence, typename Tag>
    void operator()(Sequence &seq, Tag const& tag, const std::string &val) const
    {
        seq.push(tag, val);
    }
};

function<TimePush>::type const timePush = {{}};

static const sregex timePoint=repeat<4>(_d) >> '-' >> repeat<2>(_d) >> '-' >> repeat<2>(_d) >> +as_xpr(' ')
        >> repeat<2>(_d) >> ':' >> repeat<2>(_d) >> ':' >> repeat<2>(_d) >> *('.' >> repeat<1, 6>(_d));
static const sregex timeStart=((s1=as_xpr("start")) >> *_s >> '{' >> *_s >> (s2=timePoint) >> *_s >> '}')
        [timePush(phTimePattern, as<std::string>(s1), as<std::string>(s2))];

static const sregex timeEnd=((s1=as_xpr("end")) >> *_s >> '{' >> *_s >> (s2=timePoint) >> *_s >> '}')
        [timePush(phTimePattern, as<std::string>(s1), as<std::string>(s2))];

static const sregex number       = (s1=+_d)
        [push_back(phTimeUnitList, make_pair(as<uint32_t>(s1), as<uint32_t>(s1)))];

static const sregex numberPair   = ('(' >> *_s >> (s1=+_d) >> *_s >> ',' >> *_s >> (s2=+_d) >> *_s >> ')')
        [push_back(phTimeUnitList, make_pair(as<uint32_t>(s1), as<uint32_t>(s2)))];

static const sregex numberUnit   = number | numberPair;
static const sregex timeUnitList = numberUnit >> *( *_s >> ',' >> *_s >> numberUnit);

static const sregex timeLoopUnit = ((s1=as_xpr("year")|"month"|"date"|"hour"|"minute"|"second"|"week")
        >> *_s >> '[' >> *_s >> timeUnitList >> *_s >> ']')
        [timePush(phTimePattern, as<std::string>(s1), phTimeUnitList)];

static const sregex timeLoop = as_xpr("time") >> *_s >> '{'
        >> *_s >> timeLoopUnit >> *( *_s >> '-' >> *_s >> timeLoopUnit)
        >> *_s >> '}';

static const sregex timeLine = repeat<0,1>(timeStart)
        >> +(*_s >> repeat<0,1>("->") >> *_s >> timeLoop) >> repeat<0,1>(*_s >> "->" >> *_s >> timeEnd);

}  //namespace xp

static inline void bitSet(const std::vector<std::pair<uint32_t, uint32_t>>& pairs, std::bitset<64>& bit, int off=0)
{
    const size_t bsize=bit.size();
    for(const auto& pair: pairs)
    {
        for(uint32_t i=pair.first; i<=pair.second; ++i)
        {
            if(i<bsize)
                bit.set(i+off);
        }
    }
}

TimePattern::Trait TimePattern::trait_[TimePattern::eUnitCount]=
{
    {12 },
    {31 },
    {24 },
    {60 },
    {60 },
    {7  },
};

bool TimePattern::init(const std::string& line)
{
    std::vector<std::pair<uint32_t, uint32_t>> list;

    namespace bx=boost::xpressive;
    bx::smatch what;
    what.let(xp::phTimePattern=*this);
    what.let(xp::phTimeUnitList=list);

    if(false==bx::regex_match(line, what, xp::timeLine))
        return false;
    bitCorrect();
    return start_<end_;
}

bool TimePattern::start(const std::string& line)
{
    namespace bx=boost::xpressive;
    bx::smatch what;
    what.let(xp::phTimePattern=*this);

    return bx::regex_match(line, what, xp::timeStart);
}

bool TimePattern::time(const std::string& line)
{
    std::vector<std::pair<uint32_t, uint32_t>> list;

    namespace bx=boost::xpressive;
    bx::smatch what;
    what.let(xp::phTimePattern=*this);
    what.let(xp::phTimeUnitList=list);

    return bx::regex_match(line, what, xp::timeLoop);
}

bool TimePattern::end(const std::string& line)
{
    namespace bx=boost::xpressive;
    bx::smatch what;
    what.let(xp::phTimePattern=*this);

    if(false==bx::regex_match(line, what, xp::timeEnd))
        return false;
    bitCorrect();
    return start_<end_;
}

void TimePattern::bitCorrect()
{
    if(!year_.empty())
    {
        std::sort(year_.begin(), year_.end());
        if(start_.is_not_a_date_time())
            start_=bp::ptime(bg::date(year_.front(), 1, 1));
        if(end_.is_not_a_date_time())
            end_=bp::ptime(bg::date(year_.back(), 12, 30), bp::time_duration(23, 59, 59));
    }

    for(auto& bit: masks_)
    {
        if(bit.none())
            bit.set();
    }
}

void TimePattern::push(const std::string& tag, const std::vector<std::pair<uint32_t, uint32_t>>& pairs)
{
#ifndef _MSC_VER
    switch(stringHash(tag.c_str(), tag.c_str()+tag.size()))
    {
        case constHash("year"):
        {
            for(const auto& pair: pairs)
            {
                for(uint32_t i=pair.first; i<=pair.second; ++i)
                    year_.push_back(i);
            }
        }
        case constHash("month"):
            return bitSet(pairs, masks_[eMonth], -1);
        case constHash("date"):
            return bitSet(pairs, masks_[eDate], -1);
        case constHash("hour"):
            return bitSet(pairs, masks_[eHour]);
        case constHash("minute"):
            return bitSet(pairs, masks_[eMinute]);
        case constHash("second"):
            return bitSet(pairs, masks_[eSecond]);
        case constHash("week"):
            return bitSet(pairs, masks_[eWeek]);
    }
#endif
}

void TimePattern::push(const std::string& tag, const std::string& time)
{
#ifndef _MSC_VER
    switch(stringHash(tag.c_str(), tag.c_str()+tag.size()))
    {
        case constHash("start"):
        {
            start_=boost::posix_time::time_from_string(time);
            return;
        }
        case constHash("end"):
        {
            end_=boost::posix_time::time_from_string(time);
            return;
        }
    }
#endif
}

std::time_t TimePattern::nextIn(std::time_t t) const
{
    Step context;
    stepInit(t, context);
    auto const rlt=shift(context);
    if(!rlt.second)
        return 0;

    if(rlt.first && rlt.second)
        return LocalTime::toCTime(toSplitedTime(context));

    std::time_t oldTime=t;
    std::time_t newTime=oldTime;
    while(step(context))
    {
        oldTime=newTime;
        newTime=LocalTime::toCTime(toSplitedTime(context));
        if(newTime-oldTime>1)
            return newTime;
    }

    return 0;
}

std::time_t TimePattern::nextOut(std::time_t t) const
{
    Step context;
    stepInit(t, context);
    std::time_t oldTime=t;
    std::time_t newTime=oldTime;
    while(step(context))
    {
        oldTime=newTime;
        newTime=LocalTime::toCTime(toSplitedTime(context));
        if(newTime-oldTime>1)
            return oldTime+1;
    }

    return 0;
}

bool TimePattern::expired(std::time_t t) const
{
    auto const pt = bp::from_time_t(t);
    return !(pt < end_ && pt > start_);
}

bool TimePattern::in(std::time_t tm) const
{
    const LocalTime::SplitedTime st=LocalTime::timeSplit(tm);
    for(size_t i=0; i<eUnitCount; ++i)
    {
        if(!masks_[i].test(st[i+1]))
            return false;
    }

    return true;
}

static inline std::pair<std::bitset<64>, uint32_t>
dayMask(int year, int month, const std::bitset<64>& date, const std::bitset<64>& week)
{
    size_t dayIdx=0;
    std::bitset<64> day;
    bg::date_duration const oneDay(1);
    for(auto bd=bg::date(year, month+1, 1), ed=bd.end_of_month(); bd <= ed; bd += oneDay, ++dayIdx)
    {
        const int weekIdx = bd.day_of_week();
        day[dayIdx] = (date[dayIdx] && week[weekIdx]);
    }

    return std::make_pair(day, dayIdx);
}

inline void TimePattern::indexZero(Step& context, uint32_t where)
{
    for(; where<eUnitCount; ++where)
        context.maskIndex[where]=0;
}

std::pair<bool, bool> TimePattern::shift(Step& context) const
{
    if(!year_.empty())
    {
        const auto max=year_.back();
        const auto orgYear=context.year;
        for(; context.year<=max; ++context.year)
        {
            if(year_.end()!=std::find(year_.begin(), year_.end(), context.year))
                break;
        }

        if(context.year>max)
            return std::make_pair(true, false);

        if(orgYear!=context.year)
            indexZero(context, 0);
        remask(context, context.maskIndex[eMonth]);
    }

    std::pair<bool, bool> ret(false, true);
    for(size_t i=0; i<eUnitCount; ++i)
    {
        auto& mask=context.mask[i];
        auto& index=context.maskIndex[i];
        if(mask.test(index))
            continue;

        ret.first=true;
        ret.second=step(context, i);
        if(!ret.second)
            return ret;
        indexZero(context, i+1);
    }

    return ret;
}

bool TimePattern::wind(Step& context, uint32_t const windIndex) const
{
    switch(windIndex)
    {
        case eMonth:
        {
            context.year += 1;
            if(year_.empty())
            {
                const auto month=context.maskIndex[eMonth];
                remask(context, month);
                return true;
            }

            const auto max=year_.back();
            for(; context.year<=max; ++context.year)
            {
                if(year_.end()==std::find(year_.begin(), year_.end(), context.year))
                    continue;

                const auto month=context.maskIndex[eMonth];
                remask(context, month);
                return true;
            }

            return false;
        }
        case eDate:
        {
            if(!step(context, windIndex-1))
                return false;
            const auto month=context.maskIndex[windIndex-1];
            remask(context, month);
            return true;
        }
        case eHour:
        case eMinute:
        case eSecond:
            return step(context, windIndex-1);
        default:
            return false;
    }
}

bool TimePattern::step(Step& context, uint32_t const windIndex) const
{
    auto& mask =context.mask[windIndex];
    auto& index=context.maskIndex[windIndex];
    const auto count=context.maskCount[windIndex];
    index += 1;
    while(index<count && !mask[index])
        index += 1;

    //发生回绕
    while(index >= count)
    {
        index=0;
        if(!wind(context, windIndex))
            return false;
        while(!mask[index])
            index += 1;
    }

    return true;
}

void TimePattern::remask(Step& context, uint32_t month) const
{
    //计算新月的mask，新月的maskCount
    std::tie(context.mask[eDate], context.maskCount[eDate])
        =dayMask(context.year, month, masks_[eDate], masks_[eWeek]);
}

inline LocalTime::SplitedTime TimePattern::toSplitedTime(const Step& context)
{
    const auto& mi=context.maskIndex;
    return LocalTime::SplitedTime{context.year, mi[0], mi[1], mi[2], mi[3], mi[4], mi[5]};
}

template<typename Int, size_t Count>
static inline uint8_t castGet(const std::array<Int, Count>& ary, size_t idx)
{
    return static_cast<uint8_t>(ary[idx]);
}

void TimePattern::stepInit(std::time_t t, Step& context) const
{
    const auto tt=LocalTime::timeSplit(t);
    context=Step{
        tt[0],
        masks_,
        {castGet(tt, 1), castGet(tt, 2), castGet(tt, 3), castGet(tt, 4), castGet(tt, 5), castGet(tt, 6)},
        {trait_[eMonth].count, trait_[eDate].count, trait_[eHour].count,
            trait_[eMinute].count, trait_[eSecond].count, trait_[eWeek].count}
    };

    remask(context, tt[1]);
}

std::string TimePattern::toString() const
{
    auto const timeString=[](std::time_t t) -> std::string
    {
        //2013-11-04_17:34:35
#ifndef _MSC_VER
        std::tm tm;
        ::gmtime_r(&t, &tm);
#else
        std::tm tm = *std::gmtime(&t);
#endif

        char buf[24];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);

        return buf;
    };

    std::string ret;
    if(!start_.is_not_a_date_time())
    {
        ret += "start{";
        ret += timeString(LocalTime(start_).ctimeGet());
        ret += '}';
    }

    bool notNeed=true;

    std::stringstream stm;
    if(!year_.empty())
    {
        notNeed=false;
        stm << "year[" << *year_.begin();
        for(auto itr=++year_.begin(), end=year_.end(); itr!=end; ++itr)
            stm << ',' << *itr;
        stm << ']';
    }

    const char* dict[]= { "month", "date", "hour", "minute", "second", "week" };
    const int offset[]= { 1, 1, 0, 0, 0 , 0};
    for(size_t i=0; i<eUnitCount; ++i)
    {
        const auto& mask=masks_[i];
        std::vector<std::pair<int, int>> idx;
        std::pair<int, int> pair(-1, -1);
        for(size_t j=0; j<trait_[i].count; ++j)
        {
            if(mask[j])
            {
                if(pair.first==-1)
                {
                    pair=std::make_pair(j, j);
                    continue;
                }

                pair.second=j;
                continue;
            }

            if(pair.first==-1)
                continue;

            idx.push_back(pair);
            pair=std::make_pair(-1, -1);
        }

        if(pair.first!=-1)
            idx.push_back(pair);

        assert(!idx.empty());
        if(idx.size()==1 && idx.back().first==0 && idx.back().second==trait_[i].count-1)
            continue;

        if(notNeed)
            notNeed=false;
        else
            stm << '-';
        stm << dict[i] << '[';

        auto itr=idx.begin();

        if(itr->first==itr->second)
            stm << itr->first+offset[i];
        else
            stm << '(' << itr->first+offset[i] <<',' << itr->second+offset[i] <<  ')';

        for(++itr; itr!=idx.end(); ++itr)
        {
            stm << ',';
            if(itr->first==itr->second)
                stm << itr->first+offset[i];
            else
                stm << '(' << itr->first+offset[i] <<',' << itr->second+offset[i] <<  ')';
        }
        stm << ']';
    }

    const auto& time=stm.str();
    if(!time.empty())
    {
        ret += "->time{";
        ret += time;
        ret += '}';
    }

    if(!end_.is_not_a_date_time())
    {
        ret += "->end{";
        ret += timeString(LocalTime(end_).ctimeGet());
        ret += '}';
    }

    return std::move(ret);
}

}

