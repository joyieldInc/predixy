/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include "Enums.h"

const ServerPoolRefreshMethod::TypeName
ServerPoolRefreshMethod::sPairs[3] = {
    {ServerPoolRefreshMethod::None, "none"},
    {ServerPoolRefreshMethod::Fixed, "fixed"},
    {ServerPoolRefreshMethod::Sentinel, "sentinel"},
};

