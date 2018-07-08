/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <iostream>
#include <thread>
#include "Proxy.h"
#include "Handler.h"
#include "Socket.h"
#include "Alloc.h"
#include "ListenSocket.h"
#include "AcceptSocket.h"
#include "RequestParser.h"
#include "Backtrace.h"

static bool Running = false;
static bool Abort = false;
static bool Stop = false;

static void abortHandler(int sig)
{
    if (!Abort) {
        traceInfo(sig);
    }
    Abort = true;
    if (!Running) {
        abort();
    }
}

static void stopHandler(int sig)
{
    Stop = true;
    if (!Running) {
        abort();
    }
}

Proxy::Proxy():
    mListener(nullptr),
    mDataCenter(nullptr),
    mServPool(nullptr),
    mStartTime(time(nullptr)),
    mStatsVer(0)
{
}

Proxy::~Proxy()
{
    for (auto h : mHandlers) {
        delete h;
    }
    delete mServPool;
    delete mDataCenter;
    delete mListener;
    delete mConf;
}

bool Proxy::init(int argc, char* argv[])
{
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGFPE, abortHandler);
    signal(SIGILL, abortHandler);
    signal(SIGSEGV, abortHandler);
    signal(SIGABRT, abortHandler);
    signal(SIGBUS, abortHandler);
    signal(SIGQUIT, abortHandler);
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    Command::init();
    mConf = new Conf();
    if (!mConf->init(argc, argv)) {
        return false;
    }
    Logger::gInst = new Logger();
    Logger::gInst->setLogFile(mConf->log(), mConf->logRotateSecs(), mConf->logRotateBytes());
    Logger::gInst->setAllowMissLog(mConf->allowMissLog());
    for (int i = 0; i < LogLevel::Sentinel; ++i) {
        LogLevel::Type lvl = LogLevel::Type(i);
        Logger::gInst->setLogSample(lvl, mConf->logSample(lvl));
    }
    Logger::gInst->start();
    for (auto& ac : mConf->authConfs()) {
        mAuthority.add(ac);
    }
    if (!mConf->localDC().empty()) {
        mDataCenter = new DataCenter();
        mDataCenter->init(mConf);
    }
    AllocBase::setMaxMemory(mConf->maxMemory());
    if (mConf->bufSize() > 0) {
        Buffer::setSize(mConf->bufSize());
    }

    mLatencyMonitorSet.init(mConf->latencyMonitors());
    ListenSocket* s = new ListenSocket(mConf->bind(), SOCK_STREAM);
    if (!s->setNonBlock()) {
        logError("proxy listener set nonblock fail:%s", StrError());
        Throw(InitFail, "listener set nonblock", StrError());
    }
    s->listen();
    mListener = s;
    logNotice("predixy listen in %s", mConf->bind());
    switch (mConf->serverPoolType()) {
    case ServerPool::Cluster:
        {
            ClusterServerPool* p = new ClusterServerPool(this);
            p->init(mConf->clusterServerPool());
            mServPool = p;
        }
        break;
    case ServerPool::Standalone:
        {
            StandaloneServerPool* p = new StandaloneServerPool(this);
            p->init(mConf->standaloneServerPool());
            mServPool = p;
        }
        break;
    default:
        Throw(InitFail, "unknown server pool type");
        break;
    }
    for (int i = 0; i < mConf->workerThreads(); ++i) {
        Handler* h = new Handler(this);
        mHandlers.push_back(h);
    }
    return true;
}

int Proxy::run()
{
    logNotice("predixy running with Name:%s Workers:%d",
            mConf->name(),
            (int)mHandlers.size());
    std::vector<std::shared_ptr<std::thread>> tasks;
    for (auto h : mHandlers) {
        std::shared_ptr<std::thread> t(new std::thread([=](){h->run();}));
        tasks.push_back(t);
    }
    Running = true;
    bool stop = false;
    while (!stop) {
        if (Abort) {
            stop = true;
            abort();
        } else if (Stop) {
            fprintf(stderr, "predixy will quit ASAP Bye!\n");
            stop = true;
        }
        if (!stop) {
            sleep(1);
            TimerPoint::report();
        }
    }
    for (auto h : mHandlers) {
        h->stop();
    }
    for (auto t : tasks) {
        t->join();
    }
    Logger::gInst->stop();
    TimerPoint::report();
    if (*mConf->bind() == '/') {
        unlink(mConf->bind());
    }
    return 0;
}

