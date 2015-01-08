static bool curses_initialized = false;

static void endwin_void() {
  if (curses_initialized) {
    endwin();
    curses_initialized = false;
  }
}

class renderer_curses : public renderer {
  std::map<std::pair<int,int>,int> color_pairs;

  // Map from DF color to ncurses color
  static int ncurses_map_color(int color) {
    if (color < 0) abort();
    switch (color) {
    case 0: return 0;
    case 1: return 4;
    case 2: return 2;
    case 3: return 6;
    case 4: return 1;
    case 5: return 5;
    case 6: return 3;
    case 7: return 7;
    default: return ncurses_map_color(color - 7);
    }
  }

  // Look up, or create, a curses color pair
  int lookup_pair(pair<int,int> color) {
    map<pair<int,int>,int>::iterator it = color_pairs.find(color);
    if (it != color_pairs.end()) return it->second;
    // We don't already have it. Make sure it's in range.
    if (color.first < 0 || color.first > 7 || color.second < 0 || color.second > 7) return 0;
    // We don't already have it. Generate a new pair if possible.
    if (color_pairs.size() < COLOR_PAIRS - 1) {
      const short pair = color_pairs.size() + 1;
      init_pair(pair, ncurses_map_color(color.first), ncurses_map_color(color.second));
      color_pairs[color] = pair;
      return pair;
    }
    // We don't have it, and there's no space for more. Find the closest equivalent.
    int score = 999, pair = 0;
    int rfg = color.first % 16, rbg = color.second % 16;
    for (auto it = color_pairs.cbegin(); it != color_pairs.cend(); ++it) {
      int fg = it->first.first;
      int bg = it->first.second;
      int candidate = it->second;
      int candidate_score = 0;  // Lower is better.
      if (rbg != bg) {
        if (rbg == 0 || rbg == 15)
          candidate_score += 3;  // We would like to keep the background black/white.
        if ((rbg == 7 || rbg == 8)) {
          if (bg == 7 || bg == 8)
            candidate_score += 1; // Well, it's still grey.
          else
            candidate_score += 2;
        }
      }
      if (rfg != fg) {
        if (rfg == 0 || rfg == 15)
          candidate_score += 5; // Keep the foreground black/white if at all possible.
        if (rfg == 7 || rfg == 8) {
          if (fg == 7 || fg == 8)
            candidate_score += 1; // Still grey. Meh.
          else
            candidate_score += 3;
        }
      }
      if (candidate_score < score) {
        score = candidate_score;
        pair = candidate;
      }
    }
    color_pairs[color] = pair;
    return pair;
  }

public:

  void update_tile(int x, int y) {
    const int ch   = gps.screen[x*gps.dimy*4 + y*4 + 0];
    const int fg   = gps.screen[x*gps.dimy*4 + y*4 + 1];
    const int bg   = gps.screen[x*gps.dimy*4 + y*4 + 2];
    const int bold = gps.screen[x*gps.dimy*4 + y*4 + 3];

    const int pair = lookup_pair(make_pair(fg,bg));

    if (ch == 219 && !bold) {
      // It's █, which is used for borders and digging designations.
      // A_REVERSE space looks better if it isn't completely tall.
      // Which is most of the time, for me at least.
      // █ <-- Do you see gaps?
      // █
      // The color can't be bold.
      wattrset(*stdscr_p, COLOR_PAIR(pair) | A_REVERSE);
      mvwaddstr(*stdscr_p, y, x, " ");
    } else {
      wattrset(*stdscr_p, COLOR_PAIR(pair) | (bold ? A_BOLD : 0));
      wchar_t chs[2] = {charmap[ch],0};
      mvwaddwstr(*stdscr_p, y, x, chs);
    }
  }

  void update_all() {
    for (int x = 0; x < init.display.grid_x; x++)
      for (int y = 0; y < init.display.grid_y; y++)
        update_tile(x, y);
  }

  void render() {
    refresh();
  }

  void resize(int w, int h) {
    if (enabler.overridden_grid_sizes.size() == 0)
      gps_allocate(w, h);
    erase();
    // Force a full display cycle
    gps.force_full_display_count = 1;
    enabler.flag |= ENABLERFLAG_RENDER;
  }

  void grid_resize(int w, int h) {
    gps_allocate(w, h);
  }

