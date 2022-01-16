#ifndef _VITALBUM_CONFIG_H_
#define _VITALBUM_CONFIG_H_

#include <string>

typedef struct {
    int sort = 0;
    bool image_filename = false;
    std::string device;
    std::string cwd;
} config_t;

extern config_t cfg;

namespace Config {
    int Save(config_t &config);
    int Load(void);
}

#endif
