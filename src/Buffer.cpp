/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include "String.h"
#include "Logger.h"
#include "Buffer.h"
#include "IOVec.h"

int Buffer::Size = 4096 - sizeof(Buffer);

Buffer::Buffer():
    mLen(0)
{
}

Buffer::Buffer(const Buffer& oth):
    mLen(oth.mLen)
{
    memcpy(mDat, oth.mDat, mLen);
}

Buffer::~Buffer()
{
    SharePtr<Buffer> p = ListNode<Buffer, SharePtr<Buffer>>::next();
    ListNode<Buffer, SharePtr<Buffer>>::reset();
    while (p) {
        if (p->count() == 1) {
            p = p->next();
        } else {
            break;
        }
    }
}

Buffer& Buffer::operator=(const Buffer& oth)
{
    if (this != &oth) {
        mLen = oth.mLen;
        memcpy(mDat, oth.mDat, mLen);
    }
    return *this;
}

Buffer* Buffer::append(const char* str)
{
    return append(str, strlen(str));
}

Buffer* Buffer::append(const char* dat, int len)
{
    if (len <= 0) {
        return this;
    }
    Buffer* buf = this;
    while (len > 0) {
        if (len <= buf->room()) {
            memcpy(buf->tail(), dat, len);
            buf->use(len);
            break;
        } else {
            int n = buf->room();
            memcpy(buf->tail(), dat, n);
            buf->use(n);
            dat += n;
            len -= n;
            Buffer* nbuf = BufferAlloc::create();
            buf->concat(nbuf);
            buf = nbuf;
        }
    }
    return buf;
}

Buffer* Buffer::fappend(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char* dat = tail();
    int len = room();
    int n = vsnprintf(dat, len, fmt, ap);
    va_end(ap);
    if (n >= len) {
        if (n > MaxBufFmtAppendLen) {
            return nullptr;
        }
        char buf[MaxBufFmtAppendLen];
        va_list aq;
        va_start(aq, fmt);
        n = vsnprintf(buf, sizeof(buf), fmt, aq);
        va_end(aq);
        return append(buf, n);
    }
    use(n);
    return this;
}

Buffer* Buffer::vfappend(const char* fmt, va_list ap)
{
    char* dat = tail();
    int len = room();
    va_list aq;
    va_copy(aq, ap);
    int n = vsnprintf(dat, len, fmt, ap);
    if (n >= len) {
        if (n > MaxBufFmtAppendLen) {
            va_end(aq);
            return nullptr;
        }
        char buf[MaxBufFmtAppendLen];
        n = vsnprintf(buf, sizeof(buf), fmt, aq);
        va_end(aq);
        return append(buf, n);
    }
    use(n);
    va_end(aq);
    return this;
}

Segment::Segment()
{
}

Segment::Segment(BufferPtr beginBuf, int beginPos, BufferPtr endBuf, int endPos):
    mBegin(beginBuf, beginPos),
    mCur(beginBuf, beginPos),
    mEnd(endBuf, endPos)
{
}

Segment::Segment(const Segment& oth):
    mBegin(oth.mBegin),
    mCur(oth.mCur),
    mEnd(oth.mEnd)
{
}

Segment::Segment(Segment&& oth):
    mBegin(oth.mBegin),
    mCur(oth.mCur),
    mEnd(oth.mEnd)
{
}

Segment::~Segment()
{
    clear();
}

Segment& Segment::operator=(const Segment& oth)
{
    if (this != &oth) {
        mBegin = oth.mBegin;
        mCur = oth.mCur;
        mEnd = oth.mEnd;
    }
    return *this;
}

void Segment::clear()
{
    mBegin.buf = nullptr;
    mBegin.pos = 0;
    mCur.buf = nullptr;
    mCur.pos = 0;
    mEnd.buf = nullptr;
    mEnd.pos = 0;
}

Buffer* Segment::set(Buffer* buf, const char* dat, int len)
{
    if (!buf) {
        buf = BufferAlloc::create();
    }
    int pos = buf->length();
    Buffer* nbuf = buf->append(dat, len);
    mBegin.buf = buf;
    mBegin.pos = pos;
    mCur.buf = buf;
    mCur.pos = pos;
    mEnd.buf = nbuf;
    mEnd.pos = nbuf->length();
    return nbuf;
}

Buffer* Segment::fset(Buffer* buf, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    try {
        return vfset(buf, fmt, ap);
    } catch (...) {
        va_end(ap);
        throw;
    }
    return nullptr;
}

