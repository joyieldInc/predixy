/*
 * Minimal test for Buffer::fappend negative vsnprintf handling.
 */

#include "../src/Buffer.h"
#include "../src/Logger.h"

int main() {
    Logger::gInst = nullptr;
    Buffer* buf = BufferAlloc::create();
    int before = buf->length();
    Buffer* ret = buf->fappend("%");
    if (ret && ret->length() < before) {
        return 1;
    }
    if (buf->length() < before) {
        return 2;
    }
    return 0;
}
