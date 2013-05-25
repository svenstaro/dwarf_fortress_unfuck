#include "ttf_manager.hpp"
#include "init.h"
#include <iostream>

using namespace std;

ttf_managerst ttf_manager;

bool ttf_managerst::init(int ceiling, int tile_width) {
  // Reset to a known state, clear everything
  if ((!TTF_WasInit()) && (TTF_Init() == -1)) {
    MessageBox(NULL, TTF_GetError(), "TTF error", MB_OK);
    return false;
  }
  if (font) TTF_CloseFont(font);
  handles.clear();
  for (auto it = textures.cbegin(); it != textures.cend(); ++it)
    SDL_FreeSurface(it->second);
  textures.clear();
  this->tile_width = tile_width;
  this->ceiling = ceiling;
  // Try progressively smaller point sizes until we find one that fits
  for (int sz=20; sz > 0; sz--) {
    font = TTF_OpenFont("data/art/font.ttf", sz);
    if (!font) continue;
    if (TTF_FontHeight(font) <= ceiling) {
#ifdef DEBUG
      cout << "Picked font at " << sz << " points for ceiling " << ceiling << endl;
      // get the glyph metric for the letter 'M' in a loaded font
      cout << "TTF_FontHeight " << TTF_FontHeight(font) << endl;
      cout << "TTF_FontAscent " << TTF_FontAscent(font) << endl;
      cout << "TTF_FontDescent " << TTF_FontDescent(font) << endl;
      cout << "TTF_FontLineSkip " << TTF_FontLineSkip(font) << endl;
#endif
      int minx,maxx,miny,maxy,advance;
      if (TTF_GlyphMetrics(font, 'M', &minx, &maxx, &miny, &maxy, &advance) == -1)
        puts(TTF_GetError());
      else {
        em_width = maxx;
#ifdef DEBUG
        printf("minx    : %d\n",minx);
        printf("maxx    : %d\n",maxx);
        printf("miny    : %d\n",miny);
        printf("maxy    : %d\n",maxy);
        printf("advance : %d\n",advance);
#endif
      }
      return true;
    }
    TTF_CloseFont(font);
  }
  // ..fine.
  cout << "No font found!" << endl;
  font = NULL;
  return false;
}

static void cp437_to_unicode(const string &text, vector<Uint16> &unicode) {
  unicode.resize(text.length() + 1);
  int i;
  for (i=0; i < text.size(); i++) {
    const int cp437 = (unsigned char)text[i];
    unicode[i] = charmap[cp437];
  }
  unicode[i] = 0;
}


int ttf_managerst::size_text(const string &text) {
  vector<Uint16> text_unicode;
  cp437_to_unicode(text, text_unicode);
  int width, height;
  TTF_SizeUNICODE(font, &text_unicode[0], &width, &height);
  return (width + tile_width - 1) / tile_width;
}


ttf_details ttf_managerst::get_handle(const list<ttf_id> &text, justification just) {
  // Check for an existing handle
  handleid id = {text, just};
  auto it = handles.find(id);
  if (it != handles.end()) return it->second;
  // Right. Make a new one.
  int handle = ++max_handle;
  // Split out any tabs
  list<ttf_id> split_text;
  for (auto it = text.cbegin(); it != text.cend(); ++it) {
    int pos = 0;
    int tabpos;
    while ((tabpos = it->text.find("\t", pos)) != string::npos) {
      ttf_id left;
      left.fg = it->fg; left.bg = it->bg; left.bold = it->bold;
      left.text = it->text.substr(pos, tabpos - pos);
      split_text.push_back(left);
      ttf_id tabber;
      tabber.fg = tabber.bg = tabber.bold = 255;
      split_text.push_back(tabber);
      pos = tabpos + 1;
    }
    ttf_id right;
    right.fg = it->fg; right.bg = it->bg; right.bold = it->bold;
    right.text = it->text.substr(pos);
    split_text.push_back(right);
  }
  // Find the total width of the text
  vector<Uint16> text_unicode;
  int ttf_width = 0, ttf_height = 0, text_width = 0;
  for (auto it = split_text.cbegin(); it != split_text.cend(); ++it) {
    if (it->fg == 255 && it->bg == 255 && it->bold == 255) {
      // Tab stop
      int tabstop = tab_width * em_width;
      int tab_width = tabstop - ((ttf_width - 1) % tabstop) + 1;
      ttf_width += tab_width;
      text_width += 1;
    } else {
      cp437_to_unicode(it->text, text_unicode);
      int slice_width, slice_height;
      TTF_SizeUNICODE(font, &text_unicode[0], &slice_width, &slice_height);
      ttf_width += slice_width;
      text_width += it->text.size();
    }
  }
  ttf_height = ceiling;
  // Compute geometry
  double grid_width = double(ttf_width) / tile_width;
  double offset = just == justify_right ? text_width - grid_width :
    just == justify_center ? (text_width - grid_width) / 2 :
    0;
  if (just == justify_center && text_width % 2)
    offset += 0.5; // Arbitrary fixup for approximate grid centering
  double fraction, integral;
  fraction = modf(offset, &integral);
  // Outputs:
  const int grid_offset = int(integral + 0.001); // Tiles to move to the right in addst
  const int pixel_offset = int(fraction * tile_width); // Black columns to add to the left of the image
  // const int full_grid_width = int(ceil(double(ttf_width) / double(tile_width) + fraction) + 0.1); // Total width of the image in grid units
  const int full_grid_width = text_width;
  const int pixel_width = full_grid_width * tile_width; // And pixels
  assert(pixel_width >= ttf_width);
  // Store for later
  ttf_details ret; ret.handle = handle; ret.offset = grid_offset; ret.width = full_grid_width;
  handles[id] = ret;
  // We do the actual rendering in the render thread, later on.
  todo.push_back(todum(handle, split_text, ttf_height, pixel_offset, pixel_width));
  return ret;
}

