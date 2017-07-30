/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include "Response.h"
#include "Request.h"

struct GenericResponse
{
    Response::GenericCode code;
    Reply::Type type;
    const char* content;
};

static const GenericResponse GenericResponseDefs[] = {
    {Response::Pong,                  Reply::Status,   "+PONG\r\n"},
    {Response::Ok,                    Reply::Status,   "+OK\r\n"},
    {Response::Cmd,                   Reply::Array,    "*0\r\n"},
    {Response::UnknownCmd,            Reply::Error,    "-ERR unknown command\r\n"},
    {Response::ArgWrong,              Reply::Error,    "-ERR argument wrong\r\n"},
    {Response::InvalidDb,             Reply::Error,    "-ERR invalid DB index\r\n"},
    {Response::NoPasswordSet,         Reply::Error,    "-ERR Client sent AUTH, but no password is set\r\n"},
    {Response::InvalidPassword,       Reply::Error,    "-ERR invalid password\r\n"},
    {Response::Unauth,                Reply::Error,    "-NOAUTH Authentication required.\r\n"},
    {Response::PermissionDeny,        Reply::Error,    "-ERR auth permission deny\r\n"},
    {Response::NoServer,              Reply::Error,    "-ERR no server avaliable\r\n"},
    {Response::NoServerConnection,    Reply::Error,    "-ERR no server connection avaliable\r\n"},
    {Response::ServerConnectionClose, Reply::Error,    "-ERR server connection close\r\n"},
    {Response::DeliverRequestFail,    Reply::Error,    "-ERR deliver request fail\r\n"},
    {Response::ForbidTransaction,     Reply::Error,    "-ERR forbid transaction in current server pool\r\n"},
    {Response::ConfigSubCmdUnknown,   Reply::Error,    "-ERR CONFIG subcommand must be one of GET, SET\r\n"},
    {Response::InvalidScanCursor,     Reply::Error,    "-ERR invalid cursor\r\n"},
    {Response::ScanEnd,               Reply::Array,    "*2\r\n$1\r\n0\r\n*0\r\n"}
};

thread_local static Response* GenericResponses[Response::CodeSentinel];

void Response::init()
{
    BufferPtr buf = BufferAlloc::create();
    for (auto& r : GenericResponseDefs) {
        Response* res = new Response();
        res->mType = r.type;
        if (buf->room() < (int)strlen(r.content)) {
            buf = BufferAlloc::create();
        }
        buf = res->mRes.set(buf, r.content);
        GenericResponses[r.code] = res;
    }
}

Response::Response():
    mType(Reply::None),
    mInteger(0)
{
}

Response::Response(GenericCode code):
    mType(Reply::None),
    mInteger(0)
{
    auto r = GenericResponses[code];
    mType = r->mType;
    mRes = r->mRes;
}


Response::~Response()
{
}

void Response::set(const ResponseParser& p)
{
    mType = p.type();
    mInteger = p.integer();
    mRes = p.response();
}


void Response::set(int64_t num)
{
    mType = Reply::Integer;
    mInteger = num;
    mRes.fset(nullptr, ":%ld\r\n", num);
}

void Response::setStr(const char* str, int len)
{
    mType = Reply::String;
    if (len < 0) {
        len = strlen(str);
    }
    mRes.fset(nullptr, "$%d\r\n%.*s\r\n", len, len, str);
}

void Response::setErr(const char* str, int len)
{
    mType = Reply::Error;
    if (len < 0) {
        len = strlen(str);
    }
    mRes.fset(nullptr, "-ERR %.*s\r\n", len, str);
}

void Response::adjustForLeader(Request* req)
{
    if (Request* leader = req->leader()) {
        switch (req->type()) {
        case Command::Mget:
            if (mType == Reply::Array) {
                mRes.cut(4);// cut "*1\r\n"
                if (leader == req) {
                    mHead.fset(nullptr, "*%d\r\n", req->followers());
                }
            } else {
                mType = Reply::Array;
                if (leader == req) {
                    mRes.fset(nullptr, "*%d\r\n$-1\r\n", req->followers());
                } else {
                    mRes.set(nullptr, "$-1\r\n");
                }
            }
            break;
        case Command::Mset:
            break;
        case Command::Msetnx:
        case Command::Touch:
        case Command::Exists:
        case Command::Del:
        case Command::Unlink:
            if (Response* r = leader->getResponse()) {
                if (r != this && mType == Reply::Integer && mInteger > 0) {
                    r->mInteger += mInteger;
                }
            }
            break;
        default:
            break;
        }
    }
}

bool Response::send(Socket* s)
{
    const char* dat;
    int len;
    while (mHead.get(dat, len)) {
        int n = s->write(dat, len);
        if (n > 0) {
            mHead.use(n);
        } else {
            return false;
        }
    }
    while (mRes.get(dat, len)) {
        int n = s->write(dat, len);
        if (n > 0) {
            mRes.use(n);
        } else {
            return false;
        }
    }
    return true;
}

int Response::fill(IOVec* vecs, int len, Request* req) const
{
    bool all = false;
    int n = mHead.fill(vecs, len, all);
    if (!all) {
        return n;
    }
    n += mRes.fill(vecs + n, len - n, all);
    if (n > 0 && all) {
        vecs[n - 1].req = req;
    }
    return n;

}

bool Response::getAddr(int& slot, SString<Const::MaxAddrLen>& addr, const char* token) const
{
    SegmentStr<Const::MaxAddrLen + 16> str(mRes);
    if (!str.complete() || !str.hasPrefix(token)) {
        return false;
    }
    const char* p = str.data() + strlen(token);
    slot = atoi(p);
    if (slot < 0 || slot >= Const::RedisClusterSlots) {
        return false;
    }
    p = strchr(p, ' ');
    if (!p) {
        return false;
    }
    p++;
    const char* e = strchr(p, '\r');
    if (!e) {
        return false;
    }
    int len = e - p;
    if (len >= Const::MaxAddrLen) {
        return false;
    }
    addr.set(p, len);
    return true;
}