Buffer* Segment::vfset(Buffer* buf, const char* fmt, va_list ap)
{
    if (!buf) {
        buf = BufferAlloc::create();
    }
    int pos = buf->length();
    Buffer* nbuf = buf->vfappend(fmt, ap);
    mBegin.buf = buf;
    mBegin.pos = pos;
    mCur = mBegin;
    mEnd.buf = nbuf;
    mEnd.pos = nbuf->length();
    return nbuf;
}

int Segment::length() const
{
    Buffer* buf = mBegin.buf;
    int pos = mBegin.pos;
    int len = 0;
    while (buf) {
        if (buf == mEnd.buf) {
            len += mEnd.pos - pos;
            break;
        } else {
            len += buf->length() - pos;
        }
        buf = buf->next();
        pos = 0;
    }
    return len;
}

bool Segment::get(const char*& dat, int& len) const
{
    if (mCur.buf == mEnd.buf && mCur.pos >= mEnd.pos) {
        dat = nullptr;
        len = 0;
        return false;
    }
    dat = mCur.buf->data() + mCur.pos;
    len = (mCur.buf == mEnd.buf ? mEnd.pos : mCur.buf->length()) - mCur.pos;
    return true;
}

int Segment::fill(IOVec* vecs, int len, bool& all) const
{
    int n = 0;
    BufferPos p = mCur;
    all = false;
    while (n < len && p != mEnd) {
        vecs[n].dat = p.buf->data() + p.pos;
        vecs[n].len = (p.buf == mEnd.buf ? mEnd.pos : p.buf->length()) - p.pos;
        vecs[n].seg = const_cast<Segment*>(this);
        vecs[n].buf = p.buf;
        vecs[n].pos = p.pos;
        vecs[n].req = nullptr;
        ++n;
        if (p.buf == mEnd.buf) {
            all = true;
            break;
        } else {
            p.buf = p.buf->next();
            p.pos = 0;
        }
    }
    if (n < len && n == 0) {
        all = true;
    }
    return n;
}

//caller must sure cnt < segment length
void Segment::cut(int cnt)
{
    if (cnt > 0) {
        while (cnt > 0 && mBegin.buf) {
            BufferPtr buf = mBegin.buf;
            int len = (buf == mEnd.buf ? mEnd.pos : buf->length()) - mBegin.pos;
            if (len <= cnt) {
                if (buf == mEnd.buf) {
                    mBegin.buf = nullptr;
                    mBegin.pos = 0;
                    mCur = mEnd = mBegin;
                    break;
                }
                cnt -= len;
                mBegin.buf = buf->next();
                mBegin.pos = 0;
                if (mCur.buf == buf) {
                    mCur = mBegin;
                }
            } else {
                mBegin.pos += cnt;
                if (mCur.buf == mBegin.buf && mCur.pos < mBegin.pos) {
                    mCur.pos = mBegin.pos;
                }
                break;
            }
        }
    }
}

//caller must sure cnt < segment length
void Segment::use(int cnt)
{
    while (cnt > 0) {
        if (mCur.buf == mEnd.buf) {
            mCur.pos += cnt;
            if (mCur.pos > mEnd.pos) {
                mCur.pos = mEnd.pos;
            }
            break;
        } else {
            int n = mCur.buf->length() - mCur.pos;
            if (n <= cnt) {
                mCur.buf = mCur.buf->next();
                mCur.pos = 0;
            } else {
                mCur.pos += cnt;
            }
            cnt -= n;
        }
    }
}

void Segment::seek(Buffer* buf, int pos, int cnt)
{
    mCur.buf = buf;
    mCur.pos = pos;
    use(cnt);
}

int Segment::dump(char* dat, int len) const
{
    int cnt = 0;
    BufferPtr buf = mBegin.buf;
    int pos = mBegin.pos;
    bool end = false;
    while (buf && !end) {
        int num;
        if (buf == mEnd.buf) {
            num = mEnd.pos - pos;
            end = true;
        } else {
            num = buf->length() - pos;
        }
        if (dat && len > 0) {
            memcpy(dat, buf->data() + pos, len > num ? num : len);
            dat += num;
            len -= num;
        }
        cnt += num;
        buf = buf->next();
        pos = 0;
    }
    return cnt;
}

bool Segment::hasPrefix(const char* prefix) const
{
    const char* p = prefix;
    BufferPtr buf = mBegin.buf;
    int pos = mBegin.pos;
    bool end = false;
    while (buf && !end) {
        int last = buf->length();
        if (buf == mEnd.buf) {
            last = mEnd.pos;
            end = true;
        }
        while (pos < last && *p) {
            if (buf->data()[pos++] != *p++) {
                return false;
            }
        }
        buf = buf->next();
        pos = 0;
    }
    return *p == '\0';
}
