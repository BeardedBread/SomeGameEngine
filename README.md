# Some Game Engine

**Sandbox demo here**: [link](https://sadpumpkin.itch.io/somegameengine)

An attempt to create a game with the ECS architecture in C. This is mostly a learning experience about what goes into a game engine.

The goal of the project is to make a game similar to Hannah and The Pirate Caves from Neopets. I thought that's a nice game to aim for, plus I have fond memories of it. It is not a full-on remake though!

**DISCLAIMER**: There is no obligations tied to this project. For all I know, the project can just get abandoned at anytime. I **WILL** take my time with this project.

## Implementation Notes
As mentioned, this is mostly a learning experience, so certain things are implemented for the sake of learning. However, there are certain things that I didn't bother with because dealing with them will open up another can of worms. Either that or I find it too large of a feature to implement on my own without hating my existance. For these, I will use libraries.

As this point of time (24/08/2024), these are the things I won't bother for now:
- Game rendering: I remember dealing with OpenGL for a little bit and it being a not-so-pleasant experience. This is also a large topic on its own too.
- Camera System: This is an interesting one. I would really like to implement this. However, this is sort of tied to game rendering, so no.
- Windows and raw input handling: Not keen on this.
- GUI: I'm not about to roll my own GUI library for this. I'll do a lite version, but that's about it.
- Data structures: Will only do it for specific reasons. Otherwise, use a library.
- Level editor: ... maybe. I already have a sandbox, so maybe I can turn that into an editor lite??? Won't think too much about it for now.

Libraries/Tools used:
- _raylib_ \[[link](https://github.com/raysan5/raylib)\] + _raygui_ \[[link](https://github.com/raysan5/raygui)\]: MVP of this project. Basically the backbone of the game. I've use it for some past projects and it's always a pleasant experience. Can recommend!
- _rres_ \[[link](https://github.com/raysan5/rres)\] : For resource packing. Thank you Ray!
- _rfxgen_ \[[link](https://github.com/raysan5/rfxgen)\] : For generate SFX. Ray carrying the project...
- _zstd_ \[[link](https://github.com/facebook/zstd)\] : For compression. I only used this because it looks cool.
- _cmocka_ \[[link](https://cmocka.org/)\] : For unit testing. Probably should write more unit tests...
- _sc_ \[[link](https://github.com/tezc/sc)\]: For data structures. The library targets server backend usage, so it might not be the most suitable usage, and is likely to be replaced. However, it has been a good experience using this library, so no complaints from me.
- _LDtk_ \[[link](https://ldtk.io/)\]: A nice level editor. I haven't use it to its fullest extent, but definitely having a good experience so far.
- _Aseprite_ \[[link](https://www.aseprite.org/)\]: Used it to create sprites. Good tool!
- _Emscripten_ \[[link](https://emscripten.org/)\]: For web build. It's really a marvel of technology!
- _heaptrack_ \[[link](https://github.com/KDE/heaptrack)\]: For heap profiling. Simple and straightforward to use.

## Progress
The engine features:
- An Entity-Component framework with an Entity Manager + memory pool that is specific for this project
- AABB collision system + Grid-based Broad phase collision detection
- Scene tree management and transition
- Assets management and sprite transition
- Simple level loading
- Simple Particle effects management

Current progress:
- Simple main menu + sandbox scene
- Player movement
- Tiles: Solid tiles, water, One-way platforms and ladders
- Crates: wooden and metal
- Boulder
- Chest and Level Ending
- Arrows and dynamites
- Water filling
- Sound Effects
- Simple Camera Update System
- Demo level pack loading

## Build Instruction
The build instructions are not great, so you might need to troubleshoot a little on your own.

1. Build and install raylib.
2. Clone this repository.
3. `mkdir build && cd build && cmake ..`
4. Run: `./HATPC_remake`

Depending on you raylib installation, you may need to specify its directory. Use the `RAYLIB_DIR` variable to indicate raylib's installation path when running the CMake. It is the same thing with `LIBZSTD_DIR` for zstd library.
```
cmake -DCMAKE_BUILD_TYPE=Release -DRAYLIB_DIR=/usr/local/lib -DLIBZSTD_DIR=/usr/local/lib ..
```
You may also turn off `BUILD_TESTING` to avoid building the tests.

There are also other binaries generated for testing purposes. Feel free to try them out if you manage to build them.

## Debugging && Profiling
All binaries except the _main_ one are built with ASAN, which helps to detect memory leakage.

_Heaptrack_ is used to do so for the main program to keep for memory leakage + heap usage. From my experience, it doesn't work with ASAN.

I'm looking for runtime profiler. Current candidates are: _Orbit_ and _tracey_. For small-ish gameplay, the program still runs fine.

## Note on assets
This repository will not contain the assets used in the game, such as fonts, art, and sfx. However, the program should still run without those. Assets are placed in the _res/_ directory.
