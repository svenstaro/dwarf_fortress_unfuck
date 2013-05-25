SRC=g_src/basics.cpp g_src/command_line.cpp g_src/enabler.cpp g_src/enabler_input.cpp \
	g_src/files.cpp g_src/find_files_posix.cpp g_src/graphics.cpp g_src/init.cpp \
	g_src/interface.cpp g_src/keybindings.cpp g_src/KeybindingScreen.cpp \
	g_src/random.cpp g_src/renderer_offscreen.cpp g_src/resize++.cpp \
	g_src/textures.cpp g_src/textlines.cpp g_src/ttf_manager.cpp g_src/ViewBase.cpp \
	g_src/win32_compat.cpp g_src/music_and_sound_openal.cpp

default:
	mkdir libs
	g++ -O3 -std=c++11 -m32 -L/usr/lib32 -Dunix -Dlinux -o libs/libgraphics.so -shared $(SRC) -ldl $(shell pkg-config --cflags --libs gtk+-2.0 openal sndfile sdl glu SDL_image SDL_ttf sdl zlib glew)
