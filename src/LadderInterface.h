#ifndef BASICSC2BOT_LADDERINTERFACE_H
#define BASICSC2BOT_LADDERINTERFACE_H

#include <string>
#include <sc2api/sc2_api.h>

namespace LadderInterface {
    const std::string kDefaultMap = "BelshirVestigeLE.SC2Map";

    sc2::Difficulty GetDifficultyFromString(const std::string &InDifficulty);

    sc2::Race GetRaceFromString(const std::string &RaceIn);

    struct ConnectionOptions {
        int32_t GamePort;
        int32_t StartPort;
        std::string ServerAddress;
        bool ComputerOpponent;
        sc2::Difficulty ComputerDifficulty;
        sc2::Race ComputerRace;
        std::string OpponentId;
        std::string Map;
        bool Realtime;
    };

    void ParseArguments(int argc, char *argv[], ConnectionOptions &connect_options);

    void RunBot(int argc, char *argv[], sc2::Agent *Agent, sc2::Race race);
}

#endif //BASICSC2BOT_LADDERINTERFACE_H