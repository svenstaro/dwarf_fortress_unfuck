#include "enabler.h"
#include "init.h"
#include "resize++.h"
#include "ttf_manager.hpp"

#include <iostream>
using namespace std;

void report_error(const char*, const char*);

class renderer_2d_base : public renderer {
protected:
  SDL_Surface *screen;
  map<texture_fullid, SDL_Surface*> tile_cache;
  int dispx, dispy, dimx, dimy;
  // We may shrink or enlarge dispx/dispy in response to zoom requests. dispx/y_z are the
  // size we actually display tiles at.
  int dispx_z, dispy_z;
  // Viewport origin
  int origin_x, origin_y;

  SDL_Surface *tile_cache_lookup(texture_fullid &id, bool convert=true) {
    map<texture_fullid, SDL_Surface*>::iterator it = tile_cache.find(id);
    if (it != tile_cache.end()) {
      return it->second;
    } else {
      // Create the colorized texture
      SDL_Surface *tex   = enabler.textures.get_texture_data(id.texpos);
      SDL_Surface *color;
      color = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                   tex->w, tex->h,
                                   tex->format->BitsPerPixel,
                                   tex->format->Rmask,
                                   tex->format->Gmask,
                                   tex->format->Bmask,
                                   0);
      if (!color) {
        MessageBox (NULL, "Unable to create texture!", "Fatal error", MB_OK | MB_ICONEXCLAMATION);
        abort();
      }
      
      // Fill it
      Uint32 color_fgi = SDL_MapRGB(color->format, id.r*255, id.g*255, id.b*255);
      Uint8 *color_fg = (Uint8*) &color_fgi;
      Uint32 color_bgi = SDL_MapRGB(color->format, id.br*255, id.bg*255, id.bb*255);
      Uint8 *color_bg = (Uint8*) &color_bgi;
      SDL_LockSurface(tex);
      SDL_LockSurface(color);
      
      Uint8 *pixel_src, *pixel_dst;
      for (int y = 0; y < tex->h; y++) {
        pixel_src = ((Uint8*)tex->pixels) + (y * tex->pitch);
        pixel_dst = ((Uint8*)color->pixels) + (y * color->pitch);
        for (int x = 0; x < tex->w; x++, pixel_src+=4, pixel_dst+=4) {
          float alpha = pixel_src[3] / 255.0;
          for (int c = 0; c < 3; c++) {
            float fg = color_fg[c] / 255.0, bg = color_bg[c] / 255.0, tex = pixel_src[c] / 255.0;
            pixel_dst[c] = ((alpha * (tex * fg)) + ((1 - alpha) * bg)) * 255;
          }
        }
      }
      
      SDL_UnlockSurface(color);
      SDL_UnlockSurface(tex);
      
      SDL_Surface *disp = convert ?
        SDL_Resize(color, dispx_z, dispy_z) :  // Convert to display format; deletes color
        color;  // color is not deleted, but we don't want it to be.
      // Insert and return
      tile_cache[id] = disp;
      return disp;
    }
  }
  
  virtual bool init_video(int w, int h) {
    // Get ourselves a 2D SDL window
    Uint32 flags = init.display.flag.has_flag(INIT_DISPLAY_FLAG_2DHW) ? SDL_HWSURFACE : SDL_SWSURFACE;
    flags |= init.display.flag.has_flag(INIT_DISPLAY_FLAG_2DASYNC) ? SDL_ASYNCBLIT : 0;

    // Set it up for windowed or fullscreen, depending.
    if (enabler.is_fullscreen()) { 
      flags |= SDL_FULLSCREEN;
    } else {
      if (!init.display.flag.has_flag(INIT_DISPLAY_FLAG_NOT_RESIZABLE))
        flags |= SDL_RESIZABLE;
    }

    // (Re)create the window
    screen = SDL_SetVideoMode(w, h, 32, flags);
    if (screen == NULL) cout << "INIT FAILED!" << endl;

    return screen != NULL;
  }
  
