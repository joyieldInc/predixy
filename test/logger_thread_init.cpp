/*
 * Ensure Logger destructor is safe when thread was never started.
 */

#include "../src/Logger.h"

int main() {
    Logger* logger = new Logger();
    delete logger;
    return 0;
}
