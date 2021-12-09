# Create the executable.
add_executable(Bot main.cpp Bot.cpp BotConfig.cpp BotBarracks.cpp BotSupplyDepot.cpp BotCommandCenter.cpp
        BotRefinery.cpp LadderInterface.cpp BotEngineeringBay.cpp)
target_link_libraries(Bot
        sc2api sc2lib sc2utils yaml-cpp
        )
