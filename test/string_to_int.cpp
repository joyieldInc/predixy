/*
 * Verify String::toInt handles signed values.
 */

#include "../src/String.h"

int main() {
    int v = 0;
    {
        String s("+1", 2);
        bool ok = s.toInt(v);
        if (!ok || v != 1) {
            fprintf(stderr, "toInt(+1) ok=%d v=%d len=%d dat=%c%c\n",
                    (int)ok, v, s.length(),
                    s.data() ? s.data()[0] : '?',
                    s.data() ? s.data()[1] : '?');
            return 1;
        }
    }
    {
        bool ok = String("-1", 2).toInt(v);
        if (!ok || v != -1) {
            fprintf(stderr, "toInt(-1) ok=%d v=%d\n", (int)ok, v);
            return 2;
        }
    }
    {
        bool ok = String("0", 1).toInt(v);
        if (!ok || v != 0) {
            fprintf(stderr, "toInt(0) ok=%d v=%d\n", (int)ok, v);
            return 3;
        }
    }
    {
        bool ok = String("+", 1).toInt(v);
        if (ok) {
            fprintf(stderr, "toInt(+) ok=%d v=%d\n", (int)ok, v);
            return 4;
        }
    }
    {
        bool ok = String("-", 1).toInt(v);
        if (ok) {
            fprintf(stderr, "toInt(-) ok=%d v=%d\n", (int)ok, v);
            return 5;
        }
    }
    return 0;
}
