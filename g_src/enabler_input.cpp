#include <map>
#include <vector>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <stdlib.h>
#include <math.h>
using namespace std;

#include <SDL/SDL.h>

#include "enabler_input.h"
#include "init.h"
extern initst init;
#include "platform.h"
#include "files.h"
#include "find_files.h"
#include "svector.h"
#include "curses.h"

// The timeline events we actually pass back from get_input. Well, no,
// that's just k, but..
struct Event {
  Repeat r;
  InterfaceKey k;
  int repeats;  // Starts at 0, increments once per repeat
  int serial;
  int time;
  int tick;  // The sim-tick at which we last returned this event
  bool macro;  // Created as part of macro playback.

  bool operator== (const Event &other) const {
    if (r != other.r) return false;
    if (k != other.k) return false;
    if (repeats != other.repeats) return false;
    if (serial != other.serial) return false;
    if (time != other.time) return false;
    if (macro != other.macro) return false;
    return true;
  }

  // We sort by time first, and then serial number.
  // The order of the other bits is unimportant.
  bool operator< (const Event &o) const {
    if (time != o.time) return time < o.time;
    if (serial != o.serial) return serial < o.serial;
    if (r != o.r) return r < o.r;
    if (k != o.k) return k < o.k;
    if (repeats != o.repeats) return repeats < o.repeats;
    if (macro != o.macro) return macro < o.macro;
    return false;
  }
};

// Used to decide which key-binding to display. As a heuristic, we
// prefer whichever display string is shortest.
struct less_sz {
  bool operator() (const string &a, const string &b) const {
    if (a.size() < b.size()) return true;
    if (a.size() > b.size()) return false;
    return a < b;
  }
};

// These change dynamically in the normal process of DF
static int last_serial = 0; // Input serial number, to differentiate distinct physical presses
static set<Event> timeline; // A timeline of pending key events (for next get_input)
static set<EventMatch> pressed_keys; // Keys we consider "pressed"
static int modState; // Modifier state
  
// These do not change as part of the normal dynamics of DF, only at startup/when editing.
static multimap<EventMatch,InterfaceKey> keymap;
static map<InterfaceKey,Repeat> repeatmap;
static map<InterfaceKey,set<string,less_sz> > keydisplay; // Used only for display, not for meaning

// Macro recording
static bool macro_recording = false;
static macro active_macro; // Active macro
static map<string,macro> macros;
static Time macro_end = 0; // Time at which the currently playing macro will end

// Prefix command state
static bool in_prefix_command = false;
static string prefix_command;

// Keybinding editing
static bool key_registering = false;
static list<EventMatch> stored_keys;

// Interface-file last loaded
static string interfacefile;


// Returns an unused serial number
static Time next_serial() {
  return ++last_serial;
}

static void update_keydisplay(InterfaceKey binding, string display) {
  // Need to filter out space/tab, for obvious reasons.
  if (display == " ") display = "Space";
  if (display == "\t") display = "Tab";
  map<InterfaceKey,set<string,less_sz> >::iterator it = keydisplay.find(binding);
  if (it == keydisplay.end()) {
    set<string,less_sz> s; s.insert(display);
    keydisplay[binding] = s;
  } else {
    keydisplay[binding].insert(display);
  }
}

static void assertgood(ifstream &s) {
  if (s.eof())
    MessageBox(NULL, "EOF while parsing keyboard bindings", 0, 0);
  else if (!s.good())
    MessageBox(NULL, "I/O error while parsing keyboard bindings", 0, 0);
  else
    return;
  abort();
}

// Decodes an UTF-8 encoded string into a /single/ UTF-8 character,
// discarding any overflow. Returns 0 on parse error.
int decode_utf8(const string &s) {
  int unicode = 0, length, i;
  if (s.length() == 0) return 0;
  length = decode_utf8_predict_length(s[0]);
  switch (length) {
  case 1: unicode = s[0]; break;
  case 2: unicode = s[0] & 0x1f; break;
  case 3: unicode = s[0] & 0x0f; break;
  case 4: unicode = s[0] & 0x07; break;
  default: return 0;
  }

  // Concatenate the follow-up bytes
  if (s.length() < length) return 0;
  for (i = 1; i < length; i++) {
    if ((s[i] & 0xc0) != 0x80) return 0;
    unicode = (unicode << 6) | (s[i] & 0x3f);
  }
  return unicode;
}

// Returns the length of an utf-8 sequence, based on its first byte
int decode_utf8_predict_length(char byte) {
  if ((byte & 0x80) == 0) return 1;
  if ((byte & 0xe0) == 0xc0) return 2;
  if ((byte & 0xf0) == 0xe0) return 3;
  if ((byte & 0xf8) == 0xf0) return 4;
  return 0; // Invalid start byte
}

