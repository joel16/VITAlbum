#ifndef _VITASTUDIO_LOG_H_
#define _VITASTUDIO_LOG_H_

namespace Log {
    int OpenHande(void);
    int CloseHandle(void);
    int Debug(const char *format, ...);
}

#endif
