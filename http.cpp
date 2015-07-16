﻿//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  http.cpp
//
//   Description:  http 处理
//
//       Version:  1.0
//       Created:  2015年03月03日 15时10分44秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include<cstring>
#include<boost/algorithm/string/split.hpp>

#include"httpRoot.hpp"
#include"http.hpp"

namespace ezweb
{

int HttpParser::on_message_begin(http_parser* )
{
    return 0;
}

int HttpParser::on_status(http_parser* , const char* , size_t ) 
{
    return 0;
}

int HttpParser::on_url(http_parser* hp, const char* at, size_t nb) 
{
    auto& self=get(hp);
    self.url_=std::string(at, nb);
    auto const ret=http_parser_parse_url(self.url_.data(), self.url_.size(), 0, &self.urls_);
    if(ret!=0)
        return ret;

    const auto& s=self.urls_.field_data[UF_PATH];
    self.urlPath_=self.url_.substr(s.off, s.len);
    return 0;
}

int HttpParser::on_header_field(http_parser* hp, const char* at, size_t nb) 
{
    get(hp).headKey_=std::string(at, nb);
    return 0;
}

int HttpParser::on_header_value(http_parser* hp, const char* at, size_t nb) 
{
    auto& self=get(hp);
    if(self.headKey_=="Host")
    {
        self.host_=std::string(at, nb);
    } else {
        self.headValue_=std::string(at, nb);
    }
    self.headers_[std::move(self.headKey_)]=std::move(self.headValue_);
    return 0;
}

int HttpParser::on_headers_complete(http_parser* hp)
{
    auto& self=get(hp);
    self.isHeadComplete_=true;
    return 0;
}

int HttpParser::on_body(http_parser* hp, const char* b, size_t n) 
{
    auto& self=get(hp);
    if(self.onBody_)
        return self.onBody_(b, n);
    return 0;
}

int HttpParser::on_message_complete(http_parser* hp)
{
    auto& self=get(hp);
    self.isMessageComplete_=true;
    return 0;
}

HttpParser::HttpParser(http_parser_type type)
    :isHeadComplete_(false)
    ,isMessageComplete_(false)
    ,type_(type)
{
    http_parser_init(&parser_, type_);
    parser_.data=this;

    std::memset(&settings_, 0, sizeof(settings_));
    settings_.on_message_begin=on_message_begin;
    settings_.on_url=on_url;
    settings_.on_status=on_status;
    settings_.on_header_field=on_header_field;
    settings_.on_header_value=on_header_value;
    settings_.on_headers_complete=on_headers_complete;
    settings_.on_body=on_body;
    settings_.on_message_complete=on_message_complete;
}

size_t HttpParser::parse(const char* buf, size_t nb)
{
    return http_parser_execute(&parser_, &settings_, buf, nb);
}

void HttpParser::reset()
{
    http_parser_init(&parser_, type_);
    parser_.data=this;

    isHeadComplete_=false;
    isMessageComplete_=false;
    headers_.clear();
}


std::map<HttpResponse::HttpStatus, const char*> HttpResponse::gsMsgs_=
{
    { eHttpOk,        "200 OK"},
    { eHttpNoContent, "204 No Content"},
    { eHttpMove,      "301 Moved Permanently"},
    { eHttpFound,     "302 Found"},
    { eHttpForbidden, "403 Forbidden"},
    { eHttpNotFound,  "404 Not Found"},
};

HttpResponse::HttpResponse(HttpStatus st)
    :status_(st)
{}

void HttpResponse::reset(HttpStatus st)
{
    status_=st;
    HttpMessage::reset();
}

template<typename C>
void HttpMessage::headCacheImpl(const C& c)
{
    for(const auto& head: c)
    {
        headCache_.append(head.first);
        headCache_.append(": ");
        headCache_.append(head.second);
        headCache_.append("\r\n");
    }
}

void HttpMessage::headCache()
{
    headCacheImpl(commonHead_);
    headCacheImpl(privateHead_);
}

void HttpResponse::cache()
{
    headCache_.clear();
    headCache_.append("HTTP/1.1 ");
    headCache_.append(gsMsgs_[status_]);
    headCache_.append("\r\n");

    headCache();

    headCache_.append("\r\n");
}

HttpRequest::HttpRequest(HttpMethod method)
    :method_(method)
{}

HttpRequest::HttpRequest(HttpMethod method, const std::string& path)
    :method_(method), path_(path)
{}

void HttpRequest::cache()
{
    headCache_.clear();

    headCache_.append(http_method_str(method_));
    headCache_.append(" ");
    headCache_.append(path_);
    headCache_.append(" ");
    headCache_.append("HTTP/1.1 ");
    headCache_.append("\r\n");

    headCache();

    headCache_.append("\r\n");
}

HttpResponseSPtr HttpGetDispatch::dispatch(core::ErrorCode& ec, const HttpParser& hp) const
{
    if(ec.bad())
        return logicNotFound();

#ifdef NDEBUG
    //只处理已知域名
    const char* local="127.0.0.1";
    if(hp.hostGet()!="www.ezshell.org" && std::strncmp(hp.hostGet().c_str(), local, std::strlen(local))!=0)
        return redirectMove("http://www.ezshell.org/");
#endif

    auto path=hp.pathGet();
    if(path=="/")
        return redirectMove("/blog");

    for(;;)
    {
        auto const itr = paths_.find(path);
        if(itr!=paths_.end())
        {
            auto ptr=itr->second(ec, hp);
            if(ptr)
                return ptr;
            return logicNotFound(hp.pathGet());
        }

        auto const pos=path.find_last_of('/');
        if(pos==std::string::npos || pos==0)
            return logicNotFound(hp.pathGet());

        path.resize(pos);
    }

}

HttpResponseSPtr HttpGetDispatch::logicNotFound(const std::string& path) const
{
    auto& fr=FileRoot::instance();
    auto respones=fr.get(path);
    if(respones)
        return respones;

    respones=std::make_shared<HttpResponse>(HttpResponse::eHttpNotFound);
    respones->commonHeadSet("Content-Type", "text/html; charset=utf-8");
    respones->commonHeadSet("Connection", "Close");
    respones->bodySet(R"HTML(
    <html>
        <head>
            <meta HTTP-EQUIV="REFRESH" content="3; url=/">
        </head>
        <body>
            <h1>404 - 没有发现</h1>
            <p>将跳转到首页...</p>
        </body>
    </html>)HTML");
    respones->cache();

    return respones;
}

HttpResponseSPtr HttpGetDispatch::redirectMove(const char* path) const
{
    auto respones=std::make_shared<HttpResponse>(HttpResponse::eHttpMove);
    respones->commonHeadSet("Location", path);
    respones->commonHeadSet("Connection", "Close");
    respones->cache();

    return respones;
}

}