// Encode an arbitrary unicode value as a string. Returns an empty
// string if the value is out of range.
string encode_utf8(int unicode) {
  string s;
  int i;
  if (unicode < 0 || unicode > 0x10ffff) return ""; // Out of range for utf-8
  else if (unicode <= 0x007f) { // 1-byte utf-8
    s.resize(1, 0);
  }
  else if (unicode <= 0x07ff) { // 2-byte utf-8
    s.resize(2, 0);
    s[0] = 0xc0;
  }
  else if (unicode <= 0xffff) { // 3-byte utf-8
    s.resize(3, 0);
    s[0] = 0xe0;
  }
  else { // 4-byte utf-8
    s.resize(4, 0);
    s[0] = 0xf0;
  }

  // Build up the string, right to left
  for (i = (int)s.length()-1; i > 0; i--) {
    s[i] = 0x80 | (unicode & 0x3f);
    unicode >>= 6;
  }
  // Finally, what's left goes in the low bits of s[0]
  s[0] |= unicode;
  return s;
}

string translate_mod(Uint8 mod) {
  string ret;
  if (mod & 1) ret += "Shift+";
  if (mod & 2) ret += "Ctrl+";
  if (mod & 4) ret += "Alt+";
  return ret;
}

static string display(const EventMatch &match) {
  ostringstream ret;
  ret << translate_mod(match.mod);
  switch (match.type) {
  case type_unicode: ret << (char)match.unicode; break;
  case type_key: {
    map<SDLKey,string>::iterator it = sdlNames.left.find(match.key);
    if (it != sdlNames.left.end())
      ret << it->second;
    else
      ret << "SDL+" << (int)match.key;
    break;
  }
  case type_button:
    ret << "Button " << (int)match.button;
    break;
  }
  return ret.str();
}

static string translate_repeat(Repeat r) {
  switch (r) {
  case REPEAT_NOT: return "REPEAT_NOT";
  case REPEAT_SLOW: return "REPEAT_SLOW";
  case REPEAT_FAST: return "REPEAT_FAST";
  default: return "REPEAT_BROKEN";
  }
}

// Update the modstate, since SDL_getModState doesn't /work/ for alt
static void update_modstate(const SDL_Event &e) {
  if (e.type == SDL_KEYUP) {
    switch (e.key.keysym.sym) {
    case SDLK_RSHIFT:
    case SDLK_LSHIFT:
      modState &= ~1;
      break;
    case SDLK_RCTRL:
    case SDLK_LCTRL:
      modState &= ~2;
      break;
    case SDLK_RALT:
    case SDLK_LALT:
      modState &= ~4;
      break;
    }
  } else if (e.type == SDL_KEYDOWN) {
    switch (e.key.keysym.sym) {
    case SDLK_RSHIFT:
    case SDLK_LSHIFT:
      modState |= 1;
      break;
    case SDLK_RCTRL:
    case SDLK_LCTRL:
      modState |= 2;
      break;
    case SDLK_RALT:
    case SDLK_LALT:
      modState |= 4;
      break;
    }
  }
}

// Converts SDL mod states to ours, collapsing left/right shift/alt/ctrl
Uint8 getModState() {
  return modState;
}

// Not sure what to call this, but it ain't using regexes.
static bool parse_line(const string &line, const string &regex, vector<string> &parts) {
  parts.clear();
  parts.push_back(line);
  int bytes;
  for (int l = 0, r = 0; r < regex.length();) {
    switch (regex[r]) {
    case '*': // Read until ], : or the end of the line, but at least one character.
      {
        const int start = l;
        for (; l < line.length() && (l == start || (line[l] != ']' && line[l] != ':')); l++)
          ;
        parts.push_back(line.substr(start, l - start));
        r++;
      }
      break;
    default:
      if (line[l] != regex[r]) return false;
      r++; l++;
      break;
    }
  }
  // We've made it this far, clearly the string parsed
  return true;
}

