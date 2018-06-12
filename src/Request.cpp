/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include "Request.h"
#include "RequestParser.h"
#include "Response.h"

struct GenericRequest
{
    Request::GenericCode code;
    Command::Type type;
    const char* content;
};

static const GenericRequest GenericRequestDefs[] = {
    {Request::Ping,         Command::Ping,          "*1\r\n$4\r\nping\r\n"},
    {Request::PingServ,     Command::PingServ,      "*1\r\n$4\r\nping\r\n"},
    {Request::ClusterNodes, Command::ClusterNodes,  "*2\r\n$7\r\ncluster\r\n$5\r\nnodes\r\n"},
    {Request::Asking,       Command::Asking,        "*1\r\n$6\r\nasking\r\n"},
    {Request::Readonly,     Command::Readonly,      "*1\r\n$8\r\nreadonly\r\n"},
    {Request::UnwatchServ,  Command::UnwatchServ,   "*1\r\n$7\r\nunwatch\r\n"},
    {Request::DiscardServ,  Command::DiscardServ,   "*1\r\n$7\r\ndiscard\r\n"},
    {Request::MgetHead,     Command::Mget,          "*2\r\n$4\r\nmget\r\n"},
    {Request::MsetHead,     Command::Mset,          "*3\r\n$4\r\nmset\r\n"},
    {Request::MsetnxHead,   Command::Msetnx,        "*3\r\n$6\r\nmsetnx\r\n"},
    {Request::TouchHead,    Command::Touch,         "*2\r\n$5\r\ntouch\r\n"},
    {Request::ExistsHead,   Command::Exists,        "*2\r\n$6\r\nexists\r\n"},
    {Request::DelHead,      Command::Del,           "*2\r\n$3\r\ndel\r\n"},
    {Request::UnlinkHead,   Command::Unlink,        "*2\r\n$6\r\nunlink\r\n"},
    {Request::PsubscribeHead,Command::Psubscribe,   "*2\r\n$10\r\npsubscribe\r\n"},
    {Request::SubscribeHead,Command::Subscribe,     "*2\r\n$9\r\nsubscribe\r\n"},
    {Request::PunsubscribeHead,Command::Punsubscribe,   "*2\r\n$12\r\npunsubscribe\r\n"},
    {Request::UnsubscribeHead,Command::Unsubscribe,     "*2\r\n$11\r\nunsubscribe\r\n"}
};

thread_local static Request* GenericRequests[Request::CodeSentinel];

void Request::init()
{
    BufferPtr buf = BufferAlloc::create();
    for (auto& r : GenericRequestDefs) {
        Request* req = new Request();
        req->mType= r.type;
        if (buf->room() < (int)strlen(r.content)) {
            buf = BufferAlloc::create();
        }
        buf = req->mReq.set(buf, r.content);
        GenericRequests[r.code] = req;
    }
}

Request::Request():
    mConn(nullptr),
    mType(Command::None),
    mDone(false),
    mDelivered(false),
    mInline(false),
    mFollowers(0),
    mFollowersDone(0),
    mRedirectCnt(0),
    mCreateTime(Util::elapsedUSec()),
    mData(nullptr)
{
}

Request::Request(AcceptConnection* c):
    mConn(c),
    mType(Command::None),
    mDone(false),
    mDelivered(false),
    mInline(false),
    mFollowers(0),
    mFollowersDone(0),
    mRedirectCnt(0),
    mCreateTime(Util::elapsedUSec()),
    mData(nullptr)
{
}

Request::Request(GenericCode code):
    mConn(nullptr),
    mDone(false),
    mDelivered(false),
    mInline(false),
    mFollowers(0),
    mFollowersDone(0),
    mRedirectCnt(0),
    mCreateTime(Util::elapsedUSec()),
    mData(nullptr)
{
    auto r = GenericRequests[code];
    mType = r->mType;
    mReq = r->mReq;
}

Request::~Request()
{
    clear();
}

void Request::clear()
{
    mRes = nullptr;
    mHead.clear();
    mReq.clear();
    mKey.clear();
    mLeader = nullptr;
}

void Request::set(const RequestParser& p, Request* leader)
{
    mType = p.type();
    if (leader) {
        const Request* r = nullptr;
        switch (mType) {
        case Command::Mget:
            r = GenericRequests[MgetHead];
            break;
        case Command::Mset:
            r = GenericRequests[MsetHead];
            break;
        case Command::Msetnx:
            r = GenericRequests[MsetnxHead];
            break;
        case Command::Touch:
            r = GenericRequests[TouchHead];
            break;
        case Command::Exists:
            r = GenericRequests[ExistsHead];
            break;
        case Command::Del:
            r = GenericRequests[DelHead];
            break;
        case Command::Unlink:
            r = GenericRequests[UnlinkHead];
            break;
        case Command::Psubscribe:
            r = GenericRequests[PsubscribeHead];
            break;
        case Command::Subscribe:
            r = GenericRequests[SubscribeHead];
            break;
        case Command::Punsubscribe:
            r = GenericRequests[PunsubscribeHead];
            break;
        case Command::Unsubscribe:
            r = GenericRequests[UnsubscribeHead];
            break;
        default:
            //should never reach
            abort();
            break;
        }
        mHead = r->mReq;
        mReq = p.request();
        if (leader == this) {
            if (mType == Command::Mset || mType == Command::Msetnx) {
                mFollowers = (p.argNum() - 1) >> 1;
            } else {
                mFollowers = p.argNum() - 1;
            }
        } else {
            mLeader = leader;
        }
    } else {
        mReq = p.request();
    }
    mKey = p.key();
    mInline = p.isInline();
}

