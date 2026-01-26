/*
 * Ensure Logger does not create more LogUnit instances than its capacity.
 */

#include "../src/Logger.h"

int main() {
    Logger logger(1);
    logger.setAllowMissLog(true);
    logger.setLogSample(LogLevel::Info, 1);
    for (int i = 0; i < 1000; ++i) {
        logger.log(LogLevel::Info, __FILE__, __LINE__, "msg %d", i);
    }
    if (logger.logUnitCount() > 1) {
        return 1;
    }
    return 0;
}
