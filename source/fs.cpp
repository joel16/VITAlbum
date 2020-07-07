#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib> 
#include <cstring>
#include <filesystem>

#include "fs.h"
#include "utils.h"

namespace FS {
    std::string CWD;

    int GetFileSize(const std::string &path, SceOff *size) {
        int ret = 0;
        SceIoStat stat;

        if (R_FAILED(ret = sceIoGetstat(path.c_str(), &stat)))
            return ret;
        
        *size = stat.st_size;
        return 0;
    }

    std::string GetFileExt(const std::string &filename) {
        std::string ext = std::filesystem::path(filename).extension();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::toupper);
        return ext;
    }

    static SceOff CountFiles(const std::string &path) {
        int ret = 0;
        SceOff entry_count = 0;
        SceUID dir = sceIoDopen(path.c_str());
        
        do {
            SceIoDirent entries;
            std::memset(&entries, 0, sizeof(SceIoDirent));
            
            ret = sceIoDread(dir, &entries);
            if (ret > 0)
                entry_count++;
        } while (ret > 0);
        
        sceIoDclose(dir);
        return entry_count;
    }

    static int sort(const void *p1, const void *p2) {
        SceIoDirent *entryA = (SceIoDirent *)p1;
        SceIoDirent *entryB = (SceIoDirent *)p2;
        
        if ((SCE_S_ISDIR(entryA->d_stat.st_mode)) && !(SCE_S_ISDIR(entryB->d_stat.st_mode)))
            return -1;
        else if (!(SCE_S_ISDIR(entryA->d_stat.st_mode)) && (SCE_S_ISDIR(entryB->d_stat.st_mode)))
            return 1;
        
        return strcasecmp(entryA->d_name, entryB->d_name);
    }

    SceOff GetDirList(const std::string &path, SceIoDirent **entriesp) {
        int ret = 0, i = ((path == "ux0:")? 0 : 1);
        SceOff entry_count = 0;
        SceUID dir = 0;

        entry_count = CountFiles(path) + ((path == "ux0:")? 0 : 1);

        if (R_FAILED(dir = sceIoDopen(path.c_str())))
            return dir;
        
        SceIoDirent *entries = new SceIoDirent[entry_count * sizeof(*entries)];

        // Add parent directory entry if not on root path
        if (path != "ux0:") {
            std::strcpy(entries[0].d_name, "..");
            entries[0].d_stat.st_mode = SCE_S_IFDIR;
        }
        
        do {
            ret = sceIoDread(dir, &entries[i]);
            i++;
        } while (ret > 0);

        std::qsort(entries, entry_count, sizeof(SceIoDirent), sort);
        
        if (R_FAILED(ret = sceIoDclose(dir))) {	
            delete entries;	
            return ret;	
        }
        
        *entriesp = entries;
        return entry_count;
    }

    //TODO: Clean up change directory impl.
    static SceOff ChangeDir(const std::string &path, SceIoDirent **entries) {
        SceIoDirent *new_entries;
        
        SceOff num_entries = GetDirList(path, &new_entries);
        if (num_entries < 0)
            return -1;
            
        // Free entries and change the current working directory.
        delete *entries;
        CWD = path;
        *entries = new_entries;
        return num_entries;
    }

    static int ChangeDirUp(char path[256]) {
        if (CWD.length() <= 1 && CWD.c_str()[0] == '/')
            return -1;
            
        // Remove upmost directory
        bool copy = false;
        int len = 0;
        for (ssize_t i = CWD.length(); i >= 0; i--) {
            if (CWD.c_str()[i] == '/')
                copy = true;
            if (copy) {
                path[i] = CWD.c_str()[i];
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
        std::string new_path = CWD;
        new_path.append("/");
        new_path.append(path);
        return ChangeDir(new_path, entries);
    }
    
    SceOff ChangeDirPrev(SceIoDirent **entries) {
        char new_path[256];
        if (ChangeDirUp(new_path) < 0)
            return -1;
        
        return ChangeDir(std::string(new_path), entries);
    }
    
    const std::string BuildPath(SceIoDirent *entry) {
        std::string path = CWD;
        path.append("/");
        path.append(entry->d_name);
        return path;
    }

    int ReadFile(const std::string &path, unsigned char **buffer, SceOff *size) {
        SceUID file = 0;
        int ret = 0;

        if (R_FAILED(ret = file = sceIoOpen(path.c_str(), SCE_O_RDONLY, 0)))
            return ret;
        
        *size = sceIoLseek(file, 0, SEEK_END);
        *buffer = (unsigned char *)std::malloc(*size);

        if (R_FAILED(ret = sceIoPread(file, *buffer, *size, SCE_SEEK_SET)))
            return ret;

        if (R_FAILED(ret = sceIoClose(file)))
            return ret;

        return 0;
    }
}
