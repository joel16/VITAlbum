#pragma once

#include <psp2/io/dirent.h>
#include <psp2/types.h>
#include <string>
#include <vector>

typedef enum FileType {
    FileTypeNone,
    FileTypeImage,
    FileTypeBook
} FileType;

namespace FS {
    bool FileExists(const std::string &path);
    bool DirExists(const std::string &path);
    int MakeDir(const std::string &path);
    int GetFileSize(const std::string &path, SceOff &size);
    const char *GetFileExt(const char *filename);
    bool IsBookType(const char *filename);
    bool IsImageType(const char *filename);
    int GetDirList(const std::string &path, std::vector<SceIoDirent> &entries);
    int ChangeDirNext(const std::string &path, std::vector<SceIoDirent> &entries);
    int ChangeDirPrev(std::vector<SceIoDirent> &entries);
    const std::string BuildPath(SceIoDirent &entry);
    int CreateFile(const std::string &path);
    int ReadFile(const std::string &path, unsigned char **buffer, SceOff &size);
    int WriteFile(const std::string &path, const void *data, SceSize size);
}
