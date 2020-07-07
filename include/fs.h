#ifndef _VITASTUDIO_FS_H_
#define _VITASTUDIO_FS_H_

#include <psp2/io/dirent.h>
#include <psp2/types.h>
#include <string>

namespace FS {
    extern std::string CWD;
    int GetFileSize(const std::string &path, SceOff *size);
    std::string GetFileExt(const std::string &filename);
    SceOff GetDirList(const std::string &path, SceIoDirent **entriesp);
    SceOff ChangeDirNext(const std::string &path, SceIoDirent **entries);
    SceOff ChangeDirPrev(SceIoDirent **entries);
    const std::string BuildPath(SceIoDirent *entry);
    int ReadFile(const std::string &path, unsigned char **buffer, SceOff *size);
}

#endif
