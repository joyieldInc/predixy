/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include <string.h>
#include "ClusterNodesParser.h"


ClusterNodesParser::ClusterNodesParser():
    mState(Idle),
    mLen(0)
{
}

ClusterNodesParser::~ClusterNodesParser()
{
}

void ClusterNodesParser::set(const Segment& s)
{
    mNodes = s;
    mNodes.rewind();
}

ClusterNodesParser::Status ClusterNodesParser::parse()
{
    if (mState == SlotBegin || mState == SlotEnd) {
        mSlotBegin = mSlotEnd = -1;
        mState = SlotBegin;
    }
    const char* dat;
    int len;
    while (mNodes.get(dat, len)) {
        int n = 0;
        bool node = false;
        while (n < len && !node) {
            char c = dat[n++];
            switch (mState) {
            case Idle:
                if (c == '$') {
                    mState = Len;
                } else {
                    return Error;
                }
                break;
            case Len:
                if (c >= '0' && c <= '9') {
                    mLen = mLen * 10 + (c - '0');
                } else if (c == '\r') {
                    mState = LenLF;
                } else {
                    return Error;
                }
                break;
            case LenLF:
                if (c == '\n') {
                    mState = NodeStart;
                } else {
                    return Error;
                }
                break;
            case NodeStart:
                if (c == '\r') {
                    return Finished;
                }
                mState = Field;
                mRole = Server::Unknown;
                mFieldCnt = 0;
                mNodeId.clear();
                mAddr.clear();
                mFlags.clear();
                mMaster.clear();
                mSlotBegin = -1;
                mSlotEnd = -1;
                //break;***NO break***, continue to Field
            case Field:
                if (c == ' ') {
                    if (mFieldCnt == Flags) {
                        if (strstr(mFlags, "master")) {
                            mRole = Server::Master;
                        } else if (strstr(mFlags, "slave")) {
                            mRole = Server::Slave;
                        }
                        if (strstr(mFlags, "noaddr")) {
                            mAddr.clear();
                        }
                    }
                    if (++mFieldCnt == Slot) {
                        mState = SlotBegin;
                     }
                } else if (c == '\n') {
                    node = true;
                    mState = NodeStart;
                } else {
                    switch (mFieldCnt) {
                    case NodeId:
                        if (!mNodeId.append(c)) {
                            return Error;
                        }
                        break;
                    case Addr:
                        if (!mAddr.append(c)) {
                            return Error;
                        }
                        break;
                    case Flags:
                        if (!mFlags.append(c)) {
                            return Error;
                        }
                        break;
                    case Master:
                        if (!mMaster.append(c)) {
                            return Error;
                        }
                        break;
                    default:
                        break;
                    }
                }
                break;
            case SlotBegin:
                if (c >= '0' && c <= '9') {
                    if (mSlotBegin < 0) {
                        mSlotBegin = c - '0';
                    } else {
                        mSlotBegin = mSlotBegin * 10 + (c - '0');
                    }
                } else if (c == '-') {
                    mState = SlotEnd;
                    mSlotEnd = 0;
                } else if (c == '[') {
                    mState = SlotMove;
                } else if (c == ' ') {
                    mSlotEnd = mSlotBegin + 1;
                    node = true;
                } else if (c == '\n') {
                    mSlotEnd = mSlotBegin + 1;
                    node = true;
                    mState = NodeStart;
                } else {
                    return Error;
                }
                break;
            case SlotEnd:
                if (c >= '0' && c <= '9') {
                    mSlotEnd = mSlotEnd * 10 + (c - '0');
                } else if (c == ' ') {
                    ++mSlotEnd;
                    node = true;
                    mState = SlotBegin;
                } else if (c == '\n') {
                    ++mSlotEnd;
                    node = true;
                    mState = NodeStart;
                } else {
                    return Error;
                }
                break;
            case SlotMove:
                if (c == ' ') {
                    mState = SlotBegin;
                    mSlotBegin = mSlotEnd = -1;
                } else if (c == '\n') {
                    node = true;
                    mState = NodeStart;
                }
                break;
            default:
                return Error;
            }
        }
        mNodes.use(n);
        if (node) {
            return Node;
        }
    }
    return Error;
}
