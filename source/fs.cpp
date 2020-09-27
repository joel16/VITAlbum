#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib> 
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

    static int Sort(const void *p1, const void *p2) {
        int ret = 0;
        const SceIoDirent *entryA = reinterpret_cast<const SceIoDirent*>(p1);
        const SceIoDirent *entryB = reinterpret_cast<const SceIoDirent*>(p2);

        if ((SCE_S_ISDIR(entryA->d_stat.st_mode)) && !(SCE_S_ISDIR(entryB->d_stat.st_mode)))
            return -1;
        else if (!(SCE_S_ISDIR(entryA->d_stat.st_mode)) && (SCE_S_ISDIR(entryB->d_stat.st_mode)))
            return 1;
        else {
            switch(config.sort) {
                case 0:
                    ret = strcasecmp(entryA->d_name, entryB->d_name);
                    break;

                case 1:
                    ret = strcasecmp(entryB->d_name, entryA->d_name);
                    break;

                case 2:
                    ret = entryA->d_stat.st_size > entryB->d_stat.st_size ? -1 : entryA->d_stat.st_size < entryB->d_stat.st_size ? 1 : 0;
                    break;

                case 3:
                    ret = entryB->d_stat.st_size > entryA->d_stat.st_size ? -1 : entryB->d_stat.st_size < entryA->d_stat.st_size ? 1 : 0;
                    break;
            }
        }

        return ret;
    }

    SceOff GetDirList(const std::string &path, SceIoDirent **entriesp) {
        int ret = 0, i = ((path == "ux0:")? 0 : 1);
        SceOff entry_count = 0;
        SceUID dir = 0;

        entry_count = FS::CountFiles(path) + ((path == "ux0:")? 0 : 1);

        if (R_FAILED(dir = sceIoDopen(path.c_str()))) {
            Log::Error("sceIoDopen(%s) failed: 0x%lx\n", path.c_str(), ret);
            return dir;
        }
        
        SceIoDirent *entries = new SceIoDirent[entry_count * sizeof(entries)];

        // Add parent directory entry if not on root path
        if (path != "ux0:") {
            std::strcpy(entries[0].d_name, "..");
            entries[0].d_stat.st_mode = SCE_S_IFDIR;
        }
        
        do {
            if (R_FAILED(ret = sceIoDread(dir, &entries[i])))
                Log::Error("sceIoDread(%s) failed: 0x%lx\n", path.c_str(), ret);
                
            if ((!FS::IsImageType(entries[i].d_name)) && (!SCE_S_ISDIR(entries[i].d_stat.st_mode)))
                continue;
            
            i++;
        } while (ret > 0);

        std::qsort(entries, entry_count, sizeof(SceIoDirent), FS::Sort);
        
        if (R_FAILED(ret = sceIoDclose(dir))) {
            Log::Error("sceIoDclose(%s) failed: 0x%lx\n", path.c_str(), ret);
            delete[] entries;
            return ret;
        }
        
        *entriesp = entries;
        return entry_count;
    }
    
    void FreeDirEntries(SceIoDirent **entries, SceOff entry_count) {
        if (entry_count > 0)
            delete[] (*entries);
            
        *entries = nullptr;
	}

    //TODO: Clean up change directory impl.
    static SceOff ChangeDir(const std::string &path, SceIoDirent **entries) {
        SceIoDirent *new_entries;
        
        SceOff num_entries = FS::GetDirList(path, &new_entries);
        if (num_entries < 0)
            return -1;
            
        // Free entries and change the current working directory.
        delete[] *entries;
        config.cwd = path;
        Config::Save(config);
        *entries = new_entries;
        return num_entries;
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
    
    SceOff ChangeDirNext(const std::string &path, SceIoDirent **entries) {
        std::string new_path = config.cwd;
        new_path.append("/");
        new_path.append(path);
        return FS::ChangeDir(new_path, entries);
    }
    
    SceOff ChangeDirPrev(SceIoDirent **entries) {
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
