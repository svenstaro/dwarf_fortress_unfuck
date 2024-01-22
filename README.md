Unfucking Dwarf Fortress
========================

**⚠️: As of v50, this repo shouldn't be required anymore.**

As we know, Dwarf Fortress is, most sadly, closed source. However, the multimedia
layer for font rendering, audio output and graphics is open source. The problem
is that the source is somewhat fucked with no clear way to build it and no official
upstream source repo.

This repo exists to track upstream changes, to provide a build system and to unfuck
the code a little bit.

Additionally, GTK has been removed as a hard dependancy and Dwarf Fortress will
run without it. Note that if `[WINDOWED=PROMPT]` in `data/init/init.txt` then
the game must be run from the terminal so that the user may make their choice
with an input prompt.

## Build Dependencies

To build the graphics library you must have the required libraries and
dev packages installed.

You may ignore any GTK dependency if you do not want to build with support.

### Arch Linux

```pacman -S base-devel sdl sdl_image sdl_ttf openal pango atk gtk3 ncurses glew zlib ninja```

### Fedora

```yum install gcc gcc-c++ cmake automake libXext-devel atk-devel cairo-devel gdk-pixbuf2-devel fontconfig-devel openal-soft-devel SDL_image-devel SDL_ttf-devel freetype-devel libX11-devel libICE-devel libSM-devel mesa-libGL mesa-libGL-devel glib2-devel mesa-lib GLU-devel pango-devel ncurses-devel libsndfile-devel gtk3-devel glew-devel SDL-devel glibc-devel zlib```

### Debian/Ubuntu

```apt install -y cmake ninja-build libgl1-mesa-dev libsdl1.2-dev libsdl-image1.2-dev libsdl-ttf2.0-dev libglew-dev libopenal-dev libgtk-3-dev libsndfile1-dev```

## Building Library

To build the library:

```
cmake -Bbuild -GNinja -DCMAKE_BUILD_TYPE=RelWithDebInfo
ninja -Cbuild
```

If the build completed successfully the library file is in `build/libgraphics.so`
