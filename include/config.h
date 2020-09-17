#ifndef _VITALBUM_CONFIG_H_
#define _VITALBUM_CONFIG_H_

#include <string>

typedef struct {
    int sort = 0;
    bool dev_options = false;
    bool image_filename = false;
    std::string cwd;
} config_t;

extern config_t config;

namespace Config {
    int Save(config_t config);
    int Load(void);
}

#endif