void enabler_inputst::load_keybindings(const string &file) {
  cout << "Loading bindings from " << file << endl;
  interfacefile = file;
  ifstream s(file.c_str());
  if (!s.good()) {
    MessageBox(NULL, (file + " not found, or I/O error encountered").c_str(), 0, 0);
    abort();
  }

  list<string> lines;
  while (s.good()) {
    string line;
    getline(s, line);
    lines.push_back(line);
  }

  static const string bind("[BIND:*:*]");
  static const string sym("[SYM:*:*]");
  static const string key("[KEY:*]");
  static const string button("[BUTTON:*:*]");

  list<string>::iterator line = lines.begin();
  vector<string> match;

  while (line != lines.end()) {
    if (parse_line(*line, bind, match)) {
      map<string,InterfaceKey>::iterator it = bindingNames.right.find(match[1]);
      if (it != bindingNames.right.end()) {
        InterfaceKey binding = it->second;
        // Parse repeat data
        if (match[2] == "REPEAT_FAST")
          repeatmap[(InterfaceKey)binding] = REPEAT_FAST;
        else if (match[2] == "REPEAT_SLOW")
          repeatmap[(InterfaceKey)binding] = REPEAT_SLOW;
        else if (match[2] == "REPEAT_NOT")
          repeatmap[(InterfaceKey)binding] = REPEAT_NOT;
        else {
          repeatmap[(InterfaceKey)binding] = REPEAT_NOT;
          cout << "Broken repeat request: " << match[2] << endl;
        }
        ++line;
        // Add symbols/keys/buttons
        while (line != lines.end()) {
          EventMatch matcher;
          // SDL Keys
          if (parse_line(*line, sym, match)) {
            map<string,SDLKey>::iterator it = sdlNames.right.find(match[2]);
            if (it != sdlNames.right.end()) {
              matcher.mod  = atoi(string(match[1]).c_str());
              matcher.type = type_key;
              matcher.key  = it->second;
              keymap.insert(pair<EventMatch,InterfaceKey>(matcher, (InterfaceKey)binding));
              update_keydisplay(binding, display(matcher));
            } else {
              cout << "Unknown SDLKey: " << match[2] << endl;
            }
            ++line;           
          } // Unicode
          else if (parse_line(*line, key, match)) {
            matcher.type = type_unicode;
            matcher.unicode = decode_utf8(match[1]);
            matcher.mod = KMOD_NONE;
            if (matcher.unicode) {
              keymap.insert(make_pair(matcher, (InterfaceKey)binding));
              if (matcher.unicode < 256) {
                // This unicode key is part of the latin-1 mapped portion of unicode, so we can
                // actually display it. Nice.
                char c[2] = {char(matcher.unicode), 0};
                update_keydisplay(binding, display(matcher));
              }
            } else {
              cout << "Broken unicode: " << *line << endl;
            }
            ++line;
          } // Mouse buttons
          else if (parse_line(*line, button, match)) {
            matcher.type = type_button;
            string str = match[2];
            matcher.button = atoi(str.c_str());
            if (matcher.button) {
              matcher.mod  = atoi(string(match[1]).c_str());
              keymap.insert(pair<EventMatch,InterfaceKey>(matcher, (InterfaceKey)binding));
              update_keydisplay(binding, display(matcher));
            } else {
              cout << "Broken button (should be [BUTTON:#:#]): " << *line << endl;
            }
            ++line;
          } else {
            break;
          }
        }
      } else {
        cout << "Unknown binding: " << match[1] << endl;
		++line;
      }
    } else {
      // Retry with next line
      ++line;
    }
  }
}

void enabler_inputst::save_keybindings(const string &file) {
  cout << "Saving bindings to " << file << endl;
  string temporary = file + ".partial";
  ofstream s(temporary.c_str()); 
  multimap<InterfaceKey,EventMatch> map;
  InterfaceKey last_key = INTERFACEKEY_NONE;

  if (!s.good()) {
    string t = "Failed to open " + temporary + " for writing";
    MessageBox(NULL, t.c_str(), 0, 0);
    s.close();
    return;
  }
  // Invert keyboard map
  for (multimap<EventMatch,InterfaceKey>::iterator it = keymap.begin(); it != keymap.end(); ++it)
    map.insert(pair<InterfaceKey,EventMatch>(it->second,it->first));
  // Insert an empty line for the benefit of note/wordpad
  s << endl;
  // And write.
  for (multimap<InterfaceKey,EventMatch>::iterator it = map.begin(); it != map.end(); ++it) {
    if (!s.good()) {
      MessageBox(NULL, "I/O error while writing keyboard mapping", 0, 0);
      s.close();
      return;
    }
    if (it->first != last_key) {
      last_key = it->first;
      s << "[BIND:" << bindingNames.left[it->first] << ":"
        << translate_repeat(repeatmap[it->first]) << "]" << endl;
    }
    switch (it->second.type) {
    case type_unicode:
      s << "[KEY:" << encode_utf8(it->second.unicode) << "]" << endl;
      break;
    case type_key:
      s << "[SYM:" << (int)it->second.mod << ":" << sdlNames.left[it->second.key] << "]" << endl;
      break;
    case type_button:
      s << "[BUTTON:" << (int)it->second.mod << ":" << (int)it->second.button << "]" << endl;
      break;
    }
  }
  s.close();
  replace_file(temporary, file);
}

void enabler_inputst::save_keybindings() {
  save_keybindings(interfacefile);
}

