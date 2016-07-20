//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  webView.cpp
//
//   Description:  页面视图实现
//
//       Version:  1.0
//       Created:  2015年03月06日 10时48分16秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include<string>

#include<core/msvc.hpp>
#include<core/thread.hpp>
#include<core/string.hpp>

#include<core/http/root.hpp>
#include<boost/filesystem/fstream.hpp>

#include"webView.hpp"

namespace core
{

std::vector<HtmlTemplate::Path> HtmlTemplate::paths_= { "tpl", "app/tpl" };

HtmlTemplate::HtmlTemplate()
{
    auto& fr=FileRoot::instance();
    auto root=fr.rootPathGet();
    for(auto& p: paths_)
    {
        auto const path=root/p;
        if(!boost::filesystem::is_directory(path))
            continue;

        boost::filesystem::recursive_directory_iterator b(path), e;
        for(; b!=e; ++b)
        {
            auto t=b->path();
            if(t.extension()!=".tpl")
                continue;
            Path s=t.filename();
            while(t.remove_filename()!=path)
                s=t.filename()/s;
            tplDict_[s]=contentRead(b->path());
        }
    }
}

std::string HtmlTemplate::contentRead(const Path& p)
{
    boost::filesystem::ifstream f(p);
    if(!f)
        return std::string();
    std::istreambuf_iterator<char> b(f), e;
    std::string ret;
    std::copy(b, e, std::back_inserter(ret));
    return ret;
}

const char* HtmlTemplate::pageGet() const
{
    return R"HTMLPage(
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html>
<head>
<meta content="text/html; charset=utf-8" http-equiv="content-type" />
<style type="text/css">
#topFixed
{
    top:0px;
    width: 75%;
    padding-bottom: 10px;
    padding-left: 10px;
    padding-top: 10px;
    background-color:#e6e6aa;
    color:#006400;
    font-weight:bold;
    border-bottom-style: solid;
    border-bottom-width: 5px;
}

#siteLogo
{
    border-bottom-style: dotted;
    border-bottom-width: 1px;
    font-size:200%;

}

#topHead
{
    margin-top:20px;
}

#topContent
{
    width: 75%;
}

#topRight
{
    position:fixed;
    top:30px;
    right:10px;
    border-left-width: 5px;
    border-left-style: solid;
    border-left-color: #006400;
    padding-left: 20px;
    width: 20%;
    height: 80%;
}

#topRightGoto
{
    border-bottom-width: 1px;
    border-bottom-style: solid;
    padding-top: 10px;
    margin-bottom:20px;
    height:75px;
    width:80%;
}

#topTail
{
    border-top-width: 10px;
    border-top-style: ridge;
    padding-bottom: 30px;
    padding-top: 10px;
    padding-left: 20px;
    width: 75%;
}

body
{
    background-color: #e6e6fa;
    padding-left: 10px;
    font-family: "Microsoft YaHei", Tahoma, "Open Sans";
}

a
{
    text-decoration: none;
}

a:hover
{
    color:#ff00ff;
}

h1,h2,h3,h4,h5,h6
{
    color:#5B5B00;
}

dt
{
    font-weight:bold;
    font-size:150%;
    width:45%;
    border-bottom-width: 1px;
    border-bottom-style: solid;
}

pre
{
    background-color: #bebebe;
    border-style: solid;
    border-width: 1px;
    padding-left: 10px;
    padding-top: 10px;
    padding-bottom: 10px;
    width:80%;
}

</style>
<title>${title} | 易壳网 - ezshell.org</title>
</head>
<body>

<div id="topFixed">
<div id="siteLogo">
    易壳网 - ezshell.org
</div>
<div id="topHead">
    <a href="/blog">首页</a>
    <a href="/blog">博客</a>
    ${topHead}
</div>
</div>

<div id="topContent">
<h1>${title}</h1>
${topContent}
</div>

<div id="topRight">
<div id="topRightGoto">
    <a href="#siteLogo">顶端</a>
    <a href="#topTail">底端</a>
</div>
<div>
    ${topRight}
</div>
</div>

<div id="topTail">
${topTail}
</div>

</body>
</html>
    )HTMLPage";
}

const std::string& HtmlTemplate::tplGet(const Path& file) const
{
    auto itr=tplDict_.find(file);
    if(itr==tplDict_.end())
        return zero_;
    return itr->second;
}

static inline const char* skipTo(const char* ptr, char c)
{
    while(*ptr)
    {
        if(*ptr==c)
            return ptr;
        ++ptr;
    }

    return nullptr;
}

std::map<std::string, WebView::ReplaceCall> WebView::replaceDict_=
{
    {"title",
        [](const Unit& u, std::string& ret) { ret += u.title; } },
    {"topContent",
        [](const Unit& u, std::string& ret) { ret += u.topContent; } },
    {"topRight",
        [](const Unit& u, std::string& ret) { ret += u.topRight; } },
    {"topTail",
        [](const Unit& , std::string& ret) { ret += "<a href=\"/\">易壳网</a> - ezshell.org"; } },
    {"topHead",
        [](const Unit&  , std::string&    ) { } },
};

std::string WebView::generate(const Unit& u)
{
    std::string ret;
    const char* tpl=HtmlTemplate::instance().pageGet();
    while(*tpl)
    {
        switch(*tpl)
        {
            case '$':
            {
                if(tpl[1]!='{')
                {
                    ret.push_back(*tpl);
                    break;
                }

                tpl += 2;
                auto const end=skipTo(tpl, '}');
                assert(end!=nullptr);

                const std::string key(tpl, end);
                const auto itr=replaceDict_.find(key);
                assert(itr!=replaceDict_.end());
                itr->second(u, ret);

                tpl=end;
                break;
            }
            default:
                ret.push_back(*tpl);
                break;
        }

        ++tpl;
    }

    return std::move(ret);
}



}

