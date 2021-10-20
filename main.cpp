#include <sc2api/sc2_api.h>
#include "Bot.h"

int main(int argc, char *argv[]) {
    Coordinator coordinator;
    coordinator.LoadSettings(argc, argv);

    BotConfig config = BotConfig::from_file("botconfig.yml");

    Bot bot(config);
    coordinator.SetParticipants({
                                        CreateParticipant(Race::Terran, &bot),
                                        CreateComputer(Race::Zerg)
                                });

    coordinator.LaunchStarcraft();
    coordinator.StartGame("BelShirVestigeLE.SC2Map");
    while (coordinator.Update()) {
    }

    return 0;
}
