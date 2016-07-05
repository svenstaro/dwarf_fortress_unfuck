#ifdef __APPLE__
# include "osx_messagebox.h"
#elif defined(unix)
# include <gtk/gtk.h>
#endif

#include <cassert>

#include "platform.h"
#include "enabler.h"
#include "random.h"
#include "init.h"
#include "music_and_sound_g.h"

#ifdef unix
# include <locale.h>
#endif

using namespace std;

enablerst enabler;

// For the printGLError macro
int glerrorcount = 0;

// Set to 0 when the game wants to quit
static int loopvar = 1;

// Reports an error to the user, using a MessageBox and stderr.
void report_error(const char *error_preface, const char *error_message)
{
  char *buf = NULL;
  // +4 = +colon +space +newline +nul
  buf = new char[strlen(error_preface) + strlen(error_message) + 4];
  sprintf(buf, "%s: %s\n", error_preface, error_message);
  MessageBox(NULL, buf, "Error", MB_OK);
  fprintf(stderr, "%s", buf);
  delete [] buf;
}

Either<texture_fullid,texture_ttfid> renderer::screen_to_texid(int x, int y) {
  const int tile = x * gps.dimy + y;
  const unsigned char *s = screen + tile*4;

  struct texture_fullid ret;
  int ch;
  int bold;
  int fg;
  int bg;

  // TTF text does not get the full treatment.
  if (s[3] == GRAPHICSTYPE_TTF) {
    texture_ttfid texpos = *((unsigned int *)s) & 0xffffff;
    return Either<texture_fullid,texture_ttfid>(texpos);
  } else if (s[3] == GRAPHICSTYPE_TTFCONT) {
    // TTFCONT means this is a tile that does not have TTF anchored on it, but is covered by TTF.
    // Since this may actually be stale information, we'll draw it as a blank space,
    ch = 32;
    fg = bg = bold = 0;
  } else {
    // Otherwise, it's a normal (graphical?) tile.
    ch   = s[0];
    bold = (s[3] != 0) * 8;
    fg   = (s[1] + bold) % 16;
    bg   = s[2] % 16;
  }
  
  static bool use_graphics = init.display.flag.has_flag(INIT_DISPLAY_FLAG_USE_GRAPHICS);
  
  if (use_graphics) {
    const long texpos             = screentexpos[tile];
    const char addcolor           = screentexpos_addcolor[tile];
    const unsigned char grayscale = screentexpos_grayscale[tile];
    const unsigned char cf        = screentexpos_cf[tile];
    const unsigned char cbr       = screentexpos_cbr[tile];

    if (texpos) {
      ret.texpos = texpos;
      if (grayscale) {
        ret.r = enabler.ccolor[cf][0];
        ret.g = enabler.ccolor[cf][1];
        ret.b = enabler.ccolor[cf][2];
        ret.br = enabler.ccolor[cbr][0];
        ret.bg = enabler.ccolor[cbr][1];
        ret.bb = enabler.ccolor[cbr][2];
      } else if (addcolor) {
        goto use_ch;
      } else {
        ret.r = ret.g = ret.b = 1;
        ret.br = ret.bg = ret.bb = 0;
      }
      goto skip_ch;
    }
  }
  
  ret.texpos = enabler.is_fullscreen() ?
    init.font.large_font_texpos[ch] :
    init.font.small_font_texpos[ch];
 use_ch:
  ret.r = enabler.ccolor[fg][0];
  ret.g = enabler.ccolor[fg][1];
  ret.b = enabler.ccolor[fg][2];
  ret.br = enabler.ccolor[bg][0];
  ret.bg = enabler.ccolor[bg][1];
  ret.bb = enabler.ccolor[bg][2];

 skip_ch:

  return Either<texture_fullid,texture_ttfid>(ret);
}


#ifdef CURSES
# include "renderer_curses.cpp"
#endif
#include "renderer_2d.hpp"
#include "renderer_opengl.hpp"


