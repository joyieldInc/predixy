/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_DEQUE_H_
#define _PREDIXY_DEQUE_H_


template<class T, class P = T*, int Size = 1>
class DequeNode
{
public:
    typedef P Ptr;
    DequeNode()
    {
        for (int i = 0; i < Size; ++i) {
            mPrev[i] = nullptr;
            mNext[i] = nullptr;
        }
    }
    ~DequeNode()
    {
        for (int i = 0; i < Size; ++i) {
            mPrev[i] = nullptr;
            mNext[i] = nullptr;
        }
    }
    void reset(int idx)
    {
        mPrev[idx] = nullptr;
        mNext[idx] = nullptr;
    }
    void resetPrev(int idx)
    {
        mPrev[idx] = nullptr;
    }
    void resetNext(int idx)
    {
        mNext[idx] = nullptr;
    }
    void concat(T* obj, int idx)
    {
        mNext[idx] = obj;
        static_cast<DequeNode*>(obj)->mPrev[idx] = (T*)this;
    }
    P prev(int idx) const
    {
        return mPrev[idx];
    }
    P next(int idx) const
    {
        return mNext[idx];
    }
private:
    P mPrev[Size];
    P mNext[Size];
};

template<class N, int Idx = 0>
class Deque
{
public:
    typedef typename N::Value T;
    typedef typename N::DequeNodeType Node;
    typedef typename Node::Ptr P;
public:
    Deque():
        mSize(0),
        mHead(nullptr),
        mTail(nullptr)
    {
    }
    ~Deque()
    {
        while (mSize > 0) {
            pop_front();
        }
    }
    P prev(T* obj)
    {
        return node(obj)->prev(Idx);
    }
    P next(T* obj)
    {
        return node(obj)->next(Idx);
    }
    bool exist(T* obj) const
    {
        auto n = node(obj);
        return n->prev(Idx) != nullptr || n->next(Idx) != nullptr || n == mHead;
    }
    void push_back(T* obj)
    {
        N* p = static_cast<N*>(obj);
        if (mTail) {
            static_cast<Node*>((T*)mTail)->concat(p, Idx);
            mTail = p;
        } else {
            mHead = mTail = p;
        }
        ++mSize;
    }
    void push_front(T* obj)
    {
        N* p = static_cast<N*>(obj);
        if (mHead) {
            node(obj)->concat(mHead, Idx);
            mHead = p;
        } else {
            mHead = mTail = p;
        }
        ++mSize;
    }
    void remove(T* obj)
    {
        auto n = node(obj);
        auto prev = n->prev(Idx);
        auto next = n->next(Idx);
        int exists = 1;
        if (prev && next) {
            node(prev)->concat(next, Idx);
        } else if (prev) {
            node(prev)->resetNext(Idx);
            mTail = prev;
        } else if (next) {
            node(next)->resetPrev(Idx);
            mHead = next;
        } else {
            if (mHead == n) {
                mHead = mTail = nullptr;
            } else {
                exists = 0;
            }
        }
        mSize -= exists;
        n->reset(Idx);
    }
    void move_back(T* obj)
    {
        auto n = node(obj);
        if (auto next = n->next(Idx)) {
            if (auto prev = n->prev(Idx)) {
                node(prev)->concat(next, Idx);
            } else {
                next->resetPrev(Idx);
                mHead = next;
            }
            node(mTail)->concat(obj, Idx);
            mTail = obj;
            n->resetNext(Idx);
        }
    }
    P pop_front()
    {
        P obj = mHead;
        if (obj) {
            remove(obj);
        }
        return obj;
    }
    P pop_back()
    {
        P obj = mTail;
        if (obj) {
            remove(obj);
        }
        return obj;
    }
    P front() const
    {
        return mHead;
    }
    P back() const
    {
        return mTail;
    }
    int size() const
    {
        return mSize;
    }
    bool empty() const
    {
        return mSize == 0;
    }
private:
    static Node* node(T* obj)
    {
        return static_cast<N*>(obj);
    }
private:
    int mSize;
    P mHead;
    P mTail;
};

#endif
