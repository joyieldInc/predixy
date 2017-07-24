/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _CLUSTER_NODES_PARSER_H_
#define _CLUSTER_NODES_PARSER_H_

#include "Buffer.h"
#include "String.h"
#include "Server.h"

class ClusterNodesParser
{
public:
    enum Status
    {
        Error,
        Node,
        Finished
    };
public:
    ClusterNodesParser();
    ~ClusterNodesParser();
    void set(const Segment& s);
    Status parse();
    void rewind()
    {
        mNodes.rewind();
    }
    Server::Role role() const
    {
        return mRole;
    }
    const String& nodeId() const
    {
        return mNodeId;
    }
    const String& addr() const
    {
        return mAddr;
    }
    const String& flags() const
    {
        return mFlags;
    }
    const String& master() const
    {
        return mMaster;
    }
    bool getSlot(int& begin, int& end) const
    {
        begin = mSlotBegin;
        end = mSlotEnd;
        return begin >= 0 && begin < end;
    }
private:
    enum State
    {
        Idle,
        Len,
        LenLF,
        NodeStart,
        Field,
        SlotBegin,
        SlotEnd,
        SlotMove,
        NodeLF
    };
    enum FieldType
    {
        NodeId,
        Addr,
        Flags,
        Master,
        PingSent,
        PongRecv,
        ConfigEpoch,
        LinkState,
        Slot
    };
    static const int NodeIdLen = 48;
    static const int AddrLen = 32;
    static const int FlagsLen = 48;
private:
    Segment mNodes;
    State mState;
    int mLen;
    Server::Role mRole;
    int mFieldCnt;
    SString<NodeIdLen> mNodeId;
    SString<AddrLen> mAddr;
    SString<FlagsLen> mFlags;
    SString<NodeIdLen> mMaster;
    int mSlotBegin;
    int mSlotEnd;
};

#endif
