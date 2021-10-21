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
$ git clone --recursive https://github.com/zarifmahfuz/SmartBot.git
$ cd SmartBot

:: Create build directory.
$ mkdir build
$ cd build

:: Generate VS solution.
$ cmake ../ -G "Visual Studio 16 2019"

:: Build the project using Visual Studio.
$ start BasicSc2Bot.sln
```

## Mac

Note: Try opening the SC2 game client before installing. If the game crashes before opening, you may need to change your Share name:
* Open `System Preferences`
* Click on `Sharing`
* In the `Computer Name` textfield, change the default 'Macbook Pro' to a single word name (the exact name shouldn't matter, as long as its not the default name)

```bat
:: Clone the project
$ git clone --recursive https://github.com/zarifmahfuz/SmartBot.git
$ cd SmartBot

:: Create build directory.
$ mkdir build
$ cd build

:: Generate a Makefile
:: Use 'cmake -DCMAKE_BUILD_TYPE=Debug ../' if debug info is needed
$ cmake ../

:: Build
$ make
```

# Running
First, configure the BasicSc2Bot project's command-line arguments to debug with:
1. In Visual Studio, right-click on the BasicSc2Bot project under the solution in the Solution Explorer.
2. Click Properties.
3. In the top-left of the window that appears, set the Configuration to "Active(Debug)".
4. Select the Debugging page under Configuration Properties on the left.
5. Set the value of Command Arguments to `-c -a zerg -d Hard -m CactusValleyLE.SC2Map`. This will result in the bot playing against the built-in Zerg AI on hard difficulty on the map CactusValleyLE.
6. Click OK.


You can press F5 or the ▶ button at the top of Visual Studio to run the bot with the command-line arguments you configured.