public:
  list<pair<SDL_Surface*,SDL_Rect> > ttfs_to_render;
  
  void update_tile(int x, int y) {
    // Figure out where to blit
    SDL_Rect dst;
    dst.x = dispx_z * x + origin_x;
    dst.y = dispy_z * y + origin_y;
    // Read tiles from gps, create cached texture
    Either<texture_fullid,texture_ttfid> id = screen_to_texid(x, y);
    SDL_Surface *tex;
    if (id.isL) {      // Ordinary tile, cached here
      tex = tile_cache_lookup(id.left);
      // And blit.
      SDL_BlitSurface(tex, NULL, screen, &dst);
    } else {  // TTF, cached in ttf_manager so no point in also caching here
      tex = ttf_manager.get_texture(id.right);
      // Blit later
      ttfs_to_render.push_back(make_pair(tex, dst));
    }
  }

  void update_all() {
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));
    for (int x = 0; x < gps.dimx; x++)
      for (int y = 0; y < gps.dimy; y++)
        update_tile(x, y);
  }

  virtual void render() {
    // Render the TTFs, which we left for last
    for (auto it = ttfs_to_render.begin(); it != ttfs_to_render.end(); ++it) {
      SDL_BlitSurface(it->first, NULL, screen, &it->second);
    }
    ttfs_to_render.clear();
    // And flip out.
    SDL_Flip(screen);
  }

  virtual ~renderer_2d_base() {
	for (auto it = tile_cache.cbegin(); it != tile_cache.cend(); ++it)
		SDL_FreeSurface(it->second);
	for (auto it = ttfs_to_render.cbegin(); it != ttfs_to_render.cend(); ++it)
		SDL_FreeSurface(it->first);
  }

  void grid_resize(int w, int h) {
    dimx = w; dimy = h;
    // Only reallocate the grid if it actually changes
    if (init.display.grid_x != dimx || init.display.grid_y != dimy)
      gps_allocate(dimx, dimy);
    // But always force a full display cycle
    gps.force_full_display_count = 1;
    enabler.flag |= ENABLERFLAG_RENDER;    
  }

  renderer_2d_base() {
    zoom_steps = forced_steps = 0;
  }
  
  int zoom_steps, forced_steps;
  int natural_w, natural_h;

  void compute_forced_zoom() {
    forced_steps = 0;
    pair<int,int> zoomed = compute_zoom();
    while (zoomed.first < MIN_GRID_X || zoomed.second < MIN_GRID_Y) {
      forced_steps++;
      zoomed = compute_zoom();
    }
    while (zoomed.first > MAX_GRID_X || zoomed.second > MAX_GRID_Y) {
      forced_steps--;
      zoomed = compute_zoom();
    }
  }

  pair<int,int> compute_zoom(bool clamp = false) {
    const int dispx = enabler.is_fullscreen() ?
      init.font.large_font_dispx :
      init.font.small_font_dispx;
    const int dispy = enabler.is_fullscreen() ?
      init.font.large_font_dispy :
      init.font.small_font_dispy;
    int w, h;
    if (dispx < dispy) {
      w = natural_w + zoom_steps + forced_steps;
      h = double(natural_h) * (double(w) / double(natural_w));
    } else {
      h = natural_h + zoom_steps + forced_steps;
      w = double(natural_w) * (double(h) / double(natural_h));
    }
    if (clamp) {
      w = MIN(MAX(w, MIN_GRID_X), MAX_GRID_X);
      h = MIN(MAX(h, MIN_GRID_Y), MAX_GRID_Y);
    }
    return make_pair(w,h);
  }

  
  void resize(int w, int h) {
    // We've gotten resized.. first step is to reinitialize video
    cout << "New window size: " << w << "x" << h << endl;
    init_video(w, h);
    dispx = enabler.is_fullscreen() ?
      init.font.large_font_dispx :
      init.font.small_font_dispx;
    dispy = enabler.is_fullscreen() ?
      init.font.large_font_dispy :
      init.font.small_font_dispy;
    cout << "Font size: " << dispx << "x" << dispy << endl;
    // If grid size is currently overridden, we don't change it
    if (enabler.overridden_grid_sizes.size() == 0) {
      // (Re)calculate grid-size
      dimx = MIN(MAX(w / dispx, MIN_GRID_X), MAX_GRID_X);
      dimy = MIN(MAX(h / dispy, MIN_GRID_Y), MAX_GRID_Y);
      cout << "Resizing grid to " << dimx << "x" << dimy << endl;
      grid_resize(dimx, dimy);
    }
    // Calculate zoomed tile size
    natural_w = MAX(w / dispx,1);
    natural_h = MAX(h / dispy,1);
    compute_forced_zoom();
    reshape(compute_zoom(true));
    cout << endl;
  }

  void reshape(pair<int,int> max_grid) {
    int w = max_grid.first,
      h = max_grid.second;
    // Compute the largest tile size that will fit this grid into the window, roughly maintaining aspect ratio
    double try_x = dispx, try_y = dispy;
    try_x = screen->w / w;
    try_y = MIN(try_x / dispx * dispy, screen->h / h);
    try_x = MIN(try_x, try_y / dispy * dispx);
    dispx_z = MAX(1,try_x); dispy_z = MAX(try_y,1);
    cout << "Resizing font to " << dispx_z << "x" << dispy_z << endl;
    // Remove now-obsolete tile catalog
    for (map<texture_fullid, SDL_Surface*>::iterator it = tile_cache.begin();
         it != tile_cache.end();
         ++it)
      SDL_FreeSurface(it->second);
    tile_cache.clear();
    // Recompute grid based on the new tile size
    w = CLAMP(screen->w / dispx_z, MIN_GRID_X, MAX_GRID_X);
    h = CLAMP(screen->h / dispy_z, MIN_GRID_Y, MAX_GRID_Y);
    // Reset grid size
#ifdef DEBUG
    cout << "Resizing grid to " << w << "x" << h << endl;
#endif
    gps_allocate(w,h);
    // Force redisplay
    gps.force_full_display_count = 1;
    // Calculate viewport origin, for centering
    origin_x = (screen->w - dispx_z * w) / 2;
    origin_y = (screen->h - dispy_z * h) / 2;
    // Reset TTF rendering
    ttf_manager.init(dispy_z, dispx_z);
  }

