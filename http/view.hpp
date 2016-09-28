//  Copyright [2016] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  view.hpp
//
//   Description:  html 视图
//
//       Version:  1.0
//       Created:  2016年09月28日 15时05分27秒
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

#include<string>
#include<sstream>

namespace core
{

class HtmlBlockView
{
public:
    void toHtml(std::ostream& out);

    std::string toHtml()
    {
        std::ostringstream out;
        toHtml(out);
        return out.str();
    }

    void titleSet(const std::string& s)
    {
        title_=s;
    }

    void idSet(const std::string& s)
    {
        id_=s;
    }

    void classSet(const std::string& s)
    {
        class_=s;
    }

    std::ostringstream& contentGet()
    {
        return content_;
    }

    virtual ~HtmlBlockView()=default;

protected:
    virtual void render();

protected:
    std::string id_;
    std::string class_;
    std::string title_;
    std::ostringstream content_;

    bool rendered_=false;
};

class HtmlTableView: public HtmlBlockView
{
public:
    HtmlTableView(int cc)
        :columnCount_(cc)
    {}

    template<typename... Args>
    void append(Args&&... args)
    {
        assert(sizeof...(Args)==columnCount_);

        lineStart();
        columnAppend(std::forward<Args>(args)...);
        columnIndex_ += columnCount_;
        lineClose();
    }

    template<typename Arg>
    void columnAppend(Arg&& arg)
    {
        const bool colBreak=(columnIndex_%columnCount_==0);
        if(colBreak)
            lineStart();
        columnAdd(std::forward<Arg>(arg));
        columnIndex_ += 1;
    }

private:
    void render() final;

    template<typename Arg, typename... Args>
    void columnAdd(Arg&& arg, Args&&... args)
    {
        contentGet()
            << "<td>"
            << arg
            << "</td>"
        ;

        columnAdd(std::forward<Args>(args)...);
    }

    void columnAdd()
    {}

    void lineStart();
    void lineClose();
    void tableStart();

protected:
    bool tableStarted_=false;
    bool lineCloesed_=true;

    int lineCount_=0;
    int columnIndex_=0;
    const int columnCount_=0;
};

class HtmlFormatedView: public HtmlBlockView
{
public:
    template<typename T>
    HtmlFormatedView& operator<< (const T& t)
    {
        contentGet()
            << t;
        return *this;
    }
};

}