void enabler_inputst::add_input(SDL_Event &e, Uint32 now) {
  // Before we can use this input, there are some issues to deal with:
  // - SDL provides unicode translations only for key-press events, not
  //   releases. We need to keep track of pressed keys, and generate
  //   unicode release events whenever any modifiers are hit, or if
  //   that raw keycode is released.
  // - Generally speaking, when modifiers are hit/released, we discard those
  //   events and generate press/release events for all pressed non-modifiers.
  // - It's possible for multiple events to be generated on the same tick.
  //   These are of course separate keypresses, and must be kept separate.
  //   That's what the serial is for.

  set<EventMatch>::iterator pkit;
  list<pair<KeyEvent, int> > synthetics;
  update_modstate(e);
  
  // Convert modifier state changes
  if ((e.type == SDL_KEYUP || e.type == SDL_KEYDOWN) &&
      (e.key.keysym.sym == SDLK_RSHIFT ||
       e.key.keysym.sym == SDLK_LSHIFT ||
       e.key.keysym.sym == SDLK_RCTRL  ||
       e.key.keysym.sym == SDLK_LCTRL  ||
       e.key.keysym.sym == SDLK_RALT   ||
       e.key.keysym.sym == SDLK_LALT   )) {
    for (pkit = pressed_keys.begin(); pkit != pressed_keys.end(); ++pkit) {
      // Release currently pressed keys
      KeyEvent synth;
      synth.release = true;
      synth.match = *pkit;
      synthetics.push_back(make_pair(synth, next_serial()));
      // Re-press them, with new modifiers, if they aren't unicode. We can't re-translate unicode.
      if (synth.match.type != type_unicode) {
        synth.release = false;
        synth.match.mod = getModState();
        if (!key_registering) // We don't want extras when registering keys
          synthetics.push_back(make_pair(synth, next_serial()));
      }
    }
  } else {
    // It's not a modifier. If this is a key release, then we still need
    // to find and release pressed unicode keys with this scancode
    if (e.type == SDL_KEYUP) {
      for (pkit = pressed_keys.begin(); pkit != pressed_keys.end(); ++pkit) {
        if (pkit->type == type_unicode && pkit->scancode == e.key.keysym.scancode) {
          KeyEvent synth;
          synth.release = true;
          synth.match = *pkit;
          synthetics.push_back(make_pair(synth, next_serial()));
        }
      }
    }
    // Since it's not a modifier, we also pass on symbolic/button
    // (always) and unicode (if defined) events
    //
    // However, since SDL ignores(?) ctrl and alt when translating to
    // unicode, we want to ignore unicode events if those are set.
    const int serial = next_serial();

    KeyEvent real;
    real.release = (e.type == SDL_KEYUP || e.type == SDL_MOUSEBUTTONUP) ? true : false;
    real.match.mod = getModState();
    if (e.type == SDL_MOUSEBUTTONUP || e.type == SDL_MOUSEBUTTONDOWN) {
      real.match.type = type_button;
      real.match.scancode = 0;
      real.match.button = e.button.button;
      synthetics.push_back(make_pair(real, serial));
    }
    if (e.type == SDL_KEYUP || e.type == SDL_KEYDOWN) {
      real.match.type = type_key;
      real.match.scancode = e.key.keysym.scancode;
      real.match.key = e.key.keysym.sym;
      synthetics.push_back(make_pair(real, serial));
    }
    if (e.type == SDL_KEYDOWN && e.key.keysym.unicode && getModState() < 2) {
      real.match.mod = KMOD_NONE;
      real.match.type = type_unicode;
      real.match.scancode = e.key.keysym.scancode;
      real.match.unicode = e.key.keysym.unicode;
      synthetics.push_back(make_pair(real, serial));
    }
    if (e.type == SDL_QUIT) {
      // This one, we insert directly into the timeline.
      Event e = {REPEAT_NOT, (InterfaceKey)INTERFACEKEY_OPTIONS, 0, next_serial(), now, 0};
      timeline.insert(e);
    }
  }

  list<pair<KeyEvent, int> >::iterator lit;
  for (lit = synthetics.begin(); lit != synthetics.end(); ++lit) {
    // Add or remove the key from pressed_keys, keeping that up to date
    if (lit->first.release) pressed_keys.erase(lit->first.match);
    else pressed_keys.insert(lit->first.match);
    // And pass the event on deeper.
    add_input_refined(lit->first, now, lit->second);
  }
}

