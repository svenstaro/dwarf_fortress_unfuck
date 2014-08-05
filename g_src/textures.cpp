#include <cassert>

#include "enabler.h"
#include "init.h"

// Used to sort textures
struct vsize_pos {
  int h, w;
  SDL_Surface *s;
  long pos;
  // Assigned texture-catalog coordinates
  int x, y;

  bool operator< (struct vsize_pos y) const {
    // sort produces an ascending order. We want descending. Don't argue.
    if (h > y.h) return true;
    return false;
  }
};


// Check whether a particular texture can be sized to some size,
// assuming in RGBA 32-bit format
bool testTextureSize(GLuint texnum, int w, int h) {
  GLint gpu_width;
  glBindTexture(GL_TEXTURE_2D, texnum);
      printGLError();
  glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
      printGLError();
  glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &gpu_width);
      printGLError();

  if (gpu_width == w) return true;
  return false;
}

// Texture catalog implementation
void textures::upload_textures() {
  if (uploaded) return; // Don't bother
  if (!enabler.uses_opengl()) return; // No uploading
  glEnable(GL_TEXTURE_2D);
  printGLError();
  glGenTextures(1, &gl_catalog);
  printGLError();

  // First, sort the textures by vertical size. We'll want to place the large
  // ones first.
  // Since we mustn't alter the raws array, first thing is to create a new one.
  // We pretend textures are one pixel larger than they actually are in either
  // direction, to avoid border scuffles when interpolating.
  std::vector<vsize_pos> ordered;
  long pos = 0;
  for (std::vector<SDL_Surface *>::iterator it = raws.begin();
       it != raws.end(); ++it) {
    if (*it) {
      vsize_pos item;
      item.h = (*it)->h+2;
      item.w = (*it)->w+2;
      item.s = *it;
      item.pos = pos;
      ordered.push_back(item);
    }
    pos++;
  }
  sort(ordered.begin(), ordered.end());

  /* Tiling algorithm:
  **
  ** Given a particular texture width, we pack tiles from largest to smallest
  ** by reserving rows for tiles with a particular height or lower.
  ** This does lead to space wastage when a row has, say, one 32x32 tile and
  ** fifteen 16x16 tiles, but generally not very much.
  **
  ** Possible improvement: Allow for multiple rows of smaller tiles inside
  ** a row that's at least twice as high as the smaller tiles are.
   */

  // Set the initial width to the minimum possible
  int catalog_width = 0;
  for (int i=0; i < ordered.size(); i++)
    if (catalog_width < ordered[i].w) catalog_width = ordered[i].w;
  const int width_increment = 4; // For speed, not that it matters.
  int catalog_height;
  // Figure out what the optimal texture width is
  // This may not be actually be an approximately square texture, but for the
  // moment that's what we're aiming for. On GPUs without the NPOT extension,
  // rectangular textures may actulaly use less video memory.
  // However, a square one is less likely to run into dimensional limits.
  for(;;) {
    int catalog_x = 0;
    int catalog_y = 0;
    int row_height = ordered[0].h;
    catalog_height = row_height;
    for (int pos = 0; pos < ordered.size(); pos++) {
      // Check whether we must start a new row
      if (catalog_x + ordered[pos].w > catalog_width) {
	catalog_x = 0;
	catalog_y = catalog_height;
	row_height = ordered[pos].h;
	catalog_height += row_height;
      }
      // Tentatively install tile at catalog_x, catalog_y
      ordered[pos].x = catalog_x;
      ordered[pos].y = catalog_y;
      // Goto next tile
      catalog_x += ordered[pos].w;
    }
    // If we didn't just cross "square", increment width and try again.
    if (catalog_height > catalog_width)
      catalog_width += width_increment;
    else
      break; // Otherwise we're done.
   }

#ifdef DEBUG
  std::cout << "Ideal catalog size: " << catalog_width << "x" << catalog_height << "\n";
#endif
  
  // Check whether the GPU supports non-power-of-two textures
  bool npot = false;
  if (GLEW_ARB_texture_rectangle && GLEW_ARB_texture_non_power_of_two)
    npot=true;
  
  if (!npot) {
    // Use a power-of-two texture catalog
    int newx = 1, newy = 1;
    while (newx < catalog_width) newx *= 2;
    while (newy < catalog_height) newy *= 2;
    catalog_width = newx;
    catalog_height = newy;
    std::cout << "GPU does not support non-power-of-two textures, using " << catalog_width << "x" << catalog_height << " catalog.\n";
  }
  // Check whether the GPU will allow a texture of that size
  if (!testTextureSize(gl_catalog, catalog_width, catalog_height)) {
    MessageBox(NULL,"GPU unable to accommodate texture catalog. Retry without graphical tiles, update your drivers, or better yet update your GPU.", "GL error", MB_OK);
    exit(EXIT_FAILURE);
  }

  // Guess it will. Well, then, actually upload it
  glBindTexture(GL_TEXTURE_2D, gl_catalog);
      printGLError();
  char *zeroes = new char[catalog_width*catalog_height*4];
  memset(zeroes,0,sizeof(char)*catalog_width*catalog_height*4);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, catalog_width, catalog_height, 0, GL_RGBA,
	       GL_UNSIGNED_BYTE, zeroes);
  delete[] zeroes;
      printGLError();
  glBindTexture(GL_TEXTURE_2D, gl_catalog);
      printGLError();
  GLint param = (init.window.flag.has_flag(INIT_WINDOW_FLAG_TEXTURE_LINEAR) ?
    GL_LINEAR : GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,param);
       printGLError();
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,param);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
      printGLError();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
      printGLError();
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
      printGLError();
  // Performance isn't important here. Let's make sure there are no alignment issues.
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
      printGLError();
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      printGLError();
  // While storing the positions to gl_texpos.
  if (gl_texpos) delete[] gl_texpos;
  gl_texpos = new struct gl_texpos[raws.size()];
  for (int pos = 0; pos < ordered.size(); pos++) {
    long raws_pos = ordered[pos].pos;
    SDL_Surface *s = ordered[pos].s;
    SDL_PixelFormat *f = s->format;
    SDL_LockSurface(s);
    // Make /real/ sure we get the GL format right.
    unsigned char *pixels = new unsigned char[ordered[pos].w * ordered[pos].h * 4];
    // Recall, ordered[pos].w is 2 larger than s->w because of the border.
    for (int bx=0; bx < ordered[pos].w; bx++) {
      int x = bx - 1;
      if (x == -1) x++;
      if (x == s->w) x--;
      for (int by=0; by < ordered[pos].h; by++) {
        int y = by - 1;
        if (y == -1) y++;
        if (y == s->h) y--;
        // GL textures are loaded upside-down, Y=0 at the bottom
        unsigned char *pixel_dst = &pixels[(ordered[pos].h - by - 1)*ordered[pos].w*4 + bx*4];
        unsigned char *pixel_src = &((unsigned char*)s->pixels)[y*s->w*4 + x*4];
        assert (pixel_dst < pixels + ordered[pos].w * ordered[pos].h * 4);
        assert (pixel_src < (unsigned char*)s->pixels + s->w * s->h * 4);
        // We convert all textures to RGBA format at load-time, further below
        for (int i=0; i<4; i++) {
          pixel_dst[i] = pixel_src[i];
        }
      }
    }
    // Right. Upload the texture to the catalog.
    SDL_UnlockSurface(s);
    glBindTexture(GL_TEXTURE_2D, gl_catalog);
    printGLError();
    glTexSubImage2D(GL_TEXTURE_2D, 0, ordered[pos].x, ordered[pos].y, ordered[pos].w, ordered[pos].h,
		    GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    delete[] pixels;
    printGLError();
    // Compute texture coordinates and store to gl_texpos for later output.
    // To make sure the right pixel is chosen when texturing, we must
    // pick coordinates that place us in the middle of the pixel we want.
    //
    // There's no real reason to use double instead of floats here, but
    // no reason not to either, and it just might help with precision.
    //
    // There's a one-pixel border around each tile, so we offset by 1
    gl_texpos[raws_pos].left   = ((double)ordered[pos].x+1)      / (double)catalog_width;
    gl_texpos[raws_pos].right  = ((double)ordered[pos].x+1+s->w) / (double)catalog_width;
    gl_texpos[raws_pos].top    = ((double)ordered[pos].y+1)      / (double)catalog_height;
    gl_texpos[raws_pos].bottom = ((double)ordered[pos].y+1+s->h) / (double)catalog_height;
  }
  // And that's that. Locked, loaded and ready for texturing.
  printGLError();
  uploaded=true;
}

