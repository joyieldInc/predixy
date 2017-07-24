/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include <iostream>
#include <exception>
#include "Proxy.h"

int main(int argc, char* argv[])
{
    try {
        Proxy p;
        if (!p.init(argc, argv)) {
            return 0;
        }
        return p.run();
    } catch (std::exception& e) {
        fprintf(stderr, "predixy running exception:%s\n", e.what());
    }
    return 1;
}
