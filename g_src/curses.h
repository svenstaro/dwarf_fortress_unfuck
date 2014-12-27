#ifndef DF_CURSES_H
#define DF_CURSES_H

extern "C" {
#include "GL/glew.h"
#ifndef __APPLE__
#ifdef unix
# include <ncursesw/curses.h>
# undef COLOR_BLUE
# undef COLOR_CYAN
# undef COLOR_RED
# undef COLOR_YELLOW
# include <dlfcn.h>
#endif
#endif
}

#ifndef __APPLE__
#ifdef unix
extern "C" {
  void init_curses();
  extern WINDOW **stdscr_p;
};
#endif
#endif


#endif
