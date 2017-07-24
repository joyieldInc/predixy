/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_CONF_PARSER_H_
#define _PREDIXY_CONF_PARSER_H_

#include <string.h>
#include <string>
#include <set>
#include <vector>
#include <fstream>
#include "Exception.h"
#include "Util.h"

class ConfParser
{
public:
    DefException(EmptyFileName);
    DefException(FileNameTooLong);
    DefException(GetCwdFail);
    DefException(OpenFileFail);
    DefException(IncludeFileDuplicate);
    DefException(NodeDepthTooLong);
    DefException(FileReadError);
    DefException(ParseError);
    struct Node
    {
        const char* file;
        int line;
        std::string key;
        std::string val;
        Node* next;
        Node* sub;

        ~Node()
        {
            if (sub) {
                delete sub;
            }
            Node* n = next;
            while (n) {
                Node* nn = n->next;
                n->next = nullptr;
                delete n;
                n = nn;
            }
        }
    };

private:
    enum Status
    {
        None,
        KeyVal,
        BeginScope,
        EndScope,
        Error
    };
    enum State
    {
        KeyReady,
        KeyBody,
        ValReady,
        SValBody,
        VValBody,
        ScopeReady,
        ScopeBody
    };
public:
    ConfParser(int maxNodeDepth = 10);
    ~ConfParser();
    void clear();
    Node* load(const char* file);
private:
    static std::string* getFile(const std::string& name, const char* parent);
    Status parse(std::string& line, std::string& key, std::string& val);
private:
    Node mRoot;
    int mMaxNodeDepth;
    std::vector<std::string*> mStrings;
};

#endif