void Request::setAuth(const String& password)
{
    mType = Command::AuthServ;
    mHead.clear();
    mReq.fset(nullptr,
              "*2\r\n"
              "$4\r\n"
              "auth\r\n"
              "$%d\r\n"
              "%s\r\n",
              password.length(), password.data() ? password.data() : "");
}

void Request::setSelect(int db)
{
    char buf[16];
    int num = snprintf(buf, sizeof(buf), "%d", db);
    mType = Command::SelectServ;
    mHead.clear();
    mReq.fset(nullptr,
              "*2\r\n"
              "$6\r\n"
              "select\r\n"
              "$%d\r\n"
              "%s\r\n",
              num, buf);
}

void Request::setSentinels(const String& master)
{
    mType = Command::SentinelSentinels;
    mHead.clear();
    mReq.fset(nullptr,
              "*3\r\n"
              "$8\r\n"
              "sentinel\r\n"
              "$9\r\n"
              "sentinels\r\n"
              "$%d\r\n"
              "%.*s\r\n",
              master.length(), master.length(), master.data());
}

void Request::setSentinelGetMaster(const String& master)
{
    mType = Command::SentinelGetMaster;
    mHead.clear();
    mReq.fset(nullptr,
              "*3\r\n"
              "$8\r\n"
              "sentinel\r\n"
              "$23\r\n"
              "get-master-addr-by-name\r\n"
              "$%d\r\n"
              "%.*s\r\n",
              master.length(), master.length(), master.data());
}

void Request::setSentinelSlaves(const String& master)
{
    mType = Command::SentinelSlaves;
    mHead.clear();
    mReq.fset(nullptr,
              "*3\r\n"
              "$8\r\n"
              "sentinel\r\n"
              "$6\r\n"
              "slaves\r\n"
              "$%d\r\n"
              "%.*s\r\n",
              master.length(), master.length(), master.data());
}

void Request::adjustScanCursor(long cursor)
{
    char buf[32];
    int n = snprintf(buf, sizeof(buf), "%ld", cursor);
    if (mHead.empty()) {
        SegmentStr<64> str(mReq);
        const char* p = strchr(str.data(), '$');
        if (!p) {
            return;
        }
        p = strchr(p + 1, '$');
        if (!p) {
            return;
        }
        mHead.fset(nullptr, 
                "%.*s"
                "$%d\r\n"
                "%s\r\n",
                p - str.data(), str.data(), n, buf);
        p = strchr(p, '\r');
        if (!p) {
            return;
        }
        mReq.cut(p - str.data() + 2 + mKey.length() + 2);
    } else {
        SegmentStr<64> str(mHead);
        const char* p = strchr(str.data(), '$');
        if (!p) {
            return;
        }
        p = strchr(p + 1, '$');
        if (!p) {
            return;
        }
        mHead.fset(nullptr, 
                "%.*s"
                "$%d\r\n"
                "%s\r\n",
                p - str.data(), str.data(), n, buf);
    }
}

void Request::follow(Request* leader)
{
    leader->mFollowers += 1;
    if (leader == this) {
        return;
    }
    mType = leader->mType;
    mHead = leader->mHead;
    mReq = leader->mReq;
    mKey = leader->mKey;
    mLeader = leader;
}

bool Request::send(Socket* s)
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
    while (mReq.get(dat, len)) {
        int n = s->write(dat, len);
        if (n > 0) {
            mReq.use(n);
        } else {
            return false;
        }
    }
    return true;
}

int Request::fill(IOVec* vecs, int len)
{
    bool all = false;
    int n = mHead.fill(vecs, len, all);
    if (!all) {
        return n;
    }
    n += mReq.fill(vecs + n, len - n, all);
    if (n > 0 && all) {
        vecs[n - 1].req = this;
    }
    return n;
}

void Request::setResponse(Response* res)
{
    mDone = true;
    if (Request* ld = leader()) {
        ld->mFollowersDone += 1;
        switch (mType) {
        case Command::Mget:
            mRes = res;
            break;
        case Command::Mset:
            if (Response* leaderRes = ld->getResponse()) {
                if (res->isError() && !leaderRes->isError()) {
                    ld->mRes = res;
                }
            } else {
                ld->mRes = res;
            }
            break;
        case Command::Msetnx:
            if (Response* leaderRes = ld->getResponse()) {
                if (!leaderRes->isError() &&
                     (res->isError() || res->integer() == 0)) {
                    ld->mRes = res;
                }
            } else {
                ld->mRes = res;
            }
            break;
        case Command::Touch:
        case Command::Exists:
        case Command::Del:
        case Command::Unlink:
            if (!ld->mRes) {
                ld->mRes = res;
            }
            if (ld->isDone()) {
                ld->mRes->set(ld->mRes->integer());
            }
            break;
        case Command::ScriptLoad:
            if (Response* leaderRes = ld->getResponse()) {
                if (leaderRes->isString() && !res->isString()) {
                    ld->mRes = res;
                }
            } else {
                ld->mRes = res;
            }
            break;
        default:
            //should never reach here
            mRes = res;
            break;
        }
    } else {
        mRes = res;
    }
}

bool Request::isDone() const
{
    if (isLeader()) {
        switch (mType) {
        case Command::Mget:
        case Command::Psubscribe:
        case Command::Subscribe:
        case Command::Punsubscribe:
        case Command::Unsubscribe:
            return mDone;
        default:
            break;
        }
        return mFollowers == mFollowersDone;
    }
    return mDone;
}

