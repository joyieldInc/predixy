/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_H_
#define _PREDIXY_H_

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <sys/uio.h>

class Socket;
class ListenSocket;
class AcceptSocket;
class ConnectSocket;
class Buffer;
class Command;
class Reply;
class Auth;
class Authority;
class DC;
class DataCenter;
class RequestParser;
class ResponseParser;
class Request;
class Response;
class AcceptConnection;
class ConnectConnection;
class ConnectConnectionPool;
class Server;
class ServerGroup;
class ServerPool;
struct AuthConf;
struct ServerConf;
struct ServerGroupConf;
struct ServerPoolConf;
struct SentinelServerPoolConf;
struct ClusterServerPoolConf;
class ConfParser;
class Conf;
class Transaction;
class Subscribe;
class Handler;
class Proxy;

#include "Common.h"
#include "Sync.h"
#include "HashFunc.h"
#include "ID.h"
#include "IOVec.h"
#include "List.h"
#include "Deque.h"
#include "Util.h"
#include "String.h"
#include "Timer.h"
#include "Exception.h"
#include "Logger.h"
#include "Alloc.h"
#include "Command.h"
#include "Reply.h"
#include "Multiplexor.h"
#include "Buffer.h"

typedef SharePtr<Request> RequestPtr;
typedef SharePtr<Response> ResponsePtr;
typedef SharePtr<AcceptConnection> AcceptConnectionPtr;
typedef SharePtr<ConnectConnection> ConnectConnectionPtr;


#endif