enablerst::enablerst() {
  fullscreen = false;
  sync = NULL;
  renderer = NULL;
  calculated_fps = calculated_gfps = frame_sum = gframe_sum = frame_last = gframe_last = 0;
  fps = 100; gfps = 20;
  fps_per_gfps = fps / gfps;
  last_tick = 0;
}

void renderer::display()
{
  const int dimx = init.display.grid_x;
  const int dimy = init.display.grid_y;
  static bool use_graphics = init.display.flag.has_flag(INIT_DISPLAY_FLAG_USE_GRAPHICS);
  if (gps.force_full_display_count) {
    // Update the entire screen
    update_all();
  } else {
    Uint32 *screenp = (Uint32*)screen, *oldp = (Uint32*)screen_old;
    if (use_graphics) {
      int off = 0;
      for (int x2=0; x2 < dimx; x2++) {
        for (int y2=0; y2 < dimy; y2++, ++off, ++screenp, ++oldp) {
          // We don't use pointers for the non-screen arrays because we mostly fail at the
          // *first* comparison, and having pointers for the others would exceed register
          // count.
          // Partial printing (and color-conversion): Big-ass if.
          if (*screenp == *oldp &&
              screentexpos[off] == screentexpos_old[off] &&
              screentexpos_addcolor[off] == screentexpos_addcolor_old[off] &&
              screentexpos_grayscale[off] == screentexpos_grayscale_old[off] &&
              screentexpos_cf[off] == screentexpos_cf_old[off] &&
              screentexpos_cbr[off] == screentexpos_cbr_old[off])
            {
              // Nothing's changed, this clause deliberately empty
            } else {
            update_tile(x2, y2);
          }
        }
      }
    } else {
      for (int x2=0; x2 < dimx; ++x2) {
        for (int y2=0; y2 < dimy; ++y2, ++screenp, ++oldp) {
          if (*screenp != *oldp) {
            update_tile(x2, y2);
          }
        }
      }
    }
  }
  if (gps.force_full_display_count > 0) gps.force_full_display_count--;
}

void renderer::cleanup_arrays() {
  if (screen) delete[] screen;
  if (screentexpos) delete[] screentexpos;
  if (screentexpos_addcolor) delete[] screentexpos_addcolor;
  if (screentexpos_grayscale) delete[] screentexpos_grayscale;
  if (screentexpos_cf) delete[] screentexpos_cf;
  if (screentexpos_cbr) delete[] screentexpos_cbr;
  if (screen_old) delete[] screen_old;
  if (screentexpos_old) delete[] screentexpos_old;
  if (screentexpos_addcolor_old) delete[] screentexpos_addcolor_old;
  if (screentexpos_grayscale_old) delete[] screentexpos_grayscale_old;
  if (screentexpos_cf_old) delete[] screentexpos_cf_old;
  if (screentexpos_cbr_old) delete[] screentexpos_cbr_old;
}

void renderer::gps_allocate(int x, int y) {
  cleanup_arrays();
  
  gps.screen = screen = new unsigned char[x*y*4];
  memset(screen, 0, x*y*4);
  gps.screentexpos = screentexpos = new long[x*y];
  memset(screentexpos, 0, x*y*sizeof(long));
  gps.screentexpos_addcolor = screentexpos_addcolor = new char[x*y];
  memset(screentexpos_addcolor, 0, x*y);
  gps.screentexpos_grayscale = screentexpos_grayscale = new unsigned char[x*y];
  memset(screentexpos_grayscale, 0, x*y);
  gps.screentexpos_cf = screentexpos_cf = new unsigned char[x*y];
  memset(screentexpos_cf, 0, x*y);
  gps.screentexpos_cbr = screentexpos_cbr = new unsigned char[x*y];
  memset(screentexpos_cbr, 0, x*y);

  screen_old = new unsigned char[x*y*4];
  memset(screen_old, 0, x*y*4);
  screentexpos_old = new long[x*y];
  memset(screentexpos_old, 0, x*y*sizeof(long));
  screentexpos_addcolor_old = new char[x*y];
  memset(screentexpos_addcolor_old, 0, x*y);
  screentexpos_grayscale_old = new unsigned char[x*y];
  memset(screentexpos_grayscale_old, 0, x*y);
  screentexpos_cf_old = new unsigned char[x*y];
  memset(screentexpos_cf_old, 0, x*y);
  screentexpos_cbr_old = new unsigned char[x*y];
  memset(screentexpos_cbr_old, 0, x*y);

  gps.resize(x,y);
}

