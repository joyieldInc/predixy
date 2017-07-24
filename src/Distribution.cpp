/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include <string.h>
#include <strings.h>
#include "Distribution.h"

struct TypeName
{
    Distribution::Type type;
    const char* name;
};

static TypeName Pairs[] = {
    {Distribution::None,   "none"},
    {Distribution::Modula, "modula"},
    {Distribution::Random, "random"}
};

const char* Distribution::name() const
{
    return Pairs[mType].name;
}

Distribution Distribution::parse(const char* str)
{
    for (auto& i : Pairs) {
        if (strcasecmp(i.name, str) == 0) {
            return i.type;
        }
    }
    return Distribution::None;
}