  renderer_curses() {
    init_curses();
  }

  bool get_mouse_coords(int &x, int &y) {
    return false;
  }
};

// Reads from getch, collapsing utf-8 encoding to the actual unicode
// character.  Ncurses symbols (left arrow, etc.) are returned as
// positive values, unicode as negative. Error returns 0.
static int getch_utf8() {
  int byte = wgetch(*stdscr_p);
  if (byte == ERR) return 0;
  if (byte > 0xff) return byte;
  int len = decode_utf8_predict_length(byte);
  if (!len) return 0;
  string input(len,0); input[0] = byte;
  for (int i = 1; i < len; i++) input[i] = wgetch(*stdscr_p);
  return -decode_utf8(input);
}

void enablerst::eventLoop_ncurses() {
  int x, y, oldx = 0, oldy = 0;
  renderer_curses *renderer = static_cast<renderer_curses*>(this->renderer);
  
  while (loopvar) {
    // Check for terminal resize
    getmaxyx(*stdscr_p, y, x);
    if (y != oldy || x != oldx) {
      pause_async_loop();
      renderer->resize(x, y);
      unpause_async_loop();
      oldx = x; oldy = y;
    }
    
    // Deal with input
    Uint32 now = SDL_GetTicks();
    // Read keyboard input, if any, and transform to artificial SDL
    // events for enabler_input.
    int key;
    bool paused_loop = false;
    while ((key = getch_utf8())) {
      if (!paused_loop) {
        pause_async_loop();
        paused_loop = true;
      }
      bool esc = false;
      if (key == KEY_MOUSE) {
        MEVENT ev;
        if (getmouse(&ev) == OK) {
          // TODO: Deal with curses mouse input. And turn it on above.
        }
      } else if (key == -27) { // esc
        int second = getch_utf8();
        if (second) { // That was an escape sequence
          esc = true;
          key = second;
        }
      }
      add_input_ncurses(key, now, esc);
    }

    if (paused_loop)
      unpause_async_loop();

    // Run the common logic
    do_frame();
  }
}


//// libncursesw stub ////