void renderer::swap_arrays() {
  screen = screen_old; screen_old = gps.screen; gps.screen = screen;
  screentexpos = screentexpos_old; screentexpos_old = gps.screentexpos; gps.screentexpos = screentexpos;
  screentexpos_addcolor = screentexpos_addcolor_old; screentexpos_addcolor_old = gps.screentexpos_addcolor; gps.screentexpos_addcolor = screentexpos_addcolor;
  screentexpos_grayscale = screentexpos_grayscale_old; screentexpos_grayscale_old = gps.screentexpos_grayscale; gps.screentexpos_grayscale = screentexpos_grayscale;
  screentexpos_cf = screentexpos_cf_old; screentexpos_cf_old = gps.screentexpos_cf; gps.screentexpos_cf = screentexpos_cf;
  screentexpos_cbr = screentexpos_cbr_old; screentexpos_cbr_old = gps.screentexpos_cbr; gps.screentexpos_cbr = screentexpos_cbr;

  gps.screen_limit = gps.screen + gps.dimx * gps.dimy * 4;
}

void enablerst::pause_async_loop()  {
  struct async_cmd cmd;
  cmd.cmd = async_cmd::pause;
  async_tobox.write(cmd);
  async_wait();
}

// Wait until the previous command has been acknowledged, /or/
// async_loop has quit. Incidentally execute any requests in the
// meantime.
void enablerst::async_wait() {
  if (loopvar == 0) return;
  async_msg r;
  bool reset_textures = false;
  for (;;) {
    async_frombox.read(r);
    switch (r.msg) {
    case async_msg::quit:
      loopvar = 0;
      return;
    case async_msg::complete:
      if (reset_textures) {
        puts("Resetting textures");
        textures.remove_uploaded_textures();
        textures.upload_textures();
      }
      return;
    case async_msg::set_fps:
      set_fps(r.fps);
      async_fromcomplete.write();
      break;
    case async_msg::set_gfps:
      set_gfps(r.fps);
      async_fromcomplete.write();
      break;
    case async_msg::push_resize:
      override_grid_size(r.x, r.y);
      async_fromcomplete.write();
      break;
    case async_msg::pop_resize:
      release_grid_size();
      async_fromcomplete.write();
      break;
    case async_msg::reset_textures:
      reset_textures = true;
      break;
    default:
      puts("EMERGENCY: Unknown case in async_wait");
      abort();
    }
  }
}