// Input encoding:
// 1 and up are ncurses symbols, as returned by getch.
// -1 and down are unicode values.
// esc is true if this key was part of an escape sequence.
#ifdef CURSES
void enabler_inputst::add_input_ncurses(int key, Time now, bool esc) {
  // TODO: Deal with shifted arrow keys, etc. See man 5 terminfo and tgetent.
  
  EventMatch sdl, uni; // Each key may provoke an unicode event, an "SDL-key" event, or both
  const int serial = next_serial();
  sdl.type = type_key;
  uni.type = type_unicode;
  sdl.scancode = uni.scancode = 0; // We don't use this.. hang on, who does? ..nobody. FIXME!
  sdl.mod = uni.mod = 0;
  sdl.key = SDLK_UNKNOWN;
  uni.unicode = 0;

  if (esc) { // Escape sequence, meaning alt was held. I hope.
    sdl.mod = uni.mod = DFMOD_ALT;
  }

  if (key == -10) { // Return
    sdl.key = SDLK_RETURN;
    uni.unicode = '\n';
  } else if (key == -9) { // Tab
    sdl.key = SDLK_TAB;
    uni.unicode = '\t';
  } else if (key == -27) { // If we see esc here, it's the actual esc key. Hopefully.
    sdl.key = SDLK_ESCAPE;
  } else if (key == -127) { // Backspace/del
    sdl.key = SDLK_BACKSPACE;
  } else if (key < 0 && key >= -26) { // Control-a through z (but not ctrl-j, or ctrl-i)
    sdl.mod |= DFMOD_CTRL;
    sdl.key = (SDLKey)(SDLK_a + (-key) - 1);
  } else if (key <= -32 && key >= -126) { // ASCII character set
    uni.unicode = -key;
    sdl.key = (SDLKey)-key; // Most of this maps directly to SDL keys, except..
    if (sdl.key > 64 && sdl.key < 91) { // Uppercase
      sdl.key = (SDLKey)(sdl.key + 32); // Maps to lowercase, and
      sdl.mod |= DFMOD_SHIFT; // Add shift.
    }
  } else if (key < -127) { // Unicode, no matching SDL keys
    uni.unicode = -key;
  } else if (key > 0) { // Symbols such as arrow-keys, etc, no matching unicode.
    switch (key) {
    case KEY_DOWN: sdl.key = SDLK_DOWN; break;
    case KEY_UP: sdl.key = SDLK_UP; break;
    case KEY_LEFT: sdl.key = SDLK_LEFT; break;
    case KEY_RIGHT: sdl.key = SDLK_RIGHT; break;
    case KEY_BACKSPACE: sdl.key = SDLK_BACKSPACE; break;
    case KEY_F(1): sdl.key = SDLK_F1; break;
    case KEY_F(2): sdl.key = SDLK_F2; break;
    case KEY_F(3): sdl.key = SDLK_F3; break;
    case KEY_F(4): sdl.key = SDLK_F4; break;
    case KEY_F(5): sdl.key = SDLK_F5; break;
    case KEY_F(6): sdl.key = SDLK_F6; break;
    case KEY_F(7): sdl.key = SDLK_F7; break;
    case KEY_F(8): sdl.key = SDLK_F8; break;
    case KEY_F(9): sdl.key = SDLK_F9; break;
    case KEY_F(10): sdl.key = SDLK_F10; break;
    case KEY_F(11): sdl.key = SDLK_F11; break;
    case KEY_F(12): sdl.key = SDLK_F12; break;
    case KEY_F(13): sdl.key = SDLK_F13; break;
    case KEY_F(14): sdl.key = SDLK_F14; break;
    case KEY_F(15): sdl.key = SDLK_F15; break;
    case KEY_DC: sdl.key = SDLK_DELETE; break;
    case KEY_NPAGE: sdl.key = SDLK_PAGEDOWN; break;
    case KEY_PPAGE: sdl.key = SDLK_PAGEUP; break;
    case KEY_ENTER: sdl.key = SDLK_RETURN; break;
    }
  }

  // We may be registering a new mapping, in which case we skip the
  // rest of this function.
  if (key_registering) {
    if (uni.unicode) {
      stored_keys.push_back(uni);
    }
    if (sdl.key) {
      stored_keys.push_back(sdl);
    }
    Event e; e.r = REPEAT_NOT; e.repeats = 0; e.time = now; e.serial = serial; e.k = INTERFACEKEY_KEYBINDING_COMPLETE; e.tick = enabler.simticks.read();
    timeline.insert(e);
    key_registering = false;
    return;
  }
  
  // Key repeat is handled by the terminal, and we don't get release
  // events anyway.
  KeyEvent kev; kev.release = false;
  Event e; e.r = REPEAT_NOT; e.repeats = 0; e.time = now;
  if (sdl.key) {
    set<InterfaceKey> events = key_translation(sdl);
    for (set<InterfaceKey>::iterator k = events.begin(); k != events.end(); ++k) {
      e.serial = serial;
      e.k = *k;
      timeline.insert(e);
    }
  }
  if (uni.unicode) {
    set<InterfaceKey> events = key_translation(uni);
    for (set<InterfaceKey>::iterator k = events.begin(); k != events.end(); ++k) {
      e.serial = serial;
      e.k = *k;
      timeline.insert(e);
    }
  }
}
#endif

