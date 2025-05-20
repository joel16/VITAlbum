#pragma once

#include <string>

typedef struct {
    int sort = 0;
    bool filename = true;
    std::string device;
    std::string cwd;
} config_t;

extern config_t cfg;

namespace Config {
    int Save(config_t &config);
    int Load(void);
}