void textures::remove_uploaded_textures() {
  if (!uploaded) return; // Nothing to do
  glDeleteTextures(1, &gl_catalog);
  uploaded=false;
}

SDL_Surface *textures::get_texture_data(long pos) {
  if (raws.size() > pos) {
    return raws[pos];
  } else {
    std::cerr << "Asked for nonexistent texture data\n";
    SDL_Surface *surf = SDL_CreateRGBSurface(SDL_SWSURFACE, 8, 8, 32, 0, 0, 0, 0);
    SDL_FillRect(surf, NULL, SDL_MapRGB(surf->format, 255, 0, 255));
    raws.resize(pos+1);
    raws[pos] = surf;
    return raws[pos];
  }
}

long textures::clone_texture(long src) {
  long tx;
	
  if (raws.size() > src && raws[src]) {
    SDL_Surface *dst = SDL_ConvertSurface(raws[src], raws[src]->format, SDL_SWSURFACE);
    tx=add_texture(dst);
  }
  else {
  // Okay, we've been asked to clone a nonexistent texture. Riight...
  // Fortunately add_texture doesn't look at the pointer it gets at all.
  std::cerr << "Asked to clone nonexistent texture!\n";
  tx=add_texture(NULL);
  }

  enabler.reset_textures();

  return tx;
}

