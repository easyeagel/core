//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  time.hpp
//
//   Description:  时间操作
//
//       Version:  1.0
//       Created:  2013年11月05日 15时44分21秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================


#ifndef MR_TIME_HPP
#define MR_TIME_HPP

#include<ctime>

#include<set>
#include<list>
#include<string>
#include<boost/asio/steady_timer.hpp>
#include<boost/date_time/posix_time/posix_time.hpp>

#include"lock.hpp"
#include"thread.hpp"
#include"server.hpp"

namespace core
{
class IOService;
class ErrorCode;

class LocalTime
{
    typedef boost::posix_time::ptime PTime;
public:
    enum {eYear, eMonth, eDay, eHour, eMinute, eSecond, eWeek, eCount};
    typedef std::array<uint32_t, eCount> SplitedTime;

    LocalTime()=default;
    LocalTime(std::time_t tm);

    LocalTime(const PTime& tm)
        :time_(toLocalTime(tm))
    {}

    std::time_t ctimeGet() const;

    PTime utcTimeGet() const;

    PTime localTimeGet() const
    {
        return time_;
    }

    SplitedTime splitedTimeGet() const;

    static std::time_t toCTime(const SplitedTime& st);

    static SplitedTime timeSplit(std::time_t t)
    {
        LocalTime tmp(t);
        return tmp.splitedTimeGet();
    }

    static std::time_t timeMake(int year, int month, int day, int hour, int minute, int second);

    static PTime toUTCTime(const PTime& pt)
    {
        LocalTime tmp;
        tmp.time_=pt;
        return tmp.utcTimeGet();
    }

    static PTime toLocalTime(const PTime& pt);

private:
    static std::time_t offset();

private:
    PTime time_;
};

std::time_t secondNow();
std::string nowString();
std::string nowPathString();
std::string timeString(std::time_t tm);
std::string nowStringForestall();
std::string nowPathStringForestall();

/**
* @brief 定时器，循环定时执行某项任务
* @details
*   @li 该任务不会同时存在两个实例在运行，即使一个次执行超过定时间隔
*   @li 任务是同步执行的，如果任务是异步执行，任务本身需要管理同步和可能的并发
*/
class IOTimer
{
    ///默认时间间隔是一秒，即1000毫秒
    enum {eComputeTimer=1000};
    typedef boost::asio::steady_timer SteadyTimer;

    typedef std::function<void()> CallFun;
public:
    IOTimer(size_t milli=eComputeTimer);
    IOTimer(IOService& service, size_t milli=eComputeTimer);

    typedef std::function<void(const ErrorCode& ec)> TimerCallBack;

    void timerStart(TimerCallBack&& handle);