void enablerst::async_loop() {
  async_paused = false;
  async_frames = 0;
  int total_frames = 0;
  int fps = 100; // Just a thread-local copy
  for (;;) {
    // cout << "FRAMES: " << frames << endl;
    // Check for commands
    async_cmd cmd;
    bool have_cmd = true;
    do {
      if (async_paused || (async_frames == 0 && !(enabler.flag & ENABLERFLAG_MAXFPS)))
        async_tobox.read(cmd);
      else
        have_cmd = async_tobox.try_read(cmd);
      // Obey the command, would you kindly.
      if (have_cmd) {
        switch (cmd.cmd) {
        case async_cmd::pause:
          async_paused = true;
          // puts("Paused");
          async_frombox.write(async_msg(async_msg::complete));
          break;
        case async_cmd::start:
          async_paused = false;
          async_frames = 0;
          // puts("UNpaused");
          break;
        case async_cmd::render:
          if (flag & ENABLERFLAG_RENDER) {
            total_frames++;
            renderer->swap_arrays();
            if (total_frames % 1800 == 0)
              ttf_manager.gc();
            render_things();
            flag &= ~ENABLERFLAG_RENDER;
            update_gfps();
          }
          async_frombox.write(async_msg(async_msg::complete));
          break;
        case async_cmd::inc:
          async_frames += cmd.val;
          if (async_frames > fps*3) async_frames = fps*3; // Just in case
          break;
        case async_cmd::set_fps:
          fps = cmd.val;
          break;
        }
      }
    } while (have_cmd);
    // Run the main-loop, maybe
    if (!async_paused && (async_frames || (enabler.flag & ENABLERFLAG_MAXFPS))) {
      if (mainloop()) {
        async_frombox.write(async_msg(async_msg::quit));
        return; // We're done.
      }
      simticks.lock();
      simticks.val++;
      simticks.unlock();
      async_frames--;
      if (async_frames < 0) async_frames = 0;
      update_fps();
    }
    SDL_NumJoysticks(); // Hook for dfhack
  }
}

void enablerst::do_frame() {
  // Check how long it's been, exactly
  const Uint32 now = SDL_GetTicks();
  const Uint32 interval = CLAMP(now - last_tick, 0, 1000); // Anything above a second doesn't count
  // cout << last_tick << " + " << interval << " = " << now << endl;
  last_tick = now;

  // Update outstanding-frame counts
  outstanding_frames += interval * fps / 1000;
  outstanding_gframes += interval * gfps / 1000;
  if (outstanding_gframes > 3) {
    outstanding_gframes = 3;
  }
  // cout << outstanding_frames << " " << outstanding_gframes << endl;
 
  // Update the loop's tick-counter suitably
  if (outstanding_frames >= 1) {
    async_cmd cmd(async_cmd::inc);
    cmd.val = outstanding_frames;
    outstanding_frames -= cmd.val;
    async_tobox.write(cmd);
  }

  // Store the current time, for things that are fine with approximations
  enabler.clock = SDL_GetTicks();

  // If it's time to render..
  if (outstanding_gframes >= 1 &&
      (!sync || glClientWaitSync(sync, 0, 0) == GL_ALREADY_SIGNALED)) {
    // Get the async-loop to render_things
    async_cmd cmd(async_cmd::render);
    async_tobox.write(cmd);
    async_wait();
    // Then finish here
    renderer->display();
    renderer->render();
    gputicks.lock();
    gputicks.val++;
    gputicks.unlock();
    outstanding_gframes--;
  }

  // Sleep until the next gframe
  if (outstanding_gframes < 1) {
    float fragment = 1 - outstanding_gframes;
    float milliseconds = fragment / gfps * 1000;
    // cout << milliseconds << endl;
    SDL_Delay(milliseconds);
  }
}

