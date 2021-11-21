#ifndef _VITALBUM_FS_H_
#define _VITALBUM_FS_H_

#include <psp2/io/dirent.h>
#include <psp2/types.h>
#include <string>
#include <vector>

namespace FS {
    bool FileExists(const std::string &path);
    bool DirExists(const std::string &path);
    int GetFileSize(const std::string &path, SceOff &size);
    std::string GetFileExt(const std::string &filename);
    bool IsImageType(const std::string &filename);
    int GetDirList(const std::string &path, std::vector<SceIoDirent> &entries);
    int ChangeDirNext(const std::string &path, std::vector<SceIoDirent> &entries);
    int ChangeDirPrev(std::vector<SceIoDirent> &entries);
    const std::string BuildPath(SceIoDirent &entry);
    int CreateFile(const std::string &path);
    int ReadFile(const std::string &path, unsigned char **buffer, SceOff &size);
    int WriteFile(const std::string &path, const void *data, SceSize size);
}

#endif
