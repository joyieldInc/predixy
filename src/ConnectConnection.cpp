/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include "ConnectConnection.h"
#include "Handler.h"
#include "Subscribe.h"


ConnectConnection::ConnectConnection(Server* serv, bool shared):
    ConnectSocket(serv->addr().data(), SOCK_STREAM),
    mServ(serv),
    mAcceptConnection(nullptr),
    mShared(shared),
    mAuthed(false),
    mReadonly(false)
{
    mClassType = Connection::ConnectType;
}

ConnectConnection::~ConnectConnection()
{
}

bool ConnectConnection::writeEvent(Handler* h)
{
    FuncCallTimer();
    if (connectStatus() == Connecting) {
        logInfo("h %d s %s %d connect succ",
                h->id(), peer(), fd());
        setConnectStatus(Connected);
    }
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

int ConnectConnection::fill(Handler* h, IOVec* bufs, int len)
{
    FuncCallTimer();
    int cnt = 0;
    for (auto req = mSendRequests.front(); req; req = mSendRequests.next(req)) {
        int n = req->fill(bufs, len);
        bufs += n;
        cnt += n;
        len -= n;
        if (len == 0) {
            break;
        }
    }
    return cnt;
}

bool ConnectConnection::write(Handler* h, IOVec* bufs, int len)
{
    FuncCallTimer();
    logDebug("h %d s %s %d writev %d",
            h->id(), peer(), fd(), len);
    struct iovec iov[Const::MaxIOVecLen];
    for (int i = 0; i < len; ++i) {
        iov[i].iov_base = bufs[i].dat;
        iov[i].iov_len = bufs[i].len;
    }
    int num = writev(iov, len);
    if (num < 0) {
        logWarn("h %d s %s %d writev %d fail %s",
                h->id(), peer(), fd(), len, StrError());
        return false;
    } else if (num == 0) {
        return len == 0;
    }
    h->addServerWriteStats(mServ, num);
    IOVec* vec = nullptr;
    int i;
    for (i = 0; i < len && num > 0; ++i) {
        vec = bufs + i;
        int n = vec->len;
        vec->seg->seek(vec->buf, vec->pos, n <= num ? n : num);
        if (vec->req && n <= num) {
            while (!mSendRequests.empty()) {
                RequestPtr req = mSendRequests.front();
                mSendRequests.pop_front();
                if (vec->req == req) {
                    mSentRequests.push_back(req);
                    logVerb("h %d s %s %d req %ld sent",
                            h->id(), peer(), fd(), req->id());
                    break;
                } else {
                    logNotice("h %d s %s %d req %ld is empty",
                            h->id(), peer(), fd(), req->id());
                    h->handleResponse(this, req, nullptr);
                }
            }
        }
        num -= n;
    }
    return i == len && num == 0;
}

void ConnectConnection::readEvent(Handler* h)
{
    FuncCallTimer();
    while (true) {
        Buffer* buf = getBuffer(h, mParser.isIdle());
        int pos = buf->length();
        int len = buf->room();
        int n = read(buf->tail(), len);
        if (n > 0) {
            logVerb("h %d s %s %d read bytes %d",
                    h->id(), peer(), fd(), n);
            buf->use(n);
            h->addServerReadStats(mServ, n);
            parse(h, buf, pos);
            if (n < len) {
                break;
            }
        } else {
            if (!good()) {
                logWarn("h %d s %s %d read error %s",
                        h->id(), peer(), fd(), StrError());
            }
            break;
        }
    }
}

void ConnectConnection::parse(Handler* h, Buffer* buf, int pos)
{
    FuncCallTimer();
    bool goOn = true;
    while (pos < buf->length() && goOn) {
        auto ret = mParser.parse(buf, pos);
        switch (ret) {
        case ResponseParser::Normal:
        case ResponseParser::Partial:
            goOn = false;
            break;
        case ResponseParser::Complete:
            handleResponse(h);
            break;
        default:
            setStatus(ParseError);
            goOn = false;
            break;
        }
    }
}

void ConnectConnection::handleResponse(Handler* h)
{
    FuncCallTimer();
    if (mAcceptConnection) {
        if (mAcceptConnection->inSub(true)) {
            int chs;
            switch (SubscribeParser::parse(mParser.response(), chs)) {
            case SubscribeParser::Subscribe:
            case SubscribeParser::Psubscribe:
                mAcceptConnection->decrPendSub();
                //NO break; let it continue to setSub
            case SubscribeParser::Unsubscribe:
            case SubscribeParser::Punsubscribe:
                if (chs < 0) {
                    setStatus(LogicError);
                    logError("h %d s %s %d parse subscribe response error",
                            h->id(), peer(), fd());
                } else {
                    mAcceptConnection->setSub(chs);
                }
                break;
            case SubscribeParser::Message:
            case SubscribeParser::Pmessage:
                {
                    RequestPtr req = RequestAlloc::create(mAcceptConnection);
                    req->setType(Command::SubMsg);
                    mAcceptConnection->append(req);
                    mSentRequests.push_front(req);
                }
                break;
            default:
                if (Request* req = mSentRequests.front()) {
                    switch (req->type()) {
                    case Command::Psubscribe:
                    case Command::Subscribe:
                        mAcceptConnection->decrPendSub();
                        break;
                    default:
                        break;
                    }
                }
                break;
            }
        }
    }
    if (Request* req = mSentRequests.front()) {
        ResponsePtr res = ResponseAlloc::create();
        res->set(mParser);
        mParser.reset();
        logDebug("h %d s %s %d create res %ld match req %ld",
                h->id(), peer(), fd(), res->id(), req->id());
        h->handleResponse(this, req, res);
        mSentRequests.pop_front();
    } else {
        logNotice("h %d s %s %d recv res but no req",
                h->id(), peer(), fd());
    }
}

void ConnectConnection::close(Handler* h)
{
    SendRequestList* reqs[2] = {&mSentRequests, &mSendRequests};
    for (int i = 0; i < 2; ++i) {
        while (!reqs[i]->empty()) {
            auto req = reqs[i]->front();
            h->directResponse(req, Response::ServerConnectionClose, this);
            reqs[i]->pop_front();
        }
    }
    ConnectSocket::close();
    mParser.reset();
}