    /**
    * @brief 指定任务执行的循环时间间隔
    *
    * @param milli 单位为毫秒的时间间隔
    */
    void timeSet(size_t milli)
    {
        time_=std::chrono::milliseconds(milli);
    }

private:
    void timerCall(const boost::system::error_code& ec);
    void timerStop();
    void timerStart();

private:
    Spinlock mutex_;
    bool stoped_=true;
    CallFun stopCall_;
    SteadyTimer timer_;
    TimerCallBack call_;
    std::chrono::milliseconds time_;
};

/**
* @brief 当前时间工具
* @details
*   @li 获取当前时间可能是一个性能低下的操作(std::time函数可能很慢)
*   @li 使用本类达到性能优化，但同时降低了时间分辨率
*/
class NowKit
    :public SingleInstanceT<NowKit>
{
    friend class OnceConstructT<NowKit>;

    NowKit();
public:
    /**
    * @brief 获取定时更新的当前时刻
    *
    * @return 当前时刻
    */
    static std::time_t secondGet()
    {
        return dateTimeGet().epoch;
    }

    /**
    * @brief 获取进程启动时间到当前时间间隔
    *
    * @return 进程启动时长
    */
    static uint32_t steadySecondGet()
    {
        return secondGet()-instance().secondStart_;
    }

    static void start()
    {
        instance();
    }

    struct DateTime
    {
        DateTime(std::time_t tm=std::time(nullptr));

        std::time_t epoch;

        int year;
        int month;
        int day;
        int week;
        int hour;
        int minute;
        int second;
    };

    static const DateTime& dateTimeGet()
    {
        return dateTimeThreadGet();
    }

private:
    static DateTime& dateTimeThreadGet()
    {
        struct DateTimeThread: public ThreadInstanceT<DateTime>
        {};

        return DateTimeThread::instance();
    }

private:
    std::time_t secondStart_;
    IOTimer second_;
};

/**
* @brief 整点定时器
* @note 本定时器是全局对象，大量平凡的定时任务请使用 @ref IOTimer，它将提供更高的性能
* @details
*   @li 在一个时间段开始时执行一些任务
*   @li 时间段支持: 分钟，小时，天，周，月
*   @li 不会同时存在两个任务实时在运行，参考 @ref IOTimer 描述
*/
class WholePointTimer
    : public SingleInstanceT<WholePointTimer>
{
public:

    /**
    * @brief 定时器回调函数
    * @details
    *   @li 回调函数原型满足 bool fun(const ErrorCode& ec, std::time_t tm)
    *   @li 返回: 在下个整点需要再调用该函数时，其应该返回 true；
    *       否则的话，将不会再调用
    *   @li ec: 当出现某种错误时，会把错误值传递给回调，典型错误是停机
    *   @li tm: 当前时刻值
    */
    typedef std::function<bool (const ErrorCode& ec, const NowKit::DateTime& tm)> TimerCallBack;


private:
    friend class NowKit;

    typedef boost::mutex Mutex;
    typedef boost::unique_lock<Mutex> MutexScopedLock;

    struct MutexCallList
    {
        Mutex mutex;
        std::list<TimerCallBack> list;
    };

    enum {eMinuteNumOfHour=60, eHourNumOfDay=24, eDayNumWeek=7, eDayNumMonth=31};
    typedef std::array<MutexCallList,eMinuteNumOfHour> HourCallDict;
    typedef std::array<HourCallDict, eHourNumOfDay   > DayCallDict;
    typedef std::array<DayCallDict,  eDayNumWeek     > WeekCallDict;
    typedef std::array<DayCallDict,  eDayNumMonth    > MonthCallDict;
public:
    static void minuteTimerPush(TimerCallBack&& call)
    {
        listPush(instance().minutes_, std::move(call));
    }

    static void hourTimerPush  (TimerCallBack&& call, unsigned minuteOfHour)
    {
        assert(minuteOfHour<eMinuteNumOfHour);
        if(minuteOfHour>=eMinuteNumOfHour)
            return;
        listPush(instance().hours_[minuteOfHour], std::move(call));
    }

    static void dayTimerPush  (TimerCallBack&& call, unsigned hourOfDay=0, unsigned minuteOfHour=0)
    {
        assert(hourOfDay<eHourNumOfDay && minuteOfHour<eMinuteNumOfHour);
        if(hourOfDay>=eHourNumOfDay || minuteOfHour>=eMinuteNumOfHour)
            return;
        listPush(instance().dates_[hourOfDay][minuteOfHour], std::move(call));
    }

    static void weekTimerPush  (TimerCallBack&& call, unsigned dayOfWeek=0
        , unsigned hourOfDay=0, unsigned minuteOfHour=0)
    {
        assert(hourOfDay<eHourNumOfDay && dayOfWeek<eDayNumWeek && minuteOfHour<eMinuteNumOfHour);
        if(hourOfDay>=eHourNumOfDay || dayOfWeek>=eDayNumWeek || minuteOfHour>=eMinuteNumOfHour)
            return;
        listPush(instance().weeks_[dayOfWeek][hourOfDay][minuteOfHour], std::move(call));
    }

    static void monthTimerPush (TimerCallBack&& call, unsigned dayOfMonth=0
        , unsigned hourOfDay=0, unsigned minuteOfHour=0)
    {
        assert(hourOfDay<eHourNumOfDay && dayOfMonth<eDayNumMonth && minuteOfHour<eMinuteNumOfHour);
        if(hourOfDay>=eHourNumOfDay || dayOfMonth>=eDayNumMonth || minuteOfHour>=eMinuteNumOfHour)
            return;
        listPush(instance().months_[dayOfMonth][hourOfDay][minuteOfHour], std::move(call));
    }

    //m[12, 13] w[1, 2] [1, 3, 5]:[23, 45]
    static void timerPush(TimerCallBack&& call, const std::string& timerLine);

private:
    static void secondNotify(const ErrorCode& ec, const NowKit::DateTime& tm);

    void secondDoit(const ErrorCode& ec, const NowKit::DateTime& tm);
    void listDoit(const ErrorCode& ec, const NowKit::DateTime& tm, MutexCallList& list);

    static void listPush(MutexCallList& list, TimerCallBack&& call)
    {
        MutexScopedLock lock(list.mutex);
        list.list.emplace_back(std::move(call));
    }


private:
    MutexCallList minutes_;
    HourCallDict  hours_;
    DayCallDict   dates_;
    WeekCallDict  weeks_;
    MonthCallDict months_;
};


/**
* @brief 时间轮定时功能
*
* @tparam Num 时间轮大小
* @tparam Value 定时器代理值
*/
template<typename Obj, size_t Num, typename Value, typename Opt>
class TimerWheelT: public ThreadInstanceT<Obj>
{
    typedef ThreadInstanceT<Obj> BaseThis;

protected:
    struct TimerUnit
    {
        TimerUnit(const std::shared_ptr<Value>& p)
            :ptr(p)
        {}

