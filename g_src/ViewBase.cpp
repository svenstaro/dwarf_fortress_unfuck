#include <assert.h>
#include <iostream>
#include "ViewBase.h"

using namespace std;
using namespace widgets;

void textbox::feed(set<InterfaceKey> &input) {
  // Backspace
  if (input.count(INTERFACEKEY_STRING_A000) && text.size())
    text.resize(text.size() - 1);
  // Hopefully we'll never get multiple characters in one input set,
  // but it's possible. We deal with this by inserting them in
  // alphabetical order.
  for (set<InterfaceKey>::iterator it = input.lower_bound(INTERFACEKEY_STRING_A001);
       it != input.end() && *it <= INTERFACEKEY_STRING_A255;
       ++it) {
    if (keep == false) {
      keep = true;
      text.clear();
    }
    char c = *it - INTERFACEKEY_STRING_A000;
    text += c;
  }
}

void textbox::render(int x1, int x2, int y1, int y2) {
  // We need to do some kind of line-breaking for multi-line text
  // entry boxes. This shall be implemented at need, and there is none
  // yet.
  assert(y1 == y2);
  gps.erasescreen_rect(x1,x2,y1,y2);
  gps.locate(y1,x1);
  gps.changecolor(7,0,keep);
  int width = x2 - x1 + 1;
  int start = text.length() - width;
  gps.addst(text.substr(MAX(start,0)));
}