extern "C" {
  static void *handle;
  WINDOW **stdscr_p;

  int COLOR_PAIRS;
  static int (*_erase)(void);
  static int (*_wmove)(WINDOW *w, int y, int x);
  static int (*_waddnstr)(WINDOW *w, const char *s, int n);
  static int (*_nodelay)(WINDOW *w, bool b);
  static int (*_refresh)(void);
  static int (*_wgetch)(WINDOW *w);
  static int (*_endwin)(void);
  static WINDOW *(*_initscr)(void);
  static int (*_raw)(void);
  static int (*_keypad)(WINDOW *w, bool b);
  static int (*_noecho)(void);
  static int (*_set_escdelay)(int delay);
  static int (*_curs_set)(int s);
  static int (*_start_color)(void);
  static int (*_init_pair)(short p, short fg, short bg);
  static int (*_getmouse)(MEVENT *m);
  static int (*_waddnwstr)(WINDOW *w, const wchar_t *s, int i);

  static void *dlsym_orexit(const char *symbol, bool actually_exit = true) {
    void *sym = dlsym(handle, symbol);
    if (!sym) {
      printf("Symbol not found: %s\n", symbol);
      if (actually_exit)
        exit(EXIT_FAILURE);
    }
    return sym;
  }

  int erase(void) {
    return _erase();
  }
  int wmove(WINDOW *w, int y, int x) {
    return _wmove(w, y, x);
  }
  int waddnstr(WINDOW *w, const char *s, int n) {
    return _waddnstr(w, s, n);
  }
  int nodelay(WINDOW *w, bool b) {
    return _nodelay(w, b);
  }
  int refresh(void) {
    return _refresh();
  }
  int wgetch(WINDOW *w) {
    return _wgetch(w);
  }
  int endwin(void) {
    return _endwin();
  }
  WINDOW *initscr(void) {
    return _initscr();
  }
  int raw(void) {
    return _raw();
  }
  int keypad(WINDOW *w, bool b) {
    return _keypad(w, b);
  }
  int noecho(void) {
    return _noecho();
  }
  int set_escdelay(int delay) {
    if (_set_escdelay)
      return _set_escdelay(delay);
    else
      return 0;
  }
  int curs_set(int s) {
    return _curs_set(s);
  }
  int start_color(void) {
    return _start_color();
  }
  int init_pair(short p, short fg, short bg) {
    return _init_pair(p, fg, bg);
  }
  int getmouse(MEVENT *m) {
    return _getmouse(m);
  }
  int waddnwstr(WINDOW *w, const wchar_t *s, int n) {
    return _waddnwstr(w, s, n);
  }

  void init_curses() {
    static bool stub_initialized = false;
    // Initialize the stub
    if (!stub_initialized) {
      stub_initialized = true;
      // We prefer libncursesw, but we'll accept libncurses if we have to
      handle = dlopen("libncursesw.so.5", RTLD_LAZY);
      if (handle) goto opened;
      handle = dlopen("libncursesw.so", RTLD_LAZY);
      if (handle) goto opened;
      puts("Didn't find any flavor of libncursesw, attempting libncurses");
      sleep(5);
      handle = dlopen("libncurses.dylib", RTLD_LAZY);
      if (handle) goto opened;
      handle = dlopen("libncurses.so.5", RTLD_LAZY);
      if (handle) goto opened;
      handle = dlopen("libncurses.so", RTLD_LAZY);
      if (handle) goto opened;
      handle = dlopen("libncurses.5.4.dylib", RTLD_LAZY);
      if (handle) goto opened;
      handle = dlopen("/usr/lib/libncurses.dylib", RTLD_LAZY);
      if (handle) goto opened;
      handle = dlopen("/usr/lib/libncurses.5.4.dylib", RTLD_LAZY);
      if (handle) goto opened;

    opened:
      if (!handle) {
        puts("Unable to open any flavor of libncurses!");
        exit(EXIT_FAILURE);
      }
      // Okay, look up our symbols
      int *pairs = (int*)dlsym_orexit("COLOR_PAIRS");
      COLOR_PAIRS = *pairs;
      stdscr_p = (WINDOW**)dlsym_orexit("stdscr");
      _erase = (int (*)(void))dlsym_orexit("erase");
      _wmove = (int (*)(WINDOW *w, int y, int x))dlsym_orexit("wmove");
      _waddnstr = (int (*)(WINDOW *w, const char *s, int n))dlsym_orexit("waddnstr");
      _nodelay = (int (*)(WINDOW *w, bool b))dlsym_orexit("nodelay");
      _refresh = (int (*)(void))dlsym_orexit("refresh");
      _wgetch = (int (*)(WINDOW *w))dlsym_orexit("wgetch");
      _endwin = (int (*)(void))dlsym_orexit("endwin");
      _initscr = (WINDOW *(*)(void))dlsym_orexit("initscr");
      _raw = (int (*)(void))dlsym_orexit("raw");
      _keypad = (int (*)(WINDOW *w, bool b))dlsym_orexit("keypad");
      _noecho = (int (*)(void))dlsym_orexit("noecho");
      _set_escdelay = (int (*)(int delay))dlsym_orexit("set_escdelay", false);
      _curs_set = (int (*)(int s))dlsym_orexit("curs_set");
      _start_color = (int (*)(void))dlsym_orexit("start_color");
      _init_pair = (int (*)(short p, short fg, short bg))dlsym_orexit("init_pair");
      _getmouse = (int (*)(MEVENT *m))dlsym_orexit("getmouse");
      _waddnwstr = (int (*)(WINDOW *w, const wchar_t *s, int i))dlsym_orexit("waddnwstr");
    }
    
    // Initialize curses
    if (!curses_initialized) {
      curses_initialized = true;
      WINDOW *new_window = initscr();
      if (!new_window) {
        puts("unable to create ncurses window - initscr failed!");
        exit(EXIT_FAILURE);
      }
      // in some versions of curses, initscr does not update stdscr!
      if (!*stdscr_p) *stdscr_p = new_window;
      raw();
      noecho();
      keypad(*stdscr_p, true);
      nodelay(*stdscr_p, true);
      set_escdelay(25); // Possible bug
      curs_set(0);
      mmask_t dummy;
      // mousemask(ALL_MOUSE_EVENTS, &dummy);
      start_color();
      init_pair(1, COLOR_WHITE, COLOR_BLACK);
      
      atexit(endwin_void);
    }
  }
};

