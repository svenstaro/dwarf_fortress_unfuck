#include "renderer_2d.hpp"


bool renderer_offscreen::init_video(int w, int h) {
  if (screen) SDL_FreeSurface(screen);
  // Create an offscreen buffer
  screen = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32, 0, 0, 0, 0);
  assert(screen);
  return true;
}

renderer_offscreen::~renderer_offscreen() {
  //ASSUMES renderer_offscreen IS NEVER gps_allocate()'d THROUGH reshape()/grid_resize()
		//to-do: flag for those calls on the renderer to control this behavior?
  renderer::screen = NULL;
  renderer::screentexpos = NULL;
  renderer::screentexpos_addcolor = NULL;
  renderer::screentexpos_grayscale = NULL;
  renderer::screentexpos_cf = NULL;
  renderer::screentexpos_cbr = NULL;
  renderer::screen_old = NULL;
  renderer::screentexpos_old = NULL;
  renderer::screentexpos_addcolor_old = NULL;
  renderer::screentexpos_grayscale_old = NULL;
  renderer::screentexpos_cf_old = NULL;
  renderer::screentexpos_cbr_old = NULL;

  SDL_FreeSurface(screen);
}

// Create an offscreen renderer of a given grid-size
renderer_offscreen::renderer_offscreen(int grid_x, int grid_y) {
  screen = NULL;
  dispx = enabler.is_fullscreen() ?
    init.font.large_font_dispx :
    init.font.small_font_dispx;
  dispy = enabler.is_fullscreen() ?
    init.font.large_font_dispy :
    init.font.small_font_dispy;
  dispx_z = dispx;
  dispy_z = dispy;
  origin_x = origin_y = 0;
  zoom_steps = forced_steps = 0;
  natural_w = dispx * grid_x;
  natural_h = dispy * grid_y;
  dimx = grid_x;
  dimy = grid_y;
  init_video(natural_w, natural_h);
  // Copy the GPS pointers here
  renderer::screen = gps.screen;
  renderer::screentexpos = gps.screentexpos;
  renderer::screentexpos_addcolor = gps.screentexpos_addcolor;
  renderer::screentexpos_grayscale = gps.screentexpos_grayscale;
  renderer::screentexpos_cf = gps.screentexpos_cf;
  renderer::screentexpos_cbr = gps.screentexpos_cbr;
}

// Slurp the entire gps content into the renderer at some given offset
void renderer_offscreen::update_all(int offset_x, int offset_y) {
  for (int x = 0; x < gps.dimx; x++) {
    for (int y = 0; y < gps.dimy; y++) {
      // Read tiles from gps, create cached texture
      Either<texture_fullid,texture_ttfid> id = screen_to_texid(x, y);
      SDL_Surface *tex = id.isL ?
        tile_cache_lookup(id.left, false) :
        ttf_manager.get_texture(id.right);
      if (id.isL) {
        tex = tile_cache_lookup(id.left);
      } else {
        tex = enabler.textures.get_texture_data(id.right);
      }
      // Figure out where to blit
      SDL_Rect dst;
      dst.x = dispx * (x+offset_x);
      dst.y = dispy * (y+offset_y);
      // And blit.
      SDL_BlitSurface(tex, NULL, screen, &dst);
    }
  }
}

// Save the image to some file
void renderer_offscreen::save_to_file(const string &file) {
  // TODO: Support png, etc.
  SDL_SaveBMP(screen, file.c_str());
}
