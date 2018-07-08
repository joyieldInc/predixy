/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_ENUMS_H_
#define _PREDIXY_ENUMS_H_

#include <string.h>
#include <strings.h>
#include "Exception.h"

template<class T>
class EnumBase
{
public:
    DefException(InvalidEnumValue);
    struct TypeName
    {
        int type;
        const char* name;
    };
public:
    EnumBase(int t):
        mType(t)
    {
    }
    int value() const
    {
        return mType;
    }
    bool operator==(const T& t) const
    {
        return t.value() == mType;
    }
    bool operator!=(const T& t) const
    {
        return t.value() != mType;
    }
    const char* name() const
    {
        return T::sPairs[mType].name;
    }
    static T parse(const char* str)
    {
        for (auto& i : T::sPairs) {
            if (strcasecmp(i.name, str) == 0) {
                return T(typename T::Type(i.type));
            }
        }
        Throw(InvalidEnumValue, "invalid enum value:%s", str);
    }
protected:
    int mType;
};

class ServerPoolRefreshMethod : public EnumBase<ServerPoolRefreshMethod>
{
public:
    enum Type
    {
        None,
        Fixed,
        Sentinel
    };
    static const TypeName sPairs[3];
    ServerPoolRefreshMethod(Type t = None):
        EnumBase<ServerPoolRefreshMethod>(t)
    {
    }
};

#endif