void textures::grayscale_texture(long pos) {
 SDL_Surface *s = get_texture_data(pos);
 if (!s) return;
 SDL_LockSurface(s);
 SDL_PixelFormat *f = s->format;
 Uint32 *pixels = (Uint32*)(s->pixels);
 if (f->BytesPerPixel != 4) {
   std::cerr << "grayscale_texture ran into mysteriously uncanonicalized texture\n";
   goto cleanup;
 }
 for (int i=0; i < s->w*s->h; i++) { // For each pixel
   int r = (pixels[i] & f->Rmask) >> f->Rshift;
   int g = (pixels[i] & f->Gmask) >> f->Gshift;
   int b = (pixels[i] & f->Bmask) >> f->Bshift;
   int alpha = (pixels[i] & f->Amask) >> f->Ashift;
int luminosity=(int)((float)r*0.30f+(float)g*0.59f+(float)b*0.11f);
if(luminosity<0)luminosity=0;
if(luminosity>255)luminosity=255;
   pixels[i] = (luminosity << f->Rshift) |
     (luminosity << f->Gshift) |
     (luminosity << f->Bshift) |
     (alpha << f->Ashift);
 }

 cleanup:
 SDL_UnlockSurface(s);

 enabler.reset_textures();
}

// Converts an arbitrary Surface to something like the display format
// (32-bit RGBA), and converts magenta to transparency if convert_magenta is set
// and the source surface didn't already have an alpha channel.
// It also deletes the source surface.
//
// It uses the same pixel format (RGBA, R at lowest address) regardless of
// hardware.
SDL_Surface *canonicalize_format(SDL_Surface *src, bool convert_magenta) {
  SDL_PixelFormat fmt;
  fmt.palette = NULL;
  fmt.BitsPerPixel = 32;
  fmt.BytesPerPixel = 4;
  fmt.Rloss = fmt.Gloss = fmt.Bloss = fmt.Aloss = 0;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
  fmt.Rshift = 24; fmt.Gshift = 16; fmt.Bshift = 8; fmt.Ashift = 0;
#else
  fmt.Rshift = 0; fmt.Gshift = 8; fmt.Bshift = 16; fmt.Ashift = 24;
#endif
  fmt.Rmask = 255 << fmt.Rshift;
  fmt.Gmask = 255 << fmt.Gshift;
  fmt.Bmask = 255 << fmt.Bshift;
  fmt.Amask = 255 << fmt.Ashift;
  fmt.colorkey = 0;
  fmt.alpha = 255;

  if (src->format->Amask == 0 && convert_magenta) { // No alpha
    SDL_SetColorKey(src, SDL_SRCCOLORKEY,
		    SDL_MapRGB(src->format, 255, 0, 255));
  }
  SDL_Surface *tgt = SDL_ConvertSurface(src, &fmt, SDL_SWSURFACE);
  SDL_FreeSurface(src);
  return tgt;
}