void enablerst::eventLoop_SDL()
{
  
  SDL_Event event;
  const SDL_Surface *screen = SDL_GetVideoSurface();
  Uint32 mouse_lastused = 0;
  SDL_ShowCursor(SDL_DISABLE);
 
  // Initialize the grid
  renderer->resize(screen->w, screen->h);

  while (loopvar) {
    Uint32 now = SDL_GetTicks();
    bool paused_loop = false;

    // Check for zoom commands
    zoom_commands zoom;
    while (async_zoom.try_read(zoom)) {
      if (overridden_grid_sizes.size())
        continue; // No zooming in movies
      if (!paused_loop) {
        pause_async_loop();
        paused_loop = true;
      }
      if (zoom == zoom_fullscreen)
        renderer->set_fullscreen();
      else
        renderer->zoom(zoom);
    }

    // Check for SDL events
    while (SDL_PollEvent(&event)) {
      // Make sure mainloop isn't running while we're processing input
      if (!paused_loop) {
        pause_async_loop();
        paused_loop = true;
      }
      // Handle SDL events
      switch (event.type) {
      case SDL_KEYDOWN:
        // Disable mouse if it's been long enough
        if (mouse_lastused + 5000 < now) {
          if(init.input.flag.has_flag(INIT_INPUT_FLAG_MOUSE_PICTURE)) {
            // hide the mouse picture
            // enabler.set_tile(0, TEXTURE_MOUSE, enabler.mouse_x, enabler.mouse_y);
          }
          SDL_ShowCursor(SDL_DISABLE);
        }
      case SDL_KEYUP:
      case SDL_QUIT:
        enabler.add_input(event, now);
        break;
      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
        if (!init.input.flag.has_flag(INIT_INPUT_FLAG_MOUSE_OFF)) {
          int isdown = (event.type == SDL_MOUSEBUTTONDOWN);
          if (event.button.button == SDL_BUTTON_LEFT) {
            enabler.mouse_lbut = isdown;
            enabler.mouse_lbut_down = isdown;
            if (!isdown)
              enabler.mouse_lbut_lift = 0;
          } else if (event.button.button == SDL_BUTTON_RIGHT) {
            enabler.mouse_rbut = isdown;
            enabler.mouse_rbut_down = isdown;
            if (!isdown)
              enabler.mouse_rbut_lift = 0;
          } else
            enabler.add_input(event, now);
        }
        break;
      case SDL_MOUSEMOTION:
        // Deal with the mouse hiding bit
        mouse_lastused = now;
        if(init.input.flag.has_flag(INIT_INPUT_FLAG_MOUSE_PICTURE)) {
          // turn on mouse picture
          // enabler.set_tile(gps.tex_pos[TEXTURE_MOUSE], TEXTURE_MOUSE,enabler.mouse_x, enabler.mouse_y);
        } else {
          SDL_ShowCursor(SDL_ENABLE);
        }
        break;
      case SDL_ACTIVEEVENT:
        enabler.clear_input();
        if (event.active.state & SDL_APPACTIVE) {
          if (event.active.gain) {
            enabler.flag|=ENABLERFLAG_RENDER;
            gps.force_full_display_count++;
          }
        }
        break;
      case SDL_VIDEOEXPOSE:
        gps.force_full_display_count++;
        enabler.flag|=ENABLERFLAG_RENDER;
        break;
      case SDL_VIDEORESIZE:
        if (is_fullscreen());
          //errorlog << "Caught resize event in fullscreen??\n";
        else {
          //gamelog << "Resizing window to " << event.resize.w << "x" << event.resize.h << endl << flush;
          renderer->resize(event.resize.w, event.resize.h);
        }
        break;
      } // switch (event.type)
    } //while have event

    // Update mouse state
    if (!init.input.flag.has_flag(INIT_INPUT_FLAG_MOUSE_OFF)) {
      int mouse_x = -1, mouse_y = -1, mouse_state;
      // Check whether the renderer considers this valid input or not, and write it to gps
      if ((SDL_GetAppState() & SDL_APPMOUSEFOCUS) &&
          renderer->get_mouse_coords(mouse_x, mouse_y)) {
        mouse_state = 1;
      } else {
        mouse_state = 0;
      }
      if (mouse_x != gps.mouse_x || mouse_y != gps.mouse_y ||
          mouse_state != enabler.tracking_on) {
        // Pause rendering loop and update values
        if (!paused_loop) {
          pause_async_loop();
          paused_loop = true;
        }
        enabler.tracking_on = mouse_state;
        gps.mouse_x = mouse_x;
        gps.mouse_y = mouse_y;
      }
    }

    if (paused_loop)
      unpause_async_loop();

    do_frame();
	/*
#if !defined(NO_FMOD)
    // Call FMOD::System.update(). Manages a bunch of sound stuff.
    musicsound.update();
#endif
	*/
  }
}

