Unfucking Dwarf Fortress
========================

As we know, Dwarf Fortress is, most sadly, closed source. However, the multimedia
layer for font rendering, audio output and graphics is open source. The problem
is that the source is somewhat fucked with no clear way to build it and no official
upstream source repo.

This repo exists to track upstream changes, to provide a build system and to unfuck
the code a little bit.

## Build Dependencies

To build the graphics library you must have the required libraries and
dev packages installed.

### Arch Linux

```pacman -S base-devel {sdl,sdl_image,sdl_ttf,openal,pango,atk,gtk3,ncurses,glew,zlib}```

### Fedora 20

```yum install gcc gcc-c++ cmake automake libXext-devel atk-devel cairo-devel gdk-pixbuf2-devel fontconfig-devel openal-soft-devel SDL_image-devel SDL_ttf-devel freetype-devel libX11-devel libICE-devel libSM-devel mesa-libGL mesa-libGL-devel glib2-devel mesa-lib GLU-devel pango-devel ncurses-devel libsndfile-devel gtk3-devel glew-devel SDL-devel glibc-devel zlib```

## Building Library

To build the library:

```
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
make
```

If the build completed successfully the library file is in `build/libgraphics.so`

