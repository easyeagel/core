//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  http.hpp
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

#pragma once

#include<map>
#include<memory>
#include<functional>

#include<core/thread.hpp>

#include"error.hpp"
#include"httpParser.hpp"

namespace ezweb
{

class HttpParser
{
public:
    typedef std::map<std::string, std::string> Header;

    HttpParser(http_parser_type type=HTTP_BOTH);

    const Header& headGet() const
    {
        return headers_;
    }

    const std::string& urlGet() const
    {
        return url_;
    }

    size_t parse(const char* buf, size_t nb);

    bool good() const
    {
        return parser_.http_errno==0;
    }

    bool bad() const
    {
        return !good();
    }

    bool isHeadComplete() const
    {
        return isHeadComplete_;
    }

    bool isMessageComplete() const
    {
        return isMessageComplete_;
    }

    bool isKeep() const
    {
        return http_should_keep_alive(&parser_)!=0;
    }

    void reset();

    http_method methodGet() const
    {
        return static_cast<http_method>(parser_.method);
    }

    int statusGet() const
    {
        return parser_.status_code;
    }

    std::string pathGet() const
    {
        return urlPath_;
    }

    typedef std::function<int (const char* b, size_t n)> OnBody;
    template<typename Call>
    void onBody(Call&& call)
    {
        onBody_=std::move(call);
    }

    const std::string& hostGet() const
    {
        return host_;
    }

private:
    static int on_message_begin(http_parser* );
    static int on_status(http_parser* , const char* , size_t ) ;
    static int on_url(http_parser* hp, const char* at, size_t nb) ;
    static int on_header_field(http_parser* hp, const char* at, size_t nb) ;
    static int on_header_value(http_parser* hp, const char* at, size_t nb) ;
    static int on_headers_complete(http_parser* );
    static int on_body(http_parser* , const char* , size_t ) ;
    static int on_message_complete(http_parser* );

    static HttpParser& get(http_parser* hp)
    {
        return *static_cast<HttpParser*>(hp->data);
    }

private:
    bool isHeadComplete_    : 1;
    bool isMessageComplete_ : 1;

    http_parser parser_;
    http_parser_type type_;
    http_parser_settings settings_;

    std::string url_;
    http_parser_url urls_;
    std::string urlPath_;

    std::string headKey_;
    std::string headValue_;
    std::string host_;
    std::map<std::string, std::string> headers_;

    OnBody onBody_;
};

class HttpMessage
{
public:
    typedef std::map<std::string, std::string> HeaderDict;

    const std::string& headGet() const
    {
        return headCache_;
    }

    const std::string& bodyGet() const
    {
        return bodyCache_;
    }

    void commonHeadSet(const std::string& k, const std::string& v)
    {
        commonHead_[k]=v;
    }

    void privateHeadSet(const std::string& k, const std::string& v)
    {
        commonHead_[k]=v;
    }

    void bodySet(const std::string& s)
    {
        bodyCache_=s;
    }

    void bodySet(std::string&& s)
    {
        bodyCache_=std::move(s);
    }


protected:
    void reset()
    {
        privateHead_.clear();
    }

    template<typename C>
    void headCacheImpl(const C& c);

    void headCache();

protected:
    HeaderDict commonHead_;
    HeaderDict privateHead_;

    std::string headCache_;
    std::string bodyCache_;
};

class HttpResponse: public HttpMessage
{
public:
    enum HttpStatus
    {
        eHttpOk=200,
        eHttpNoContent=204,
        
        eHttpMove=301,
        eHttpFound=302,

        eHttpForbidden=403,
        eHttpNotFound=404,
    };

    HttpResponse(HttpStatus status);

    void reset(HttpStatus status);

    void cache();

private:
    static std::map<HttpStatus, const char*> gsMsgs_;

private:
    HttpStatus status_;
};

class HttpRequest: public HttpMessage
{
public:
    typedef http_method HttpMethod;
    HttpRequest(HttpMethod method);
    HttpRequest(HttpMethod method, const std::string& path);

    void reset(HttpMethod method)
    {
        method_=method;
        HttpMessage::reset();
    }

    void reset(HttpMethod method, const std::string& path)
    {
        path_=path;
        method_=method;
        HttpMessage::reset();
    }

    void cache();

private:
    HttpMethod method_;
    std::string path_;
};

typedef std::shared_ptr<HttpResponse> HttpResponseSPtr;

class HttpGetDispatch: public core::SingleInstanceT<HttpGetDispatch>
{
    HttpGetDispatch()=default;
    friend class core::OnceConstructT<HttpGetDispatch>;
public: 
    typedef std::function<HttpResponseSPtr (core::ErrorCode&, const HttpParser&)> GetCall;
    typedef std::map<std::string, GetCall> PathDict;

    template<typename Call>
    void insert(const std::string& url, Call&& call)
    {
        paths_[url]=std::move(call);
    }

    HttpResponseSPtr dispatch(core::ErrorCode& ec, const HttpParser& hp) const;

private:
    HttpResponseSPtr logicNotFound(const std::string& path=std::string()) const;
    HttpResponseSPtr redirectMove(const char* path) const;

private:
    PathDict paths_;
};

}

