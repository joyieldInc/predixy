/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include "Alloc.h"

long AllocBase::MaxMemory(0);
AtomicLong AllocBase::UsedMemory(0);
