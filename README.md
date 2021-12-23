# SmartSC2Bot

# Building
## Requirements
* [Visual Studio 2019](https://www.visualstudio.com/downloads/)
* [CMake](https://cmake.org/download/)
* StarCraft II ([Windows](https://starcraft2.com/en-us/)) ([Linux](https://github.com/Blizzard/s2client-proto#linux-packages)) 
* [StarCraft II Map Packs](https://github.com/Blizzard/s2client-proto#map-packs)

Install StarCraft II and extract the contents of the Ladder 2017 Season 1 map pack into the `Maps` directory in the install location of StarCraft II. If it doesn't exist, create it. On Windows, by default it is `C:\Program Files (x86)\StarCraft II\Maps`.

## Windows

Download and install [Visual Studio 2019](https://www.visualstudio.com/downloads/) if you need it. Building with Visual Studio 2019 not yet supported.

```bat
:: Clone the project
$ git clone --recursive https://github.com/zarifmahfuz/SmartSC2Bot.git
$ cd SmartSC2Bot

:: Create build directory.
$ mkdir build
$ cd build

:: Generate VS solution.
$ cmake ../ -G "Visual Studio 16 2019"
```
The solution file that is generated in the build directory can be opened with Visual Studio.
From there, the project can be compiled.

## Mac

Note: Try opening the SC2 game client before installing. If the game crashes before opening, you may need to change your Share name:
* Open `System Preferences`
* Click on `Sharing`
* In the `Computer Name` textfield, change the default 'Macbook Pro' to a single word name (the exact name shouldn't matter, as long as its not the default name)

```bat
:: Clone the project
$ git clone --recursive https://github.com/zarifmahfuz/SmartSC2Bot.git
$ cd SmartSC2Bot

:: Create build directory.
$ mkdir build
$ cd build

:: Generate a Makefile
:: Use 'cmake -DCMAKE_BUILD_TYPE=Debug ../' if debug info is needed
$ cmake ../

:: Build
$ make
```

For both Windows and Mac, an executable `Bot` should be generated in the `build/bin` directory. 

# Running
To run the bot, run `build/bin/Bot` and pass the command-line arguments that the LadderInterface expects.
For example, the following command runs the bot against a Hard-difficulty Zerg computer opponent on Cactus Valley.

```
$ build/bin/Bot -c -a zerg -d Hard -m CactusValleyLE.SC2Map
```

This will run StarCraft II from its default installation directory, and look for CactusValleyLE.SC2Map in a subdirectory of it called "Maps".

To see a full list of command-line arguments run the following command.

```
$ build/bin/Bot --help
```

