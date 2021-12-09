#include <sc2api/sc2_api.h>
#include "Bot.h"
#include "LadderInterface.h"

int main(int argc, char *argv[]) {
    std::string bot_config_file_name = "config/botconfig_rush_1.yml";
    if (argc >= 2 && argv[1][0] != '-')
        bot_config_file_name = argv[1];

    BotConfig config = BotConfig::from_file(bot_config_file_name);
    Bot bot(config);

    LadderInterface::RunBot(argc, argv, &bot, Race::Terran);

    return 0;
}