void enabler_inputst::add_input_refined(KeyEvent &e, Uint32 now, int serial) {
  // We may be registering a new mapping, in which case we skip the
  // rest of this function.
  if (key_registering && !e.release) {
    stored_keys.push_back(e.match);
    Event e; e.r = REPEAT_NOT; e.repeats = 0; e.time = now; e.serial = serial; e.k = INTERFACEKEY_KEYBINDING_COMPLETE; e.tick = enabler.simticks.read();
    timeline.insert(e);
    return;
  }

  // If this is a key-press event, we add it to the timeline. If it's
  // a release, we remove any pending repeats, but not those that
  // haven't repeated yet (which are on their first cycle); those we
  // just set to non-repeating.
  set<InterfaceKey> keys = key_translation(e.match);
  if (e.release) {
    set<Event>::iterator it = timeline.begin();
    while (it != timeline.end()) {
      set<Event>::iterator el = it++;
      if (keys.count(el->k)) {
        if (el->repeats) {
          timeline.erase(el);
        } else {
          Event new_el = *el;
          new_el.r = REPEAT_NOT;
          timeline.erase(el);
          timeline.insert(new_el);
        }
      }
    }
  } else {
    set<InterfaceKey>::iterator key;
    // As policy, when the user hits a non-repeating key we'd want to
    // also cancel any keys that are currently repeating. This allows
    // for easy recovery from stuck keys.
    //
    // Unfortunately, each key may be bound to multiple
    // commands. So, lacking information on which commands are
    // accepted at the moment, there is no way we can know if it's
    // okay to cancel repeats unless /all/ the bindings are
    // non-repeating.
    for (set<InterfaceKey>::iterator k = keys.begin(); k != keys.end(); ++k) {
      Event e = {key_repeat(*k), *k, 0, serial, now, enabler.simticks.read()};
      timeline.insert(e);
    }
    // if (cancel_ok) {
    //   // Set everything on the timeline to non-repeating
    //   multimap<Time,Event>::iterator it;
    //   for (it = timeline.begin(); it != timeline.end(); ++it) {
    //     it->second.r = REPEAT_NOT;
    //   }
  }
}


void enabler_inputst::clear_input() {
  timeline.clear();
  pressed_keys.clear();
  modState = 0;
  last_serial = 0;
}

set<InterfaceKey> enabler_inputst::get_input(Time now) {
  // We walk the timeline, returning all events corresponding to a
  // single physical keypress, and inserting repeats relative to the
  // current time, not when the events we're now returning were
  // *supposed* to happen.

  set<InterfaceKey> input;
  set<Event>::iterator ev = timeline.begin();
  if (ev == timeline.end() || ev->time > now) {
    return input; // No input (yet).
  }

  const Time first_time = ev->time;
  const int first_serial = ev->serial;
  int simtick = enabler.simticks.read();
  bool event_from_macro = false;
  while (ev != timeline.end() && ev->time == first_time && ev->serial == first_serial) {
    // Avoid recording macro-sources events as macro events.
    if (ev->macro) event_from_macro = true;
    // To make sure the user had a chance to cancel (by lifting the key), we require there
    // to be at least three simulation ticks before the first repeat.
    if (ev->repeats == 1 && ev->tick > simtick - 3) {
    } else {
      input.insert(ev->k);
    }
    // Schedule a repeat
    Event next = *ev;
    next.repeats++;
    switch (next.r) {
    case REPEAT_NOT:
      break;
    case REPEAT_SLOW:
      if (ev->repeats == 0) {
        next.time = now + init.input.hold_time;
        timeline.insert(next);
        break;
      }
    case REPEAT_FAST:
      double accel = 1;
      if (ev->repeats >= init.input.repeat_accel_start) {
        // Compute acceleration
        accel = MIN(init.input.repeat_accel_limit,
                    sqrt(double(next.repeats - init.input.repeat_accel_start) + 16) - 3);
      } 
      next.time = now + double(init.input.repeat_time) / accel;
      timeline.insert(next);
      break;
    }
    // Delete the event from the timeline and iterate
    timeline.erase(ev++);
  }
#ifdef DEBUG
  if (input.size() && !init.display.flag.has_flag(INIT_DISPLAY_FLAG_TEXT)) {
    cout << "Returning input:\n";
    set<InterfaceKey>::iterator it;
    for (it = input.begin(); it != input.end(); ++it)
        cout << "    " << GetKeyDisplay(*it) << ": " << GetBindingDisplay(*it) << endl;
  }
#endif
  // It could be argued that the "record event" step of recording
  // belongs in add_input, not here.  I don't hold with this
  // argument. The whole point is to record events as the user seems
  // them happen.
  if (macro_recording && !event_from_macro) {
    set<InterfaceKey> macro_input = input;
    macro_input.erase(INTERFACEKEY_RECORD_MACRO);
    macro_input.erase(INTERFACEKEY_PLAY_MACRO);
    macro_input.erase(INTERFACEKEY_SAVE_MACRO);
    macro_input.erase(INTERFACEKEY_LOAD_MACRO);
    if (macro_input.size())
      active_macro.push_back(macro_input);
  }
  return input;
}

set<InterfaceKey> enabler_inputst::key_translation(EventMatch &match) {
  set<InterfaceKey> bindings;
  pair<multimap<EventMatch,InterfaceKey>::iterator,multimap<EventMatch,InterfaceKey>::iterator> its;
  
  for (its = keymap.equal_range(match); its.first != its.second; ++its.first)
    bindings.insert((its.first)->second);

  return bindings;
}

