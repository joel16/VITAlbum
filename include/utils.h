#pragma once

/// Checks whether a result code indicates success.
#define R_SUCCEEDED(res)   ((res)>=0)
/// Checks whether a result code indicates failure.
#define R_FAILED(res)      ((res)<0)
/// Returns the level of a result code.

namespace Utils {
    int InitAppUtil(void);
    int EndAppUtil(void);
    void GetSizeString(char *string, double size);
}
