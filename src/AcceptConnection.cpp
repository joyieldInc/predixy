/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include "AcceptConnection.h"
#include "Conf.h"
#include "Handler.h"


AcceptConnection::AcceptConnection(int fd, sockaddr* addr, socklen_t len):
    AcceptSocket(fd, addr, len),
    mAuth(nullptr),
    mConnectConnection(nullptr),
    mLastActiveTime(0),
    mBlockRequest(false)
{
    mClassType = Connection::AcceptType;
}

AcceptConnection::~AcceptConnection()
{
    close();
}

void AcceptConnection::close()
{
    AcceptSocket::close();
    while (!mRequests.empty()) {
        auto req = mRequests.front();
        req->detach();
        mRequests.pop_front();
    }
}

bool AcceptConnection::writeEvent(Handler* h)
{
    FuncCallTimer();
    IOVec bufs[Const::MaxIOVecLen];
    bool finished = true;
    while (true) {
        int len = fill(h, bufs, Const::MaxIOVecLen);
        if (len == 0) {
            finished = true;
            break;
        }
        finished = write(h, bufs, len);
        if (!finished || len < Const::MaxIOVecLen) {
           break;
        }
    }
    return finished;
}

int AcceptConnection::fill(Handler* h, IOVec* bufs, int len)
{
    FuncCallTimer();
    int cnt = 0;
    Request* req = mRequests.empty() ? nullptr : mRequests.front();
    while (req && len > 0) {
        if (!req->isDone()) {
            if (!isBlockRequest()) {
                while (req) {
                    if (req->isDelivered()) {
                        break;
                    }
                    h->handleRequest(req);
                    req = mRequests.next(req);
                }
            }
            break;
        }
        auto res = req->getResponse();
        Request* next = mRequests.next(req);
        if (res) {
            logDebug("h %d c %s %d req %ld fill res %ld",
                    h->id(), peer(), fd(), req->id(), res->id());
            int n = res->fill(bufs, len, req);
            bufs += n;
            cnt += n;
            len -= n;
        } else if (cnt == 0) {//req no res and req is first in list
            mRequests.pop_front();
            logDebug("h %d c %s %d req %ld in front is done and no res",
                    h->id(), peer(), fd(), req->id());
        }
        req = next;
    }
    return cnt;
}

bool AcceptConnection::write(Handler* h, IOVec* bufs, int len)
{
    FuncCallTimer();
    logDebug("h %d c %s %d writev %d",
              h->id(), peer(), fd(), len);
    struct iovec iov[Const::MaxIOVecLen];
    for (int i = 0; i < len; ++i) {
        iov[i].iov_base = bufs[i].dat;
        iov[i].iov_len = bufs[i].len;
    }
    int num = writev(iov, len);
    if (num < 0) {
        logDebug("h %d c %s %d writev %d fail %s",
                h->id(), peer(), fd(), len, StrError());
        return false;
    } else if (num == 0) {
        return len == 0;
    }
    h->stats().sendClientBytes += num;
    IOVec* vec = nullptr;
    int i;
    for (i = 0; i < len && num > 0; ++i) {
        vec = bufs + i;
        int n = vec->len;
        vec->seg->seek(vec->buf, vec->pos, n <= num ? n : num);
        if (vec->req && n <= num) {
            while (!mRequests.empty()) {
                auto req = mRequests.pop_front();
                if (vec->req == req) {
                    logVerb("h %d c %s %d req %ld res sent",
                             h->id(), peer(), fd(), req->id());
                    req->clear();
                    break;
                } else {
                    logVerb("h %d c %s %d req %ld no res",
                            h->id(), peer(), fd(), req->id());
                    req->clear();
                }
            }
        }
        num -= n;
    }
    return i == len && num == 0;
}

void AcceptConnection::readEvent(Handler* h)
{
    FuncCallTimer();
    while (true) {
        auto buf = getBuffer(h, mParser.isIdle());
        int pos = buf->length();
        int len = buf->room();
        int n = read(buf->tail(), len);
        if (n > 0) {
            buf->use(n);
            h->stats().recvClientBytes += n;
            parse(h, buf, pos);
        }
        if (n < len) {
            break;
        }
    }
}

void AcceptConnection::parse(Handler* h, Buffer* buf, int pos)
{
    FuncCallTimer();
    bool goOn = true;
    bool split = h->proxy()->isSplitMultiKey() && !inTransaction();
    while (pos < buf->length() && goOn) {
        auto ret = mParser.parse(buf, pos, split);
        switch (ret) {
        case RequestParser::Normal:
            goOn = false;
            break;
        case RequestParser::Partial:
            {
                Request* req = RequestAlloc::create(this);
                mRequests.push_back(req);
                if (!mReqLeader) {
                    mReqLeader = req;
                }
                req->set(mParser, mReqLeader);
                h->handleRequest(req);
            }
            break;
        case RequestParser::Complete:
            {
                Request* req = RequestAlloc::create(this);
                mRequests.push_back(req);
                if (mReqLeader) {
                    req->set(mParser, mReqLeader);
                    mReqLeader = nullptr;
                } else {
                    req->set(mParser);
                }
                h->handleRequest(req);
                mParser.reset();
            }
            break;
        case RequestParser::CmdError:
            {
                Request* req = RequestAlloc::create(this);
                mRequests.push_back(req);
                if (inTransaction()) {
                    req->set(mParser);
                    h->handleRequest(req);
                } else {
                    ResponsePtr res = ResponseAlloc::create();
                    char err[1024];
                    int len = snprintf(err, sizeof(err), "unknown command '%s'",
                                       mParser.cmd());
                    res->setErr(err, len);
                    h->handleResponse(nullptr, req, res);
                }
                mParser.reset();
            }
            break;
        case RequestParser::ArgError:
            {
                Request* req = RequestAlloc::create(this);
                mRequests.push_back(req);
                if (inTransaction()) {
                    req->set(mParser);
                    h->handleRequest(req);
                } else {
                    ResponsePtr res = ResponseAlloc::create();
                    char err[1024];
                    int len = snprintf(err, sizeof(err),
                                    "wrong number of arguments for '%s' command",
                                    Command::get(mParser.type()).name);
                    res->setErr(err, len);
                    h->handleResponse(nullptr, req, res);
                }
                mParser.reset();
            }
            break;
        default:
            setStatus(ParseError);
            goOn = false;
            break;
        }
    }
}

bool AcceptConnection::send(Handler* h, Request* req, Response* res)
{
    FuncCallTimer();
    if (mRequests.front()->isDone()) {
        return true;
    }
    return false;
}