// Finds or creates a free spot in the texture array, and inserts
// surface in that spot, then returns the location.
long textures::add_texture(SDL_Surface *surface) {
  long sz = raws.size();
  // Look for a free spot
  for (long pos=0; pos < sz; pos++) {
    if (raws[pos] == NULL) {
      raws[pos] = surface;
      return pos;
    }
  }

  // No free spot, make one
  raws.push_back(surface);
  return sz;
}

void textures::load_multi_pdim(const string &filename, long *tex_pos, long dimx,
			       long dimy, bool convert_magenta,
			       long *disp_x, long *disp_y) {
  SDL_Surface *raw = IMG_Load(filename.c_str());
  if (!raw) {
    MessageBox(NULL, ("Not found: " + filename).c_str(), "Tileset not found", MB_OK);
    exit(1);
  }
  SDL_Surface *src = canonicalize_format(raw, convert_magenta);
  SDL_SetAlpha(src, 0, 255);
  *disp_x = src->w / dimx;
  *disp_y = src->h / dimy;
  long idx = 0;
  for (int y=0; y < dimy; y++) {
    for (int x=0; x < dimx; x++) {
      SDL_Surface *tile = SDL_CreateRGBSurface(SDL_SWSURFACE, *disp_x, *disp_y,
					       32, src->format->Rmask,
					       src->format->Gmask,
					       src->format->Bmask,
					       src->format->Amask);
      SDL_SetAlpha(tile, 0,255);
      SDL_Rect pos_src;
      pos_src.x = *disp_x * x;
      pos_src.y = *disp_y * y;
      pos_src.w =  *disp_x;
      pos_src.h =  *disp_y;
      SDL_BlitSurface(src, &pos_src, tile, NULL);
      tex_pos[idx++] = add_texture(tile);
    }
  }
  SDL_FreeSurface(src);
  // Re-upload textures if necessary
  enabler.reset_textures();
}

long textures::load(const string &filename, bool convert_magenta) {
  SDL_Surface *raw = IMG_Load(filename.c_str());
  if (!raw) {
    MessageBox(NULL, ("Not found: " + filename).c_str(), "Image not found", MB_OK);
    exit(1);
  }
  SDL_Surface *tex = canonicalize_format(raw, convert_magenta);
  long pos = add_texture(tex);
  // Re-upload if necessary
  enabler.reset_textures();
  return pos;
}

void textures::delete_texture(long pos) {
  // We can't actually resize the array, as
  // (a) it'd be slow, and
  // (b) it'd change all the positions. Bad stuff would happen.
  if (raws[pos]) {
    SDL_FreeSurface(raws[pos]);
    raws[pos] = NULL;
  }
}