        TimerUnit(TimerUnit&& o)
            :ptr(std::move(o.ptr))
        {}

        std::weak_ptr<Value> ptr;

        ~TimerUnit()
        {
            auto p=ptr.lock();
            if(p)
                Opt()(p);
        }
    };

    typedef std::shared_ptr<TimerUnit> SetUnit;
    typedef std::set<SetUnit> Set;

    static size_t whereGet()
    {
        return BaseThis::instance().where_;
    }

public:
    TimerWheelT(size_t milli)
        :timer_(ThreadThis::serviceGet(), milli)
    {
        timer_.timerStart(
            [this](const ErrorCode& ec)
            {
                timerCall(ec);
            }
        );
    }

    static void update(const std::shared_ptr<TimerUnit>& p)
    {
        BaseThis::instance().updateImpl(p);
    }

private:
    void updateImpl(const std::shared_ptr<TimerUnit>& p)
    {
        wheel_[where_].emplace(p);
    }

    void timerCall(const ErrorCode& ec)
    {
        if(ec.bad())
        {
            for(auto& wheel: wheel_)
                wheel.clear();
            return;
        }

        switch(where_)
        {
            case Num-1:
            {
                where_ = 0;
                wheel_[where_].clear();
                return;
            }
            default:
            {
                where_ += 1;
                wheel_[where_].clear();
                return;
            }
        }
    }

private:
    IOTimer timer_;
    size_t where_=0;
    std::array<Set, Num> wheel_;
};

//start{2014-01-10 14:14:02}
//-> time{
//  -year  [2014, 2015, (2017-2019)]
//  -month [2, 3, (4, 10)]
//  -date  [1, 3, 5, (10, 20)]
//  -hour  [1, 2, (18, 20)]
//  -minute[2, 3, (40, 50)]
//  -second[2, 3, (30, 50)]
//  -week  [1, 2, (4, 6)]
// }
//-> end{2014-12-12 24:00:00}
namespace xp
{
    struct TimePush;
}

class TimePattern
{
    friend class xp::TimePush;

    enum {eMonth, eDate, eHour, eMinute, eSecond, eWeek, eUnitCount};

    typedef uint32_t Year_t;
    typedef std::vector<Year_t> YearVector;

    typedef std::bitset<64> Mask;
    typedef std::array<Mask, eUnitCount> MaskArray;
    typedef std::array<uint8_t, eUnitCount> MaskIndex;

    struct Trait
    {
        uint8_t count;
    };

    struct Step
    {
        Year_t year;

        MaskArray mask;
        MaskIndex maskIndex;
        MaskIndex maskCount;
    };

public:
    bool init(const std::string& line);

    bool start(const std::string& line);
    bool time (const std::string& line);
    bool end  (const std::string& line);

    std::time_t nextIn (std::time_t tm) const;
    std::time_t nextOut(std::time_t tm) const;

    bool in     (std::time_t tm) const;
    bool expired(std::time_t tm) const;

    std::string toString() const;

private:
    void bitCorrect();
    void push(const std::string& tag, const std::string& time);
    void push(const std::string& tag, const std::vector<std::pair<uint32_t, uint32_t>>& pairs);

    void remask(Step& step, uint32_t month) const;
    bool wind(Step& step, uint32_t const windIndex) const;
    bool step(Step& context, uint32_t const windIndex=TimePattern::eSecond) const;

    void stepInit(std::time_t t, Step& context) const;

    //执行了step，step执行结果
    std::pair<bool, bool> shift(Step& context) const;

    static void indexZero(Step& context, uint32_t where);
    static LocalTime::SplitedTime toSplitedTime(const Step& context);

private:
    boost::posix_time::ptime start_;
    boost::posix_time::ptime end_;

    YearVector year_;
    MaskArray masks_;
    static Trait trait_[eUnitCount];
};

}


#endif //MR_TIME_HPP