int enablerst::loop(string cmdline) {
  command_line = cmdline;

  // Initialize the tick counters
  simticks.write(0);
  gputicks.write(0);
  
  // Call DF's initialization routine
  if (!beginroutine())
    exit(EXIT_FAILURE);
  
  // Allocate a renderer
  if (init.display.flag.has_flag(INIT_DISPLAY_FLAG_TEXT)) {
#ifdef CURSES
    renderer = new renderer_curses();
#else
    report_error("PRINT_MODE", "TEXT not supported on windows");
    exit(EXIT_FAILURE);
#endif
  } else if (init.display.flag.has_flag(INIT_DISPLAY_FLAG_2D)) {
    renderer = new renderer_2d();
  } else if (init.display.flag.has_flag(INIT_DISPLAY_FLAG_ACCUM_BUFFER)) {
    renderer = new renderer_accum_buffer();
  } else if (init.display.flag.has_flag(INIT_DISPLAY_FLAG_FRAME_BUFFER)) {
    renderer = new renderer_framebuffer();
  } else if (init.display.flag.has_flag(INIT_DISPLAY_FLAG_PARTIAL_PRINT)) {
    if (init.display.partial_print_count)
      renderer = new renderer_partial();
    else
      renderer = new renderer_once();
  } else if (init.display.flag.has_flag(INIT_DISPLAY_FLAG_VBO)) {
    renderer = new renderer_vbo();
  } else {
    renderer = new renderer_opengl();
  }

  // At this point we should have a window that is setup to render DF.
  if (init.display.flag.has_flag(INIT_DISPLAY_FLAG_TEXT)) {
#ifdef CURSES
    eventLoop_ncurses();
#endif
  } else {
    SDL_EnableUNICODE(1);
    eventLoop_SDL();
  }

  endroutine();

  // Clean up graphical resources
  delete renderer;
}

void enablerst::override_grid_size(int x, int y) {
  if (SDL_ThreadID() != renderer_threadid) {
    // Ask the renderer to do it
    async_msg m(async_msg::push_resize);
    m.x = x; m.y = y;
    async_frombox.write(m);
    async_fromcomplete.read();
  } else {
    // We are the renderer; do it.
    overridden_grid_sizes.push(make_pair(init.display.grid_x,init.display.grid_y));
    renderer->grid_resize(x, y);
  }
}

void enablerst::release_grid_size() {
  if (SDL_ThreadID() != renderer_threadid) {
    async_frombox.write(async_msg(async_msg::pop_resize));
    async_fromcomplete.read();
  } else {
    if (!overridden_grid_sizes.size()) return;
    // FIXME: Find out whatever is causing release to be called too rarely; right now
    // we're overriding once per movie but apparently only releasing for the last one.
    pair<int,int> sz;
    while (overridden_grid_sizes.size()) {
      sz = overridden_grid_sizes.top();
      overridden_grid_sizes.pop();
    }
    zoom_display(zoom_resetgrid);
  }
}

void enablerst::zoom_display(zoom_commands command) {
  async_zoom.write(command);
}

int enablerst::calculate_fps() {
  if (frame_timings.size() < 50)
    return get_fps();
  else
    return calculated_fps;
}
int enablerst::calculate_gfps() {
  if (gframe_timings.size() < 50)
    return get_gfps();
  else
    return calculated_gfps;
}

void enablerst::do_update_fps(queue<int> &q, int &sum, int &last, int &calc) {
  while (q.size() > 50 && sum > 10000) {
    sum -= q.front();
    q.pop();
  }
  const int now = SDL_GetTicks();
  const int interval = now - last;
  q.push(interval);
  sum += interval;
  last = now;
  if (sum)
    calc = (int)q.size() * 1000 / sum;
}

void enablerst::clear_fps() {
  while (frame_timings.size())
    frame_timings.pop();
  frame_sum = 0;
  frame_last = SDL_GetTicks();
  calculated_fps = get_fps();
}

void enablerst::update_fps() {
  do_update_fps(frame_timings, frame_sum, frame_last, calculated_fps);
}

void enablerst::update_gfps() {
  do_update_fps(gframe_timings, gframe_sum, gframe_last, calculated_gfps);
}

