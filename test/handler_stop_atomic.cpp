/*
 * Compile-only test to ensure Handler::mStop is atomic.
 */

#define private public
#include "../src/Handler.h"
#undef private

#include <type_traits>
#include <utility>

int main() {
    using StopType = decltype(std::declval<Handler>().mStop);
    static_assert(std::is_same<StopType, Atomic<bool>>::value,
                  "Handler::mStop should be atomic");
    return 0;
}
