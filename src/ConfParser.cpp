/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <memory>
#include "ConfParser.h"

ConfParser::ConfParser(int maxNodeDepth):
    mMaxNodeDepth(maxNodeDepth)
{
    mRoot.next = mRoot.sub = nullptr;
}

ConfParser::~ConfParser()
{
    clear();
}

void ConfParser::clear()
{
    if (mRoot.next) {
        delete mRoot.next;
    }
    if (mRoot.sub) {
        delete mRoot.sub;
    }
    mRoot.next = nullptr;
    mRoot.sub = nullptr;
    for (auto s : mStrings) {
        delete s;
    }
    mStrings.clear();
}

std::string* ConfParser::getFile(const std::string& name, const char* parent)
{
    if (name.empty()) {
        Throw(EmptyFileName, "filename is empty");
    }
    if (name[0] == '/') {
        return new std::string(name);
    }
    std::string ret;
    char buf[PATH_MAX];
    if (parent && *parent) {
        int len = strlen(parent);
        if (len < (int)sizeof(buf)) {
            int n = len - 1;
            while (n >= 0) {
                if (parent[n] == '/') {
                    break;
                }
                --n;
            }
            if (n >= 0) {
                ret.assign(parent, 0, n + 1);
            }
        } else {
            Throw(FileNameTooLong, "parent file name %s too long", parent);
        }
    } else {
        if (char* path = getcwd(buf, sizeof(buf))) {
            ret = path;
            ret += '/';
        } else {
            Throw(GetCwdFail, "get current directory fail %s", StrError());
        }
    }
    ret += name;
    std::string* r = new std::string();
    r->swap(ret);
    return r;
}

struct File
{
    const char* name;
    std::shared_ptr<std::ifstream> stream;
    int line;
};

ConfParser::Node* ConfParser::load(const char* file)
{
    clear();
    std::string* name = getFile(file, nullptr);
    mStrings.push_back(name);
    std::shared_ptr<std::ifstream> s(new std::ifstream(name->c_str()));
    if (!s->good()) {
        Throw(OpenFileFail, "open file %s fail %s", name->c_str(), StrError());
    }
    std::vector<File> files;
    files.push_back(File{name->c_str(), s, 0});
    std::string line;
    std::string key;
    std::string val;
    std::vector<Node*> stack;
    Node* node = &mRoot;
    while (!files.empty()) {
        File* f = &files.back();
        while (std::getline(*f->stream, line)) {
            f->line += 1;
            Status st = parse(line, key, val);
            switch (st) {
            case None:
                break;
            case KeyVal:
                if (strcasecmp(key.c_str(), "Include") == 0) {
                    name = getFile(val, f->name);
                    mStrings.push_back(name);
                    for (auto& pre : files) {
                        if (strcmp(pre.name, name->c_str()) == 0) {
                            Throw(IncludeFileDuplicate, "include file %s duplicate in %s:%d",
                                name->c_str(), f->name, f->line);
                        }
                    }
                    s = decltype(s)(new std::ifstream(name->c_str()));
                    if (!s->good()) {
                        Throw(OpenFileFail, "open file %s fail %s", name->c_str(), StrError());
                    }
                    files.push_back(File{name->c_str(), s, 0});
                    f = &files.back();
                } else {
                    auto n = new Node{f->name, f->line, key, val, nullptr, nullptr};
                    if (node) {
                        node->next = n;
                    } else if (!stack.empty()) {
                        stack.back()->sub = n;
                    }
                    node = n;
                }
                break;
            case BeginScope:
                if ((int)stack.size() == mMaxNodeDepth) {
                    Throw(NodeDepthTooLong, "node depth limit %d in %s:%d",
                            mMaxNodeDepth, f->name, f->line);
                } else {
                    auto n = new Node{f->name, f->line, key, val, nullptr, nullptr};
                    if (node) {
                        node->next = n;
                    } else if (!stack.empty()) {
                        stack.back()->sub = n;
                    }
                    stack.push_back(n);
                    node = nullptr;
                }
                break;
            case EndScope:
                if (stack.empty()) {
                    Throw(ParseError, "parse error unmatched end scope in %s:%d", f->name, f->line);
                } else {
                    node = stack.back();
                    stack.resize(stack.size() - 1);
                }
                break;
            case Error:
            default:
                Throw(ParseError, "parse error in %s:%d", f->name, f->line);
            }
        }
        if (f->stream->eof()) {
            files.resize(files.size() - 1);
        } else if (f->stream->bad()) {
            Throw(FileReadError, "read file %s error", f->name);
        }
    }
    if (!stack.empty()) {
        Throw(ParseError, "parse error some scope no end %s:%d",
                stack.back()->file, stack.back()->line);
    }
    return mRoot.next;
}

