//  Copyright [2016] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  view.cpp
//
//   Description:  html 视图
//
//       Version:  1.0
//       Created:  2016年09月28日 15时31分45秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include<core/http/view.hpp>

namespace core
{

void HtmlBlockView::toHtml(std::ostream& out)
{
    out
        << "<div";

    if(!id_.empty())
        out << " id=\"" << id_ << "\"";

    if(!class_.empty())
        out << " class=\"" << class_ << "\"";
    else 
        out << " class=\"blockView\"";

    out << ">";

    if(!title_.empty())
    {
        out << "<div class=\"blockTitle\">"
            << title_
            << "</div>"
        ;
    }

    if(rendered_==false)
    {
        render();
        rendered_=false;
    }

    {
        out << "<div class=\"blockContent\">";
        auto content=content_.str();
        if(content.empty())
            out << u8"内容为空";
        else
            out << content;

        out << "</div>";
    }

    out << "</div>";
}

void HtmlBlockView::render()
{
}

void HtmlTableView::render()
{
    if(lineCloesed_==false)
        lineClose();
    if(tableStarted_)
        contentGet() << "</table>";
}

void HtmlTableView::lineClose()
{
    const int n=(columnIndex_%columnCount_==0) ? 0 : columnCount_-columnIndex_%columnCount_;
    for(int i=0; i<n; ++i)
        contentGet() << "<td></td>";
    contentGet() << "</tr>";
    lineCloesed_=true;
}

void HtmlTableView::tableStart()
{
    contentGet() << "<table>";
    tableStarted_=true;
}

void HtmlTableView::lineStart()
{
    if(tableStarted_==false)
        tableStart();

    if(lineCloesed_==false)
        lineClose();

    contentGet()
        << "<tr"
        << (lineCount_%2==0 ? " class=\"trOne\"" : " class=\"trTwo\"")
        << ">";

    lineCloesed_=false;
    lineCount_ += 1;
}

}


