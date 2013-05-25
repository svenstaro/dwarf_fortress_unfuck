#ifndef TTF_MANAGER_HPP
#define TTF_MANAGER_HPP

#include "init.h"
#include "enabler.h"
#ifdef __APPLE__
#include <SDL_ttf/SDL_ttf.h>
#else
#include <SDL/SDL_ttf.h>
#endif
#include <unordered_map>
#include <list>

using std::unordered_map;
using std::list;

struct handleid {
  list<ttf_id> text;
  justification just;

  bool operator< (const handleid &other) const {
    if (text != other.text) return text < other.text;
    return just < other.just;
  }

  bool operator== (const handleid &other) const {
    return just == other.just && text == other.text;
  }
};

namespace std {
  template<> struct hash<struct handleid> {
    size_t operator()(handleid val) const {
      size_t h = 0;
      auto end = val.text.cend();
      for (auto it = val.text.cbegin(); it != end; ++it) {
        h += hash<ttf_id>()(*it);
      }
      return h + val.just;
    }
  };
};

struct ttf_details {
  int handle;
  int offset;
  int width;
};

class ttf_managerst {
  TTF_Font *font;
  int max_handle;
  int tile_width, ceiling;
  double tab_width;
  int em_width;
  unordered_map<handleid, ttf_details> handles;
  unordered_map<int, SDL_Surface*> textures;
  struct todum {
    int handle;
    list<ttf_id> text;
    int height;
    int pixel_offset, pixel_width;
    todum(int handle, const list<ttf_id> &t, int h, int po, int pw) :
      handle(handle), text(t), height(h), pixel_offset(po), pixel_width(pw) {}
  };
  list<todum> todo;
public:
  ttf_managerst() {
    font = NULL;
    max_handle = 1;
    tab_width = 2;
    em_width = 8;
  }
  ~ttf_managerst() {
	for (auto it = textures.cbegin(); it != textures.cend(); ++it)
		SDL_FreeSurface(it->second);
    if (font) TTF_CloseFont(font);
  }
  bool init(int ceiling, int tile_width);
  bool was_init() const { return font != NULL; }
  // Return the expected size of a bit of text, in tiles.
  int size_text(const string &text);
  ttf_details get_handle(const list<ttf_id> &text, justification just);
  // Returns rendered text. Renders too, if necessary.
  // The returned SDL_Surface is owned by the ttf_managerst.
  SDL_Surface *get_texture(int handle);
  // Garbage-collect ttf surfaces
  void gc();
  // Set tab-stop width (in ems, i.e. tile widths)
  void set_tab_width(double width) { tab_width = width; }
  // Check if TTF is currently active
  bool ttf_active() const {
    return was_init() &&
      (::init.font.use_ttf == ttf_on ||
       (::init.font.use_ttf == ttf_auto && ::init.font.ttf_limit <= ceiling));
  }
};

extern ttf_managerst ttf_manager;

#endif