private:
  
  void set_fullscreen() {
    if (enabler.is_fullscreen()) {
      init.display.desired_windowed_width = screen->w;
      init.display.desired_windowed_height = screen->h;
      resize(init.display.desired_fullscreen_width,
             init.display.desired_fullscreen_height);
    } else {
      resize(init.display.desired_windowed_width, init.display.desired_windowed_height);
    }
  }

  bool get_mouse_coords(int &x, int &y) {
    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);
    mouse_x -= origin_x; mouse_y -= origin_y;
    if (mouse_x < 0 || mouse_x >= dispx_z*dimx ||
        mouse_y < 0 || mouse_y >= dispy_z*dimy)
      return false;
    x = mouse_x / dispx_z;
    y = mouse_y / dispy_z;
    return true;
  }

  void zoom(zoom_commands cmd) {
    pair<int,int> before = compute_zoom(true);
    int before_steps = zoom_steps;
    switch (cmd) {
    case zoom_in:    zoom_steps -= init.input.zoom_speed; break;
    case zoom_out:   zoom_steps += init.input.zoom_speed; break;
    case zoom_reset:
      zoom_steps = 0;
    case zoom_resetgrid:
      compute_forced_zoom();
      break;
    }
    pair<int,int> after = compute_zoom(true);
    if (after == before && (cmd == zoom_in || cmd == zoom_out))
      zoom_steps = before_steps;
    else
      reshape(after);
  }
  
};

class renderer_2d : public renderer_2d_base {
public:
  renderer_2d() {
    // Disable key repeat
    SDL_EnableKeyRepeat(0, 0);
    // Set window title/icon.
    SDL_WM_SetCaption(GAME_TITLE_STRING, NULL);
    SDL_Surface *icon = IMG_Load("data/art/icon.png");
    if (icon != NULL) {
      SDL_WM_SetIcon(icon, NULL);
      // The icon's surface doesn't get used past this point.
      SDL_FreeSurface(icon); 
    }
    
    // Find the current desktop resolution if fullscreen resolution is auto
    if (init.display.desired_fullscreen_width  == 0 ||
        init.display.desired_fullscreen_height == 0) {
      const struct SDL_VideoInfo *info = SDL_GetVideoInfo();
      init.display.desired_fullscreen_width = info->current_w;
      init.display.desired_fullscreen_height = info->current_h;
    }

    // Initialize our window
    bool worked = init_video(enabler.is_fullscreen() ?
                             init.display.desired_fullscreen_width :
                             init.display.desired_windowed_width,
                             enabler.is_fullscreen() ?
                             init.display.desired_fullscreen_height :
                             init.display.desired_windowed_height);

    // Fallback to windowed mode if fullscreen fails
    if (!worked && enabler.is_fullscreen()) {
      enabler.fullscreen = false;
      report_error("SDL initialization failure, trying windowed mode", SDL_GetError());
      worked = init_video(init.display.desired_windowed_width,
                          init.display.desired_windowed_height);
    }
    // Quit if windowed fails
    if (!worked) {
      report_error("SDL initialization failure", SDL_GetError());
      exit(EXIT_FAILURE);
    }
  }
};

class renderer_offscreen : public renderer_2d_base {
  virtual bool init_video(int, int);
public:
  virtual ~renderer_offscreen();
  renderer_offscreen(int, int);
  void update_all(int, int);
  void save_to_file(const string &file);
};
