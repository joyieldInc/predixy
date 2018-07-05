/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_BUFFER_H_
#define _PREDIXY_BUFFER_H_

#include "Common.h"
#include "List.h"
#include "Alloc.h"
#include "Timer.h"
#include "String.h"

class Segment;
class Buffer;
struct IOVec;
typedef SharePtr<Buffer> BufferPtr;

class Buffer :
    public ListNode<Buffer, SharePtr<Buffer>>,
    public RefCntObj<Buffer>
{
public:
    typedef Alloc<Buffer, Const::BufferAllocCacheSize> Allocator;
    static const int MaxBufFmtAppendLen = 8192;
public:
    Buffer& operator=(const Buffer&);
    Buffer* append(const char* str);
    Buffer* append(const char* dat, int len);
    Buffer* fappend(const char* fmt, ...);
    Buffer* vfappend(const char* fmt, va_list ap);
    void reset()
    {
        mLen = 0;
    }
    char* data()
    {
        return mDat;
    }
    const char* data() const
    {
        return mDat;
    }
    int length() const
    {
        return mLen;
    }
    char* tail()
    {
        return mDat + mLen;
    }
    const char* tail() const
    {
        return mDat + mLen;
    }
    int room() const
    {
        return Size - mLen;
    }
    void use(int cnt)
    {
        mLen += cnt;
    }
    bool full() const
    {
        return mLen >= Size;
    }
    bool empty() const
    {
        return mLen == 0;
    }
    static int getSize()
    {
        return Size;
    }
    static void setSize(int sz)
    {
        Size = sz;
    }
private:
    Buffer();
    Buffer(const Buffer&);
    ~Buffer();
private:
    int mLen;
    char mDat[0];

    static int Size;
    friend class Segment;
    friend class Alloc<Buffer, Const::BufferAllocCacheSize>;
};

typedef List<Buffer> BufferList;
typedef Buffer::Allocator BufferAlloc;

template<>
inline int allocSize<Buffer>()
{
    return Buffer::getSize() + sizeof(Buffer);
}

struct BufferPos
{
    BufferPtr buf;
    int pos;

    BufferPos():
        pos(0)
    {
    }
    BufferPos(BufferPtr b, int p):
        buf(b),
        pos(p)
    {
    }
    BufferPos(const BufferPos& oth):
        buf(oth.buf),
        pos(oth.pos)
    {
    }
    BufferPos(BufferPos&& oth):
        buf(oth.buf),
        pos(oth.pos)
    {
    }
    BufferPos& operator=(const BufferPos& oth)
    {
        if (this != &oth) {
            buf = oth.buf;
            pos = oth.pos;
        }
        return *this;
    }
    bool operator==(const BufferPos& oth) const
    {
        return buf == oth.buf && pos == oth.pos;
    }
    bool operator!=(const BufferPos& oth) const
    {
        return !operator==(oth);
    }
};

class Segment
{
public:
    Segment();
    Segment(BufferPtr beginBuf, int beginPos, BufferPtr endBuf, int endPos);
    Segment(const Segment& oth);
    Segment(Segment&& oth);
    ~Segment();
    Segment& operator=(const Segment& oth);
    void clear();
    Buffer* set(Buffer* buf, const char* dat, int len);
    Buffer* fset(Buffer* buf, const char* fmt, ...);
    Buffer* vfset(Buffer* buf, const char* fmt, va_list ap);
    int length() const;
    void cut(int cnt);
    void use(int cnt);
    void seek(Buffer* buf, int pos, int cnt);
    bool get(const char*& dat, int& len) const;
    int fill(IOVec* vecs, int len, bool& all) const;
    int dump(char* dat, int len) const;
    bool hasPrefix(const char* prefix) const;
    Buffer* set(Buffer* buf, const char* str)
    {
        return set(buf, str, strlen(str));
    }
    bool across() const
    {
        return mBegin.buf != mEnd.buf;
    }
    BufferPos& begin()
    {
        return mBegin;
    }
    const BufferPos& begin() const
    {
        return mBegin;
    }
    BufferPos& end()
    {
        return mEnd;
    }
    const BufferPos& end () const
    {
        return mEnd;
    }
    void rewind()
    {
        mCur = mBegin;
    }
    bool empty() const
    {
        return mBegin.buf == mEnd.buf && mBegin.pos == mEnd.pos;
    }
private:
    BufferPos mBegin;
    BufferPos mCur;
    BufferPos mEnd;
};

typedef List<Segment> SegmentList;

template<int Size>
class SegmentStr : public String
{
public:
    SegmentStr(const Segment& seg)
    {
        set(seg);
    }
    void set(const Segment& seg)
    {
        FuncCallTimer();
        if (seg.across()) {
            mDat = mBuf;
            int len = seg.dump(mBuf, Size);
            mLen = len < Size ? len : Size;
        } else {
            mDat = seg.begin().buf->data() + seg.begin().pos;
            mLen = seg.end().pos - seg.begin().pos;
        }
    }
    bool complete() const
    {
        return mDat != mBuf || mLen < Size;
    }
private:
    char mBuf[Size];
};

#endif
