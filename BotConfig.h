#ifndef BASICSC2BOT_BOTCONFIG_H
#define BASICSC2BOT_BOTCONFIG_H

#include <string>
#include <yaml-cpp/yaml.h>

class BotConfig {
public:
    static BotConfig from_file(const std::string &filename);

    const int test_value;

private:
    explicit BotConfig(const YAML::Node&);
};

#endif //BASICSC2BOT_BOTCONFIG_H