void enablerst::set_fps(int fps) {
  if (SDL_ThreadID() != renderer_threadid) {
    async_msg m(async_msg::set_fps);
    m.fps = fps;
    async_paused = true;
    async_frombox.write(m);
    async_fromcomplete.read();
  } else {
    if (fps == 0)
      fps = 1048576;
    this->fps = fps;
    fps_per_gfps = fps / gfps;
    struct async_cmd cmd;
    cmd.cmd = async_cmd::set_fps;
    cmd.val = fps;
    async_tobox.write(cmd);
    async_tobox.write(async_cmd(async_cmd::start));
  }
}

void enablerst::set_gfps(int gfps) {
  if (SDL_ThreadID() != renderer_threadid) {
    async_msg m(async_msg::set_gfps);
    m.fps = gfps;
    async_frombox.write(m);
    async_fromcomplete.read();
  } else {
    if (gfps == 0)
      gfps = 50;
    this->gfps = gfps;
    fps_per_gfps = fps / gfps;
  }
}

int call_loop(void *dummy) {
  enabler.async_loop();
  return 0;
}

int main (int argc, char* argv[]) {
#ifdef unix
  setlocale(LC_ALL, "");
#endif
#if !defined(__APPLE__) && defined(unix)
  bool gtk_ok = false;
  if (getenv("DISPLAY"))
    gtk_ok = gtk_init_check(&argc, &argv);
#endif

  // Initialise minimal SDL subsystems.
  int retval = SDL_Init(SDL_INIT_TIMER);
  // Report failure?
  if (retval != 0) {
    report_error("SDL initialization failure", SDL_GetError());
    return false;
  }
  enabler.renderer_threadid = SDL_ThreadID();

  // Spawn simulation thread
  SDL_CreateThread(call_loop, NULL);

  init.begin(); // Load init.txt settings
  
#if !defined(__APPLE__) && defined(unix)
  if (!gtk_ok && !init.display.flag.has_flag(INIT_DISPLAY_FLAG_TEXT)) {
    puts("Display not found and PRINT_MODE not set to TEXT, aborting.");
    exit(EXIT_FAILURE);
  }
  if (init.display.flag.has_flag(INIT_DISPLAY_FLAG_TEXT) &&
      init.display.flag.has_flag(INIT_DISPLAY_FLAG_USE_GRAPHICS)) {
    puts("Graphical tiles are not compatible with text output, sorry");
    exit(EXIT_FAILURE);
  }
#endif

  // Initialize video, if we /use/ video
  retval = SDL_InitSubSystem(init.display.flag.has_flag(INIT_DISPLAY_FLAG_TEXT) ? 0 : SDL_INIT_VIDEO);
  if (retval != 0) {
    report_error("SDL initialization failure", SDL_GetError());
    return false;
  }
  
#ifdef linux
  if (!init.media.flag.has_flag(INIT_MEDIA_FLAG_SOUND_OFF)) {
    // Initialize OpenAL
    if (!musicsound.initsound()) {
      puts("Initializing OpenAL failed, no sound will be played");
      init.media.flag.add_flag(INIT_MEDIA_FLAG_SOUND_OFF);
    }
  }
#endif

#ifdef WIN32
  // Attempt to get as good a timer as possible
  int ms = 1;
  while (timeBeginPeriod(ms) != TIMERR_NOERROR) ms++;
#endif

  // Load keyboard map
  keybinding_init();
  enabler.load_keybindings("data/init/interface.txt");

  string cmdLine;
  for (int i = 1; i < argc; ++i) { 
    char *option = argv[i];
	string opt=option;
	if(opt.length()>=1)
		{
		//main removes quotes, unlike the winmain version, so it has to be rebuilt
		if(opt[0]=='-')
			{
			cmdLine += opt;
			cmdLine += " ";
			}
		else
			{
			cmdLine += "\"";
			cmdLine += opt;
			cmdLine += "\"";
			cmdLine += " ";
			}
		}
  }
  int result = enabler.loop(cmdLine);

  SDL_Quit();

#ifdef WIN32
  timeEndPeriod(ms);
#endif
  
  return result;
}

