#ifndef VIEWBASE_H
#define VIEWBASE_H

#include <set>
#include <map>
#include <string>

#include "keybindings.h"
#include "graphics.h"

enum InterfaceBreakdownTypes
{
	INTERFACE_BREAKDOWN_NONE,
	INTERFACE_BREAKDOWN_QUIT,
	INTERFACE_BREAKDOWN_STOPSCREEN,
	INTERFACE_BREAKDOWN_TOFIRST,
	INTERFACE_BREAKDOWNNUM
};

class viewscreenst
{
	public:
		viewscreenst *child;
		viewscreenst *parent;
		char breakdownlevel;

		char option_key_pressed;
		virtual void feed(std::set<InterfaceKey> &events){}
		virtual void logic(){}
		virtual void render(){}
                virtual void resize(int w, int h){}

		virtual void help();
		virtual char movies_okay(){return 1;}
		virtual char is_option_screen(){return 0;}
		virtual char is_save_screen(){return 0;}
		viewscreenst()
			{
			child=0;
			parent=0;
			breakdownlevel=INTERFACE_BREAKDOWN_NONE;
			option_key_pressed=0;
			}
		virtual ~viewscreenst(){}

		virtual bool key_conflict(InterfaceKey test_key);
};

namespace widgets {

  using namespace std;
  
  template <typename T>
  class menu {
    typedef map<int,pair<string, T> > dict;
    dict lines;
    int selection;
    int last_displayheight;
    bool bleached;
    map<int, pair<int,int> > colors;
    
    // Given 'total' lines, with 'sel' selected, and 'space' to draw in,
    // returns the first line that should be drawn.
    int first_line(int total, int sel, int space) {
      // There is no doubt some clever math to do this, but I'm tired and don't care.
      for (int start = 0;; start += space / 2) {
        if (start + space/2 >= sel) return start;
        if (start + space >= total) return start;
      }
    }
    pair<string,T> mp(string s, T t) { return make_pair(s,t); }

    // Scrolls N lines up/down; positive = down
    void scroll(int n) {
      typename dict::iterator it = lines.find(selection);
      for (int i = 0; i < abs(n); i++) {
        if (n < 0 && it == lines.begin()) { // We've hit the top
          if (i) break;
          else {
            it = --(lines.end());
            break;
          }
        }
        if (n < 0) --it; else ++it; // Scroll one line
        if (it == lines.end()) { // We've hit the bottom
          if (i) {
            --it;
            break;
          }
          else {
            it = lines.begin();
            break;
          }
        }
        // If we hit neither the top nor bottom, loop.
      }

      selection = it->first;
    }
    
  public:
    menu() { clear(); }
    int size() { return lines.size(); }
    // Adds a line just past the last taken position
    void add(string text, T token) {
      if (!lines.size()) {
        lines[0] = mp(text,token);
      } else {
        typename dict::iterator it = --(lines.end());
        lines[it->first + 1] = mp(text,token);
      }
    }
    // (Re)sets the text of the given line
    void set(int line, string text, T token) {
      lines[line] = mp(text,token);
    }
    // Set the color of a line
    void set_color(int line, int fg, int bg) {
      colors[line] = make_pair(fg,bg);
    }
    // Handles (page) up/down
    void feed(std::set<InterfaceKey> &input) {
      if (!lines.size()) return;
      if (input.count(INTERFACEKEY_STANDARDSCROLL_UP)) scroll(-1);
      if (input.count(INTERFACEKEY_STANDARDSCROLL_DOWN)) scroll(1);
      if (input.count(INTERFACEKEY_STANDARDSCROLL_PAGEUP)) scroll(-(last_displayheight / 2));
      if (input.count(INTERFACEKEY_STANDARDSCROLL_PAGEDOWN)) scroll(last_displayheight / 2);        
    }
    void render(int x1, int x2, int y1, int y2) {
      gps.erasescreen_rect(x1,x2,y1,y2);
      int h = y2 - y1 + 1,
        w = x2 - x1 + 1,
        x = x1, y = y1;
      last_displayheight = h;
      if (!lines.size()) return;
      int total = (--lines.end())->first + 1;
      int first = first_line(total, selection, h);
      typename dict::iterator it = lines.lower_bound(first);
      for (; it != lines.end() && it->first - first < h; ++it) {
        gps.locate(it->first - first + y, x);
        map<int,pair<int,int> >::iterator color = colors.find(it->first - first);
        int fg = 7, bg = 0;
        if (color != colors.end()) {
          fg = color->second.first;
          bg = color->second.second;
        }
        gps.changecolor(fg, bg, it->first == selection && !bleached);
        gps.addst(it->second.first.substr(0, w));
      }
    }
    // Read out the current selection
    T get_selection() { return lines[selection].second; }
    int get_pos() { return selection; }
    // Set the position by line
    void set_pos(int pos) {
      if (pos < size())
        selection = pos;
    }
    // Delete the currently selected line
    void del_selection() {
      typename dict::iterator it = lines.find(selection);
      typename dict::iterator newsel = it;
      ++newsel;
      if (newsel == lines.end()) {
        newsel = it;
        --newsel;
      }
      lines.erase(it);
      if (lines.size()) selection = newsel->first;
    }
    // If true, don't draw a highlight
    void bleach(bool b) { bleached = b; }
    // Reset the menu
    void clear() {
      selection = 0;
      lines.clear();
      last_displayheight = 10;
      bleached = false;
      colors.clear();
    }
  };

  class textbox {
    string text;
    bool keep;
  public:
    textbox() { textbox("", false); }
    textbox(string initializer, bool keep) { this->keep = keep; text = initializer; }
    string get_text() { return text; }
    // Only cares about INTERFACEKEY_STRING events
    void feed(std::set<InterfaceKey> &input);
    void render(int x1, int x2, int y1, int y2);
  };

}

#endif
