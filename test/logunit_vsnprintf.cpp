/*
 * Minimal test for LogUnit::vformat negative vsnprintf handling.
 */

#include "../src/Logger.h"

int main() {
    LogUnit unit;
    unit.format(LogLevel::Info, __FILE__, __LINE__, "%");
    if (unit.length() < 0) {
        return 1;
    }
    return 0;
}
