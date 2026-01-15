/*
 * Compile-only test to ensure Logger::mStop is atomic.
 */

#define private public
#include "../src/Logger.h"
#undef private

#include <type_traits>
#include <utility>

int main() {
    using StopType = decltype(std::declval<Logger>().mStop);
    static_assert(std::is_same<StopType, Atomic<bool>>::value,
                  "Logger::mStop should be atomic");
    return 0;
}
