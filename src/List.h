/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_LIST_H_
#define _PREDIXY_LIST_H_

template<class T, class P = T*, int Size = 1>
class ListNode
{
public:
    typedef P Ptr;
    ListNode()
    {
        for (int i = 0; i < Size; ++i) {
            mNext[i] = nullptr;
        }
    }
    ~ListNode()
    {
        for (int i = 0; i < Size; ++i) {
            mNext[i] = nullptr;
        }
    }
    void reset(int idx = 0)
    {
        mNext[idx] = nullptr;
    }
    void concat(T* obj, int idx = 0)
    {
        mNext[idx] = obj;
    }
    P next(int idx = 0) const
    {
        return mNext[idx];
    }
private:
    P mNext[Size];
};

template<class N, int Idx = 0>
class List
{
public:
    typedef typename N::Value T;
    typedef typename N::ListNodeType Node;
    typedef typename Node::Ptr P;
public:
    List():
        mSize(0),
        mHead(nullptr),
        mTail(nullptr)
    {
    }
    ~List()
    {
        while (mSize > 0) {
            pop_front();
        }
    }
    P next(T* obj)
    {
        return node(obj)->next(Idx);
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
    P pop_front()
    {
        P obj = mHead;
        if (obj) {
            Node* n = node((T*)obj);
            mHead = n->next(Idx); 
            if (--mSize == 0) {
                mTail = nullptr;
            }
            n->reset(Idx);
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
