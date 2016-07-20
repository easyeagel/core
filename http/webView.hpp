//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  webView.hpp
//
//   Description:  网页视图
//
//       Version:  1.0
//       Created:  2015年03月06日 10时44分01秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#pragma once

#include<cassert>

#include<map>
#include<memory>
#include<vector>
#include<functional>

#include<boost/filesystem.hpp>

#include<core/thread.hpp>

namespace core
{

class HtmlTemplate: public core::SingleInstanceT<HtmlTemplate>
{
    HtmlTemplate();
    friend class core::OnceConstructT<HtmlTemplate>;
    typedef boost::filesystem::path Path;

public:
    const char* pageGet() const;

    static void pathAdd(const Path& path);

    struct TableColor
    {
        const char* head;
        const char* one;
        const char* two;
    };

    const TableColor& tableColorGet() const
    {
        static TableColor gs={"#FFBB77", "#DEDEBE", "#EFFFD7"};
        return gs;
    };

    const std::string& tplGet(const Path& file) const;
private:
    std::string contentRead(const Path& p);

private:
    static std::vector<Path> paths_;
    std::string zero_;
    std::map<Path, std::string> tplDict_;
};

namespace html
{

template<typename T>
struct TypeOf
{
    typedef T type;
};

template<size_t N>
struct TypeOf<char [N]>
{
    typedef const char* type;
};

template<typename Value>
class TextT
{
public:
    TextT(const Value& s)
        :text_(s)
    {}

    const typename TypeOf<Value>::type& get() const
    {
        return text_;
    }

private:
    typename TypeOf<Value>::type text_;
};

template<typename Value>
TextT<Value> text(const Value& v)
{
    return TextT<Value>(v);
}

template<typename First, typename Second>
class AttPairT
{
public:
    AttPairT(const First& n, const Second& v)
        :name_(n), value_(v)
    {}

    const typename TypeOf<First>::type& nameGet() const
    {
        return name_;
    }

    const typename TypeOf<Second>::type& valueGet() const
    {
        return value_;
    }

private:
    typename TypeOf<First>::type name_;
    typename TypeOf<Second>::type value_;
};

template<typename First, typename Second>
AttPairT<First, Second> att(const First& f, const Second& s)
{
    return AttPairT<First, Second>(f, s);
}

class Builder
{
public:
    Builder(const char* s)
        :text_(s)
    {}

    const char* get() const
    {
        return text_;
    }

private:
    const char* text_;
};

class TagEnd {};
const static TagEnd endtag={};

class HtmlBuilder
{
public:
    HtmlBuilder(std::string& dest, const char* name)
        :name_(name), dest_(dest), father_(this)
    {
        dest_ += '<';
        dest_ += name_;
    }

    HtmlBuilder(HtmlBuilder& father, const char* name)
        :name_(name), dest_(father.dest_), father_(&father)
    {
        dest_ += "><";
        dest_ += name_;
    }

    template<typename First, typename Second>
    HtmlBuilder& operator<<(const AttPairT<First, Second>& att)
    {
        dest_ += ' ';
        dest_ += att.nameGet();
        dest_ += "=\"";
        dest_ += att.valueGet();
        dest_ += '"';

        return *this;
    }

    template<typename V>
    HtmlBuilder& operator<<(const TextT<V>& text)
    {
        dest_ += '>';
        dest_ += text.get();

        return *this;
    }

    HtmlBuilder operator<<(const Builder& child)
    {
        return HtmlBuilder(*this, child.get());
    }

    HtmlBuilder& operator<<(const TagEnd& )
    {
        end();
        return *father_;
    }

    HtmlBuilder& end()
    {
        dest_ += "</";
        dest_ += name_;
        dest_ += '>';

        end_=true;

        return *this;
    }

    ~HtmlBuilder()
    {
        if(end_==false)
            end();
    }

private:
    bool end_=false;
    const char* name_;

    std::string& dest_;
    HtmlBuilder* father_=nullptr;
};

static inline HtmlBuilder build(std::string& dest, const char* n)
{
    return HtmlBuilder(dest, n);
}

static inline Builder build(const char* n)
{
    return Builder(n);
}

}

struct WebBlogAuthor;
struct WebBlogContent;
class WebView
{
public:
    struct Unit
    {
        std::string title;
        std::string topContent;
        std::string topRight;
    };

    std::string generate(const Unit& u);
    typedef std::function<void (const Unit& u, std::string& ret)> ReplaceCall;
private:
    static std::map<std::string, ReplaceCall> replaceDict_;
};


}