string enabler_inputst::GetKeyDisplay(int binding) {
  map<InterfaceKey,set<string,less_sz> >::iterator it = keydisplay.find(binding);
  if (it != keydisplay.end() && it->second.size())
    return *it->second.begin();
  else {
    cout << "Missing binding displayed: " + bindingNames.left[binding] << endl;
    return "?";
  }
}

string enabler_inputst::GetBindingDisplay(int binding) {
  map<InterfaceKey,string>::iterator it = bindingNames.left.find(binding);
  if (it != bindingNames.left.end())
    return it->second;
  else
    return "NO BINDING";
}

string enabler_inputst::GetBindingTextDisplay(int binding) {
  map<InterfaceKey,string>::iterator it = displayNames.left.find(binding);
  if (it !=displayNames.left.end())
    return it->second;
  else
    return "NO BINDING";
}

Repeat enabler_inputst::key_repeat(InterfaceKey binding) {
  map<InterfaceKey,Repeat>::iterator it = repeatmap.find(binding);
  if (it != repeatmap.end())
    return it->second;
  else
    return REPEAT_NOT;
}

void enabler_inputst::key_repeat(InterfaceKey binding, Repeat repeat) {
  repeatmap[binding] = repeat;
}

void enabler_inputst::record_input() {
  active_macro.clear();
  macro_recording = true;
} 

void enabler_inputst::record_stop() {
  macro_recording = false;
}

bool enabler_inputst::is_recording() {
  return macro_recording;
}

void enabler_inputst::play_macro() {
  Time now = SDL_GetTicks();
  for_each(timeline.begin(), timeline.end(), [&](Event e){
      now = MAX(now, e.time);
    });
  for (macro::iterator sim = active_macro.begin(); sim != active_macro.end(); ++sim) {
    Event e; e.r = REPEAT_NOT; e.repeats = 0; e.serial = next_serial(); e.time = now;
    e.macro = true;  // Avoid exponential macro blowup.
    for (set<InterfaceKey>::iterator k = sim->begin(); k != sim->end(); ++k) {
      e.k = *k;
      timeline.insert(e);
      now += init.input.macro_time;
    }
  }
  macro_end = MAX(macro_end, now);
}

bool enabler_inputst::is_macro_playing() {
  return SDL_GetTicks() <= macro_end;
}

// Replaces any illegal letters.
static string filter_filename(string name, char replacement) {
  for (int i = 0; i < name.length(); i++) {
    switch (name[i]) {
    case '<': name[i] = replacement; break;
    case '>': name[i] = replacement; break;
    case ':': name[i] = replacement; break;
    case '"': name[i] = replacement; break;
    case '/': name[i] = replacement; break;
    case '\\': name[i] = replacement; break;
    case '|': name[i] = replacement; break;
    case '?': name[i] = replacement; break;
    case '*': name[i] = replacement; break;
    }
    if (name[i] <= 31) name[i] = replacement;
  }
  return name;
}

void enabler_inputst::load_macro_from_file(const string &file) {
  ifstream s(file.c_str());
  char buf[100];
  s.getline(buf, 100);
  string name(buf);
  if (macros.find(name) != macros.end()) return; // Already got it.

  macro macro;
  set<InterfaceKey> group;
  for(;;) {
    s.getline(buf, 100);
    if (!s.good()) {
      MessageBox(NULL, "I/O error while loading macro", 0, 0);
      s.close();
      return;
    }
    string line(buf);
    if (line == "End of macro") {
      if (group.size()) macro.push_back(group);
      break;
    } else if (line == "\tEnd of group") {
      if (group.size()) macro.push_back(group);
      group.clear();
    } else if (line.substr(0,2) != "\t\t" ) {
      if( line.substr(1).find("\t") != string::npos) {
        // expecting /t##/tCMD for a repeated command
        istringstream ss(line.substr(1));
        int count;
        string remainingline;

        if(ss >> count) {
          ss >> remainingline;
          if(remainingline.size()) {
            for(int i=0; i < count; i++) {
              map<string,InterfaceKey>::iterator it = bindingNames.right.find(remainingline);
              if (it == bindingNames.right.end()) {
                cout << "Binding name unknown while loading macro: " << line.substr(1) << endl;
              } else {
                group.insert(it->second);
                if (group.size()) macro.push_back(group);
                group.clear();
              }
            }
          }
          else {
            cout << "Binding missing while loading macro: " << line.substr(1) << endl;
          }
        } else {
          cout << "Quantity not numeric or Unexpected tab(s) while loading macro: " << line.substr(1) << endl;
        }
      }
      else
      {
        // expecting /tCMD for a non-grouped command
        map<string,InterfaceKey>::iterator it = bindingNames.right.find(line.substr(1));
        if (it == bindingNames.right.end()) {
          cout << "Binding name unknown while loading macro: " << line.substr(1) << endl;
        } else {
          group.insert(it->second);
          if (group.size()) macro.push_back(group);
          group.clear();
        }
      }
    } else {
      map<string,InterfaceKey>::iterator it = bindingNames.right.find(line.substr(2));
      if (it == bindingNames.right.end())
        cout << "Binding name unknown while loading macro: " << line.substr(2) << endl;
      else
        group.insert(it->second);
    }
  }
  if (s.good())
    macros[name] = macro;
  else
    MessageBox(NULL, "I/O error while loading macro", 0, 0);
  s.close();
}

