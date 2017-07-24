/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include "Reply.h"

const char* Reply::TypeStr[Sentinel] = {
    "None",
    "Status",
    "Err",
    "Str",
    "Int",
    "Array"
};
