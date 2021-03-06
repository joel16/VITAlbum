#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>

#include "config.h"
#include "fs.h"
#include "log.h"
#include "utils.h"

namespace FS {
    bool FileExists(const std::string &path) {
        SceUID file = 0;
        
        if (R_SUCCEEDED(file = sceIoOpen(path.c_str(), SCE_O_RDONLY, 0777))) {
            sceIoClose(file);
            return true;
        }
        
        return false;
    }

    bool DirExists(const std::string &path) {
        SceUID dir = 0;
        
        if (R_SUCCEEDED(dir = sceIoDopen(path.c_str()))) {
            sceIoDclose(dir);
            return true;
        }
        
        return false;
    }

    int GetFileSize(const std::string &path, SceOff *size) {
        int ret = 0;
        SceIoStat stat;

        if (R_FAILED(ret = sceIoGetstat(path.c_str(), &stat))) {
            Log::Error("sceIoGetstat(%s) failed: 0x%lx\n", path.c_str(), ret);
            return ret;
        }
        
        *size = stat.st_size;
        return 0;
    }

    std::string GetFileExt(const std::string &filename) {
        std::string ext = std::filesystem::path(filename).extension();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::toupper);
        return ext;
    }

    bool IsImageType(const std::string &filename) {
        std::string ext = FS::GetFileExt(filename);
        
        if ((!ext.compare(".BMP")) || (!ext.compare(".GIF")) || (!ext.compare(".ICO")) || (!ext.compare(".JPG")) || (!ext.compare(".JPEG"))
            || (!ext.compare(".PGM")) || (!ext.compare(".PPM")) || (!ext.compare(".PNG")) || (!ext.compare(".PSD")) || (!ext.compare(".SVG"))
            || (!ext.compare(".TGA")) || (!ext.compare(".TIFF")) || (!ext.compare(".WEBP")))
            return true;

        return false;

    }

    static SceOff CountFiles(const std::string &path) {
        int ret = 0;
        SceOff entry_count = 0;
        SceUID dir = 0;
        
        if (R_FAILED(dir = sceIoDopen(path.c_str()))) {
            Log::Error("sceIoDopen(%s) failed: 0x%lx\n", path.c_str(), ret);
            return dir;
        }
        
        do {
            SceIoDirent entries;
            std::memset(&entries, 0, sizeof(SceIoDirent));
            
            if (R_FAILED(ret = sceIoDread(dir, &entries)))
                Log::Error("sceIoDread(%s) failed: 0x%lx\n", path.c_str(), ret);
            
            if ((!FS::IsImageType(entries.d_name)) && (!SCE_S_ISDIR(entries.d_stat.st_mode)))
                continue;
            
            if (ret > 0)
                entry_count++;
        } while (ret > 0);
        
        if (R_FAILED(ret = sceIoDclose(dir))) {
            Log::Error("sceIoDclose(%s) failed: 0x%lx\n", path.c_str(), ret);
            return ret;
        }
        
        return entry_count;
    }

    static bool Sort(const SceIoDirent &entryA, const SceIoDirent &entryB) {
        if ((SCE_S_ISDIR(entryA.d_stat.st_mode)) && !(SCE_S_ISDIR(entryB.d_stat.st_mode)))
            return true;
        else if (!(SCE_S_ISDIR(entryA.d_stat.st_mode)) && (SCE_S_ISDIR(entryB.d_stat.st_mode)))
            return false;
        else {
            switch(config.sort) {
                case 0:
                    if (strcasecmp(entryA.d_name, entryB.d_name) < 0)
                        return true;
                    
                    break;

                case 1:
                    if (strcasecmp(entryB.d_name, entryA.d_name) < 0)
                        return true;
                    
                    break;

                case 2:
                    if (entryB.d_stat.st_size < entryA.d_stat.st_size)
                        return true;
                    
                    break;

                case 3:
                    if (entryA.d_stat.st_size < entryB.d_stat.st_size)
                        return true;
                    
                    break;
            }
        }
        
        return false;
    }

    int GetDirList(const std::string &path, std::vector<SceIoDirent> &entries) {
        int ret = 0, i = ((path == "ux0:")? 0 : 1);
        SceUID dir = 0;

        SceOff entry_count = FS::CountFiles(path) + ((path == "ux0:")? 0 : 1);

        if (R_FAILED(dir = sceIoDopen(path.c_str()))) {
            Log::Error("sceIoDopen(%s) failed: 0x%lx\n", path.c_str(), ret);
            return dir;
        }
        
        entries.resize(entry_count);

        // Add parent directory entry if not on root path
        if (path != "ux0:") {
            std::strcpy(entries[0].d_name, "..");
            entries[0].d_stat.st_mode = SCE_S_IFDIR;
        }
        
        do {
            SceIoDirent dirent;
            if (R_FAILED(ret = sceIoDread(dir, &dirent)))
                Log::Error("sceIoDread(%s) failed: 0x%lx\n", path.c_str(), ret);
            
            if ((!FS::IsImageType(dirent.d_name)) && (!SCE_S_ISDIR(dirent.d_stat.st_mode)))
                continue;
            
            entries.push_back(dirent);
            i++;
        } while (ret > 0);

        std::sort(entries.begin(), entries.end(), FS::Sort);
        
        if (R_FAILED(ret = sceIoDclose(dir))) {
            Log::Error("sceIoDclose(%s) failed: 0x%lx\n", path.c_str(), ret);
            return ret;
        }

        return 0;
    }

    //TODO: Clean up change directory impl.
    static int ChangeDir(const std::string &path, std::vector<SceIoDirent> &entries) {
        int ret = 0;
        std::vector<SceIoDirent> new_entries;
        
        if (R_FAILED(ret = FS::GetDirList(path, new_entries)))
            return ret;
            
        // Free entries and change the current working directory.
        entries.clear();
        config.cwd = path;
        Config::Save(config);
        entries = new_entries;
        return 0;;
    }

    static int ChangeDirUp(char path[256]) {
        if (config.cwd.length() <= 1 && config.cwd.c_str()[0] == '/')
            return -1;
            
        // Remove upmost directory
        bool copy = false;
        int len = 0;
        for (ssize_t i = config.cwd.length(); i >= 0; i--) {
            if (config.cwd.c_str()[i] == '/')
                copy = true;
            if (copy) {
                path[i] = config.cwd.c_str()[i];
                len++;
            }
        }
        
        // remove trailing slash
        if (len > 1 && path[len - 1] == '/')
            len--;
        
        path[len] = '\0';
        return 0;
    }
    
    int ChangeDirNext(const std::string &path, std::vector<SceIoDirent> &entries) {
        std::string new_path = config.cwd;
        new_path.append("/");
        new_path.append(path);
        return FS::ChangeDir(new_path, entries);
    }
    
    int ChangeDirPrev(std::vector<SceIoDirent> &entries) {
        char new_path[256];
        if (FS::ChangeDirUp(new_path) < 0)
            return -1;
        
        return FS::ChangeDir(std::string(new_path), entries);
    }
    
    const std::string BuildPath(SceIoDirent *entry) {
        std::string path = config.cwd;
        path.append("/");
        path.append(entry->d_name);
        return path;
    }

    int CreateFile(const std::string &path) {
        int ret = 0;
        SceUID file = 0;
        
        if (R_FAILED(ret = file = sceIoOpen(path.c_str(), SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777))) {
            Log::Error("sceIoOpen(%s) failed: 0x%lx\n", path.c_str(), ret);
            return ret;
        }
            
        if (R_FAILED(ret = sceIoClose(file))) {
            Log::Error("sceIoClose(%s) failed: 0x%lx\n", path.c_str(), ret);
            return ret;
        }
            
        return 0;
    }

    int ReadFile(const std::string &path, unsigned char **buffer, SceOff *size) {
        int ret = 0;
        SceUID file = 0;

        if (R_FAILED(ret = file = sceIoOpen(path.c_str(), SCE_O_RDONLY, 0))) {
            Log::Error("sceIoOpen(%s) failed: 0x%lx\n", path.c_str(), ret);
            return ret;
        }
        
        *size = sceIoLseek(file, 0, SEEK_END);
        *buffer = new unsigned char[*size];

        if (R_FAILED(ret = sceIoPread(file, *buffer, *size, SCE_SEEK_SET))) {
            Log::Error("sceIoPread(%s) failed: 0x%lx\n", path.c_str(), ret);
            return ret;
        }

        if (R_FAILED(ret = sceIoClose(file))) {
            Log::Error("sceIoClose(%s) failed: 0x%lx\n", path.c_str(), ret);
            return ret;
        }

        return 0;
    }

    int WriteFile(const std::string &path, const void *data, SceSize size) {
        int ret = 0, bytes_written = 0;
        SceUID file = 0;

        if (R_FAILED(ret = file = sceIoOpen(path.c_str(), SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777))) {
            Log::Error("sceIoOpen(%s) failed: 0x%lx\n", path.c_str(), ret);
            return ret;
        }

        if (R_FAILED(ret = bytes_written = sceIoWrite(file, data, size))) {
            Log::Error("sceIoWrite(%s) failed: 0x%lx\n", path.c_str(), ret);
            sceIoClose(file);
            return ret;
        }
        
        if (R_FAILED(ret = sceIoClose(file))) {
            Log::Error("sceIoClose(%s) failed: 0x%lx\n", path.c_str(), ret);
            return ret;
        }

        return bytes_written;
    }
}