void text_system_file_infost::initialize_info()
{
  std::ifstream fseed(filename.c_str());
  if(fseed.is_open())
    {
      string str;

      while(std::getline(fseed,str))
	{
	  if(str.length()>0)number++;
	}
    }
  else
    {
      string str;
      str="Error Initializing Text: ";
      str+=filename;
      errorlog_string(str);
    }
  fseed.close();
}

void text_system_file_infost::get_text(text_infost &text)
{
  text.clean();

  if(number==0)return;

  std::ifstream fseed(filename.c_str());
  if(fseed.is_open())
    {
      string str;

      int num=trandom(number);

      //SKIP AHEAD TO THE RIGHT SPOT
      while(num>0)
	{
	  std::getline(fseed,str);
	  num--;
	}

      //PROCESS THE STRING INTO TEXT ELEMENTS
      if(std::getline(fseed,str))
	{
	  int curpos;
	  string nextstr;
	  char doing_long=0;

	  text_info_elementst *newel;
	  long end=(long)str.length();
			
	  while(end>0)
	    {
	      if(isspace(str[end-1]))end--;
	      else break;
	    }
			
	  str.resize(end);

	  for(curpos=0;curpos<end;curpos++)
	    {
	      //HANDLE TOKEN OR ENDING
	      //TWO FILE TOKENS IN A ROW MEANS LONG
	      //ONE MEANS STRING
	      if(str[curpos]==file_token || curpos==end-1)
		{
		  if(str[curpos]!=file_token)nextstr+=str[curpos];

		  //HAVE SOMETHING == SAVE IT
		  if(!nextstr.empty())
		    {
		      if(doing_long)
			{
			  newel=new text_info_element_longst(atoi(nextstr.c_str()));
			  text.element.push_back(newel);
			  doing_long=0;
			}
		      else
			{
			  newel=new text_info_element_stringst(nextstr);
			  text.element.push_back(newel);
			}

		      nextstr.erase();
		    }
		  //STARTING A LONG
		  else
		    {
		      doing_long=1;
		    }
		}
	      //JUST ADD IN ANYTHING ELSE
	      else
		{
		  nextstr+=str[curpos];
		}
	    }
	}
    }
  fseed.close();
}

void curses_text_boxst::add_paragraph(const string &src,int32_t para_width)
{
	stringvectst sp;
	sp.add_string(src);
	add_paragraph(sp,para_width);
}

void curses_text_boxst::add_paragraph(stringvectst &src,int32_t para_width)
{
	bool skip_leading_spaces=false;

	//ADD EACH OF THE STRINGS ON IN TURN
	string curstr;
	long strlength=0;
	long s,pos;
	for(s=0;s<src.str.size();s++)
		{
		//GRAB EACH WORD, AND SEE IF IT FITS, IF NOT START A NEW LINE
		for(pos=0;pos<src.str[s]->dat.size();pos++)
			{
			if(skip_leading_spaces)
				{
				if(src.str[s]->dat[pos]==' ')continue;
				else skip_leading_spaces=false;
				}

			//ADD TO WORD
			curstr+=src.str[s]->dat[pos];

			//IF TOO LONG, CUT BACK TO FIRST SPACE
			if(curstr.length()>para_width)
				{
				long opos=pos;

				long minus=0;
				do
					{
					pos--;
					minus++;
					}while(src.str[s]->dat[pos]!=' '&&pos>0);

				//IF WENT ALL THE WAY BACK, INTRODUCE A SPACE
				if(minus==curstr.size())
					{
					src.str[s]->dat.insert(opos-1," ");
					}
				else
					{
					curstr.resize(curstr.size()-minus);
					text.add_string(curstr);
					skip_leading_spaces=true;
					}
				curstr.erase();
				}
			}
		}

	//FLUSH FINAL BIT
	if(!curstr.empty())text.add_string(curstr);
}