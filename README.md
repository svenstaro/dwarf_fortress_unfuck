Unfucking Dwarf Fortress
========================

As we know, Dwarf Fortress is, most sadly, closed source. However, the multimedia
layer for font rendering, audio output and graphics is open source. The problem
is that the source is somewhat fucked with no clear way to build it and no official
upstream source repo.

This repo exists to track upstream changes, to provide a build system and to unfuck
the code a little bit.

## Build Dependencies

To build the graphics library you must have the required 32 bit libraries and
dev packages installed.

### Arch Linux

Have [multilib] enabled in pacman.conf. And then run:  

```pacman -S gcc-multilib lib32-{sdl,sdl_image,sdl_ttf,openal,pango,atk,gdk-pixbuf2,gtk2,ncurses}```

### Fedora 20

```yum install gcc gcc-c++ cmake automake libXext-devel.i686 atk-devel.i686 cairo-devel.i686 gdk-pixbuf2-devel.i686 fontconfig-devel.i686 openal-soft-devel.i686 SDL_image-devel.i686 SDL_ttf-devel.i686 freetype-devel.i686 libX11-devel.i686 libICE-devel.i686 libSM-devel.i686 mesa-libGL.i686 mesa-libGL-devel.i686 glib2-devel.i686 mesa-libGLU-devel.i686 pango-devel.i686 ncurses-devel.i686 libsndfile-devel.i686 gtk2-devel.i686 glew-devel.i686 SDL-devel.i686 glibc-devel.i686```

## Building Library

To build the library:

```
mkdir build && cd build
cmake ..
make -j4
```

If the build completed successfully the library file is in `build/libgraphics.so`

