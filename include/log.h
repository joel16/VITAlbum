#pragma once

namespace Log {
    int Init(void);
    int Exit(void);
    void Error(const char *format, ...);
}