ConfParser::Status ConfParser::parse(std::string& line, std::string& key, std::string& val)
{
    key.clear();
    val.clear();
    State s = KeyReady;
    std::string::size_type i = 0;
    std::string::size_type pos = 0;
    bool escape = false;
    for (i = 0; i < line.size(); ++i) {
        char c = line[i];
        switch (s) {
        case KeyReady:
            if (c == '#' || c == '\n') {
                goto Done;
            } else if (c != ' ' && c != '\t' && c != '\r') {
                pos = i;
                s = KeyBody;
            }
            break;
        case KeyBody:
            if (c == '#' || c == '\n') {
                line.resize(i);
                goto Done;
            } else if (c == ' ' || c == '\t') {
                key.assign(line, pos, i - pos);
                s = ValReady;
            }
            break;
        case ValReady:
            if (c == '#' || c == '\n') {
                goto Done;
            } else if (c != ' ' && c != '\t' && c != '\r') {
                pos = i;
                if (c == '"') {
                    val.resize(line.size() - i);
                    val.resize(0);
                    s = SValBody;
                } else {
                    s = VValBody;
                }
            }
            break;
        case SValBody:
            if (c == '\\') {
                if (escape) {
                    escape = false;
                    val += c;
                } else {
                    escape = true;
                }
            } else if (c == '"') {
                if (escape) {
                    escape = false;
                    val += c;
                } else {
                    s = ScopeReady;
                }
            } else {
                val += c;
            }
            break;
        case VValBody:
            if (c == '#' || c == '\n') {
                line.resize(i);
                goto Done;
            }
            break;
        case ScopeReady:
            if (c == '#' || c == '\n') {
                goto Done;
            } else if (c == '{') {
                s = ScopeBody;
            } else if (c != ' ' && c != '\t' && c != '\r') {
                return Error;
            }
            break;
        case ScopeBody:
            if (c == '#' || c == '\n') {
                goto Done;
            } else if (c != ' ' && c != '\t' && c != '\r') {
                return Error;
            }
        default:
            break;
        }
    }
Done:
    switch (s) {
    case KeyReady:
        break;
    case KeyBody:
        key.assign(line, pos, line.size() - pos);
        if (key.size() == 1 && key[0] == '}') {
            return EndScope;
        }
        return KeyVal;
    case ValReady:
        if (key.size() == 1 && key[0] == '}') {
            return EndScope;
        }
        return KeyVal;
    case SValBody:
        return KeyVal;
    case VValBody:
    {
        auto ret = KeyVal;
        val.assign(line, pos, line.size() - pos);
        if (val.back() == '{') {
            val.resize(val.size() - 1);
            ret = BeginScope;
        }
        int vsp = 0;
        for (auto it = val.rbegin(); it != val.rend(); ++it) {
            if (isspace(*it)) {
                ++vsp;
            }
        }
        val.resize(val.size() - vsp);
        return ret;
    }
    case ScopeReady:
        return KeyVal;
    case ScopeBody:
        return BeginScope;
    default:
        return Error;
    }
    return None;
}
