#ifndef DF_CURSES_H
#define DF_CURSES_H

extern "C" {
#include "GL/glew.h"
#if defined(__unix__) || defined(__APPLE__)
#ifdef __APPLE__
# include "ncursesw/curses.h"
#else
# include <curses.h>
#endif
# undef COLOR_BLUE
# undef COLOR_CYAN
# undef COLOR_RED
# undef COLOR_YELLOW
# include <dlfcn.h>
#endif
}

#if defined(__unix__) || defined(__APPLE__)
extern "C" {
  void init_curses();
  extern WINDOW **stdscr_p;
};
#endif


#endif