SDL_Surface *ttf_managerst::get_texture(int handle) {
  // Run any outstanding renders
  if (!todo.empty()) {
    vector<Uint16> text_unicode;
    for (auto it = todo.cbegin(); it != todo.cend(); ++it) {
      const int height = it->height;
      SDL_Surface *textimg = SDL_CreateRGBSurface(SDL_SWSURFACE, it->pixel_width, height, 32, 0, 0, 0, 0);
// #ifdef DEBUG
//       SDL_FillRect(textimg, NULL, SDL_MapRGBA(textimg->format, 255, 0, 0, 255));
// #endif
      // Render each of the text segments
      int idx = 0;
      int xpos = it->pixel_offset;
      for (auto seg = it->text.cbegin(); seg != it->text.cend();) {
        const ttf_id &text = *seg;
        ++seg;
        ++idx;
        if (text.fg == 255 && text.bg == 255 && text.bold == 255) {
          // Skip to tab stop
          int tabstop = tab_width * em_width;
          int tab_width = tabstop - ((xpos - 1) % tabstop) + 1;
          xpos += tab_width;
          continue;
        }
        if (text.text.size() <= 0)
          continue;
        cp437_to_unicode(text.text, text_unicode);
        const int fg = (text.fg + text.bold * 8) % 16;
        SDL_Color fgc = {Uint8(enabler.ccolor[fg][0]*255),
                         Uint8(enabler.ccolor[fg][1]*255),
                         Uint8(enabler.ccolor[fg][2]*255)};
        const int bg = text.bg % 16;
        Uint32 bgc = SDL_MapRGB(textimg->format,
                                Uint8(enabler.ccolor[bg][0]*255),
                                Uint8(enabler.ccolor[bg][1]*255),
                                Uint8(enabler.ccolor[bg][2]*255));
#ifdef DEBUG
        // SDL_Color white = {255,255,255};
        // Uint32 red = SDL_MapRGB(textimg->format, 255,0,0);
        // fgc = white;
        // bgc = red;
#endif
        if (idx == 0) {
          // Fill in the left side
          SDL_Rect left = {0, 0, Sint16(xpos), Sint16(height)};
          SDL_FillRect(textimg, &left, bgc);
        } else if (seg == it->text.cend()) {
          // Fill in the right side
          SDL_Rect right = {Sint16(xpos), 0, Sint16(it->pixel_width), Sint16(height)};
          SDL_FillRect(textimg, &right, bgc);
        }
        // Render the TTF segment
        SDL_Surface *textimg_seg = TTF_RenderUNICODE_Blended(font, &text_unicode[0], fgc);
        // Fill the background color of this part of the textimg
        SDL_Rect dest = {Sint16(xpos), 0, Sint16(textimg_seg->w), Sint16(height)};
        SDL_FillRect(textimg, &dest,
                     SDL_MapRGB(textimg->format,
                                // Uint8(255),
                                // Uint8(255),
                                // Uint8(255)));
                                Uint8(enabler.ccolor[bg][0]*255),
                                Uint8(enabler.ccolor[bg][1]*255),
                                Uint8(enabler.ccolor[bg][2]*255)));
        // And copy the TTF segment over.
        SDL_Rect dest2 = {Sint16(xpos), 0, 0, 0};
        SDL_BlitSurface(textimg_seg, NULL, textimg, &dest2);
        // Ready for next segment.
        xpos += textimg_seg->w;
        SDL_FreeSurface(textimg_seg);
      }
      // ..and make the whole thing display format. Phew!
      SDL_Surface *textimg_2 = SDL_DisplayFormat(textimg);
#ifdef DEBUG
      // cout << "Rendering \"" << text.text << "\" at height " << box2->h << endl;
      // cout << " width " << textimg->w << " in box of " << box->w << endl;
#endif
      SDL_FreeSurface(textimg);
      // Store it for later.
      textures[it->handle] = textimg_2;
    }
    todo.clear();
  }
  // Find the li'l texture
  SDL_Surface *tex = textures[handle];
  if (!tex) {
    cout << "Missing/broken TTF handle: " << handle << endl;
  }
  return tex;
}

void ttf_managerst::gc() {
  // Just delete everything, for now.
  for (auto it = textures.begin(); it != textures.end(); ++it)
    SDL_FreeSurface(it->second);
  textures.clear();
  handles.clear();
  todo.clear();
}