void enabler_inputst::save_macro_to_file(const string &file, const string &name, const macro &macro) {
  ofstream s(file.c_str());
  s << name << endl;
  for (macro::const_iterator group = macro.begin(); group != macro.end(); ++group) {
    for (set<InterfaceKey>::const_iterator key = group->begin(); key != group->end(); ++key)
      s << "\t\t" << bindingNames.left[*key] << endl;
    s << "\tEnd of group" << endl;
  }
  s << "End of macro" << endl;
  s.close();
}

list<string> enabler_inputst::list_macros() {
  // First, check for unloaded macros
  svector<char*> files;
  find_files_by_pattern("data/init/macros/*.mak", files);
  for (int i = 0; i < files.size(); i++) {
    string file(files[i]);
    delete files[i];
    file = "data/init/macros/" + file;
    load_macro_from_file(file);
  }
  // Then return all in-memory macros
  list<string> ret;
  for (map<string,macro>::iterator it = macros.begin(); it != macros.end(); ++it)
    ret.push_back(it->first);
  return ret;
}

void enabler_inputst::load_macro(string name) {
  if (macros.find(name) != macros.end())
    active_macro = macros[name];
  else
    macros.clear();
}

void enabler_inputst::save_macro(string name) {
  macros[name] = active_macro;
  CreateDirectory("data",NULL);
  CreateDirectory("data/init",NULL);
  CreateDirectory("data/init/macros",NULL);
  save_macro_to_file("data/init/macros/" + filter_filename(name, '_') + ".mak", name, active_macro);
}

void enabler_inputst::delete_macro(string name) {
  map<string,macro>::iterator it = macros.find(name);
  if (it != macros.end()) macros.erase(it);
  // TODO: Store the filename it was loaded from instead
  string filename = "data/init/macros/" + filter_filename(name, '_') + ".mak";
  remove(filename.c_str());
}


// Sets the next key-press to be stored instead of executed.
void enabler_inputst::register_key() {
  key_registering = true;
  stored_keys.clear();
}

// Returns a description of stored keys. Max one of each type.
list<RegisteredKey> enabler_inputst::getRegisteredKey() {
  key_registering = false;
  list<RegisteredKey> ret;
  for (list<EventMatch>::iterator it = stored_keys.begin(); it != stored_keys.end(); ++it) {
    struct RegisteredKey r = {it->type, display(*it)};
    ret.push_back(r);
  }
  return ret;
}

// Binds one of the stored keys to key
void enabler_inputst::bindRegisteredKey(MatchType type, InterfaceKey key) {
  for (list<EventMatch>::iterator it = stored_keys.begin(); it != stored_keys.end(); ++it) {
    if (it->type == type) {
      keymap.insert(pair<EventMatch,InterfaceKey>(*it, key));
      update_keydisplay(key, display(*it));
    }
  }
}

bool enabler_inputst::is_registering() {
  return key_registering;
}


list<EventMatch> enabler_inputst::list_keys(InterfaceKey key) {
  list<EventMatch> ret;
  // Oh, now this is inefficient.
  for (multimap<EventMatch,InterfaceKey>::iterator it = keymap.begin(); it != keymap.end(); ++it)
    if (it->second == key) ret.push_back(it->first);
  return ret;
}

void enabler_inputst::remove_key(InterfaceKey key, EventMatch ev) {
  for (multimap<EventMatch,InterfaceKey>::iterator it = keymap.find(ev);
       it != keymap.end() && it->first == ev;
       ++it) {
    if (it->second == key) keymap.erase(it++);
  }
  // Also remove the key from key displaying, assuming we can find it
  map<InterfaceKey,set<string,less_sz> >::iterator it = keydisplay.find(key);
  if (it != keydisplay.end())
    it->second.erase(display(ev));
}

bool enabler_inputst::prefix_building() {
  return in_prefix_command;
}

void enabler_inputst::prefix_toggle() {
  in_prefix_command = !in_prefix_command;
  prefix_command.clear();
}

void enabler_inputst::prefix_add_digit(char digit) {
  prefix_command.push_back(digit);
#ifdef DEBUG
  cout << "Built prefix to " << prefix_command << endl;
#endif
  if (atoi(prefix_command.c_str()) > 99)
    prefix_command = "99";  // Let's not go overboard here.
}

int enabler_inputst::prefix_end() {
  if (prefix_command.size()) {
    int repeats = atoi(prefix_command.c_str());
    prefix_toggle();
    return repeats;
  } else {
    return 1;
  }
}

string enabler_inputst::prefix() {
  return prefix_command;
}
