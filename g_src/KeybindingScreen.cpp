#ifdef __APPLE__
# include "osx_messagebox.h"
#elif defined(unix)
# include <gtk/gtk.h>
#endif

#include "GL/glew.h"

#ifndef INTEGER_TYPES

#define INTEGER_TYPES

#ifdef WIN32
	typedef signed char int8_t;
	typedef short int16_t;
	typedef int int32_t;
	typedef long long int64_t;
	typedef unsigned char uint8_t;
	typedef unsigned short uint16_t;
	typedef unsigned int uint32_t;
	typedef unsigned long long uint64_t;
#endif

typedef int32_t VIndex;
typedef int32_t Ordinal;

#endif

#include "graphics.h"
#include "init.h"
#include "keybindings.h"
#include "KeybindingScreen.h"

#include <list>
#include <map>
#include <iostream>
#include <sstream>
#include <ctype.h>

using namespace std;

struct BindingGroup {
  string name;
  InterfaceKey start, end;
};

const BindingGroup groups[] = {
  {"General"    , INTERFACEKEY_NONE,        WORLDKEY_START-1},
  {"World"      , WORLDKEY_START,           ADVENTURERKEY_START-1},
  {"Adventurer" , ADVENTURERKEY_START,      EMBARKKEY_START-1},
  {"Dwarf mode" , DWARFMAINKEY_START,       MILITIAKEY_START-1},
  {"Embark"     , EMBARKKEY_START,          BUILDINGKEY_START-1},
  {"Building"   , BUILDINGKEY_START,        WORKSHOPKEY_START-1},
  {"Workshop"   , WORKSHOPKEY_START,        PILEZONEKEY_START-1},
  {"Pilezone"   , PILEZONEKEY_START,        STOCKORDERKEY_START-1},
  {"Stockorder" , STOCKORDERKEY_START,      DWARFMAINKEY_START-1},
  {"Militia"    , MILITIAKEY_START,         INTERFACEKEY_STRING_A000-1},
  {"Text entry" , INTERFACEKEY_STRING_A000, INTERFACEKEY_STRING_A255}
};

KeybindingScreen::KeybindingScreen() {
  gview.addscreen(this, INTERFACE_PUSH_AT_BACK, NULL); // HACK
  mode = mode_main;

  main.add("Macros", sel_macros);
  for (int i = 0; i < ARRSZ(groups); i++)
    main.set(i+2, groups[i].name, sel_first_group + i);
  main.set(ARRSZ(groups)+3, "Save and exit", sel_save_exit);
  main.add("Exit, discard changes when DF quits", sel_just_exit);
  enabler.flag |= ENABLERFLAG_RENDER;
}

void KeybindingScreen::feed(set<InterfaceKey> &input) {
  enabler.flag|=ENABLERFLAG_RENDER;
  if (input.count(INTERFACEKEY_KEYBINDING_COMPLETE)) {
    list<RegisteredKey> keys = enabler.getRegisteredKey();
    if (keys.size() == 0) {
      puts("No keys registered ?!");
      mode = mode_keyR;
    } else {
      keyRegister.clear();
      list<RegisteredKey> keys = enabler.getRegisteredKey();
      for (list<RegisteredKey>::iterator it = keys.begin(); it != keys.end(); ++it) {
        string display;
        switch (it->type) {
        case type_button: display = "Mouse button: "; break;
        case type_key: display = "By position: "; break;
        case type_unicode: display = "By letter: "; break;
        }
        keyRegister.add(display + it->display, it->type);
      }
    }
  } else if (input.count(INTERFACEKEY_STANDARDSCROLL_PAGEUP) ||
      input.count(INTERFACEKEY_STANDARDSCROLL_PAGEDOWN) ||
      input.count(INTERFACEKEY_STANDARDSCROLL_UP) ||
      input.count(INTERFACEKEY_STANDARDSCROLL_DOWN)) {
    switch (mode) {
    case mode_main: main.feed(input); break;
    case mode_keyL: keyL.feed(input); reset_keyR(); break;
    case mode_keyR: keyR.feed(input); break;
    case mode_macro: macro.feed(input); break;
    case mode_register: keyRegister.feed(input); break;
    }
  } else if (mode == mode_keyL && input.count(INTERFACEKEY_STANDARDSCROLL_RIGHT))
    mode = mode_keyR;
  else if (mode == mode_main && input.count(INTERFACEKEY_STANDARDSCROLL_RIGHT)) {
    if (main.get_selection() == sel_macros) enter_macros();
    if (main.get_selection() >= sel_first_group)
      enter_key(main.get_selection() - sel_first_group);
  } else if (mode == mode_keyR && input.count(INTERFACEKEY_STANDARDSCROLL_LEFT))
    mode = mode_keyL;
  else if ((mode == mode_keyL || mode == mode_macro) && input.count(INTERFACEKEY_STANDARDSCROLL_LEFT))
    mode = mode_main;
  else if (input.count(INTERFACEKEY_STRING_A000)) { // Backspace: Delete something.
    switch (mode) {
    case mode_macro:
      if (macro.get_selection() != "") {
        enabler.delete_macro(macro.get_selection());
        macro.del_selection();
        if (!macro.size())
          macro.add("No macros!", "");
      }
      break;
    case mode_keyR:
      keyR_selector sel = keyR.get_selection();
      if (sel.sel == sel_event) {
        enabler.remove_key(keyL.get_selection(), sel.event);
        reset_keyR();
      }
      break;
    }
  } else if (input.count(INTERFACEKEY_SELECT)) {
    switch (mode) {
    case mode_main:
      if (main.get_selection() == sel_macros) { // Macros
        enter_macros();
      } else if (main.get_selection() == sel_save_exit) { // Save and exit
        enabler.save_keybindings();
        breakdownlevel = INTERFACE_BREAKDOWN_STOPSCREEN;
        return;
      } else if (main.get_selection() == sel_just_exit) { // Just exit
        breakdownlevel = INTERFACE_BREAKDOWN_STOPSCREEN;
        return;
      } else { // Some key-binding group
        enter_key(main.get_selection() - sel_first_group);
      }
      break;
    case mode_keyR: {
      InterfaceKey key = keyL.get_selection();
      switch (keyR.get_selection().sel) {
      case sel_add:
        enabler.register_key();
        mode = mode_register;
        break;
      case sel_rep_none:
        enabler.key_repeat(key, REPEAT_NOT);
        reset_keyR();
        break;
      case sel_rep_slow:
        enabler.key_repeat(key, REPEAT_SLOW);
        reset_keyR();
        break;
      case sel_rep_fast:
        enabler.key_repeat(key, REPEAT_FAST);
        reset_keyR();
        break;
      }}
      break;
    case mode_register:
      enabler.bindRegisteredKey(keyRegister.get_selection(), keyL.get_selection());
      mode = mode_keyR;
      reset_keyR();
      break;
    }
  } else if (input.count(INTERFACEKEY_LEAVESCREEN) || input.count(INTERFACEKEY_OPTIONS)) {
    if (mode == mode_register)
      mode = mode_keyR;
    else
      mode = mode_main;
  }
}

void KeybindingScreen::logic() {
  if (mode == mode_register)
    enabler.flag|=ENABLERFLAG_RENDER;
}

void KeybindingScreen::enter_macros() {
  mode = mode_macro;
  macro.clear();
  list<string> macros = enabler.list_macros();
  for (list<string>::iterator it = macros.begin(); it != macros.end(); ++it)
    macro.add(*it, *it);
  if (!macros.size())
    macro.add("No macros!", "");
}

void KeybindingScreen::enter_key(int group) {
  mode = mode_keyL;
  keyL.clear();
  for (InterfaceKey i = groups[group].start; i <= groups[group].end; i++) {
    if (i != INTERFACEKEY_NONE)
      keyL.add(enabler.GetBindingTextDisplay(i), i);
  }
  reset_keyR();
}

void KeybindingScreen::reset_keyR() {
  int lastpos = keyR.get_pos();
  keyR.clear();
  struct keyR_selector sel;
  sel.sel = sel_add;
  keyR.add("Add binding", sel);
  InterfaceKey key = keyL.get_selection();
  list<EventMatch> matchers = enabler.list_keys(key);
  Repeat rep = enabler.key_repeat(key);
  sel.sel = sel_rep_none;
  keyR.set(2, "Don't repeat", sel);
  if (rep == REPEAT_NOT) keyR.set_color(2, 4, 0);
  sel.sel = sel_rep_slow;
  keyR.set(3, "Delayed repeat", sel);
  if (rep == REPEAT_SLOW) keyR.set_color(3, 4, 0);
  sel.sel = sel_rep_fast;
  keyR.set(4, "Immediate repeat", sel);
  if (rep == REPEAT_FAST) keyR.set_color(4, 4, 0);
  int i = 6;
  for (list<EventMatch>::iterator it = matchers.begin(); it != matchers.end(); ++it, ++i) {
    ostringstream desc;
    switch (it->type) {
    case type_unicode:
      desc << "By letter: ";
      if (it->unicode < 256 && isgraph(it->unicode)) // Is it printable?
        desc << (char)it->unicode;
      else
        desc << "U+" << hex << uppercase << it->unicode;
      break;
    case type_key:
      desc << "By position: " << translate_mod(it->mod) << sdlNames.left[it->key];
      break;
    case type_button:
      desc << "Mouse: " << (int)it->button;
      break;
    }
    sel.sel = sel_event;
    sel.event = *it;
    keyR.set(i, desc.str(), sel);
  }
  keyR.set_pos(lastpos);
}

void KeybindingScreen::render_macro() {
  drawborder("Macros");
  gps.locate(3, 3);
  gps.changecolor(4,0,1);
  gps.addst("Select a macro, then press " + enabler.GetKeyDisplay(INTERFACEKEY_STRING_A000) + " to delete.");
  macro.render(6, init.display.grid_x-2, 5, init.display.grid_y-2);
}

void KeybindingScreen::render_key() {
  if (enabler.is_registering()) {
    gps.changecolor(4,0,1);
    drawborder("Keybinding - currently registering new key");
  } else
    drawborder("Keybinding");
  gps.locate(3, 6);
  gps.changecolor(4,0,1);
  gps.addst("Select a binding, then press " + enabler.GetKeyDisplay(INTERFACEKEY_STRING_A000) + " to delete.");
  keyL.render(6, init.display.grid_x/2 - 1, 5, init.display.grid_y-2);
  if (mode == mode_keyL || mode == mode_register)
    keyR.bleach(true);
  else
    keyR.bleach(false);
  keyR.render(init.display.grid_x/2 + 1, init.display.grid_x-2, 5, init.display.grid_y-2);
}

void KeybindingScreen::render_register() {
  int x1 = init.display.grid_x / 2 - 20,
    x2 = init.display.grid_x / 2 + 20,
    y1 = init.display.grid_y / 2 - 1,
    y2 = init.display.grid_y / 2 + 1;
  if (!enabler.is_registering()) {
    y2 = y1 + keyRegister.size() + 1;
  }
  gps.erasescreen_rect(x1, x2, y1, y2);
  gps.changecolor(1,1,1);
  for (int x = x1; x <= x2; x++) {
    gps.locate(y1, x); gps.addchar(' ');
    gps.locate(y2, x); gps.addchar(' ');
  }
  for (int y = y1 + 1; y < y2; y++) {
    gps.locate(y, x1); gps.addchar(' ');
    gps.locate(y, x2); gps.addchar(' ');
  }
  if (enabler.is_registering()) {
    gps.changecolor(7,0,1);
    gps.locate(y1+1, x1+2);
    gps.addst(translate_mod(getModState()));
  } else {
    keyRegister.render(x1+1, x2-1, y1+1, y2-1);
    gps.locate(y2, x1+2);
    gps.changecolor(7,1,1);
    gps.addst("Select binding, or press " + enabler.GetKeyDisplay(INTERFACEKEY_LEAVESCREEN) + " to abort");
  }
}

// Render the main menu
void KeybindingScreen::render_main() {
  drawborder("Key binding & macro center");
  main.render(6, init.display.grid_x - 3, 3, init.display.grid_y - 4);
}

void KeybindingScreen::render() {
  switch(mode) {
  case mode_main: render_main(); break;
  case mode_keyL: case mode_keyR: render_key(); break;
  case mode_macro: render_macro(); break;
  case mode_register:
    render_key();
    render_register();
    break;
  }
}

void KeybindingScreen::help() {
}


MacroScreenLoad::MacroScreenLoad() {
  list<string> macros = enabler.list_macros();
  width = 10;
  if (!macros.size()) {
    menu.add("No macros!", "");
    height = 1;
  } else
    height = macros.size();

  for (list<string>::iterator it = macros.begin(); it != macros.end(); ++it) {
    if (it->length() > width) width = it->length();
    menu.add(*it, *it);
  }
  enabler.flag |= ENABLERFLAG_RENDER;
  // render();
  // gps.renewscreen();
}

void MacroScreenLoad::feed(set<InterfaceKey> &input) {
  enabler.flag|=ENABLERFLAG_RENDER;
  if (input.count(INTERFACEKEY_SELECT)) {
    string id = menu.get_selection();
    if (id != "") enabler.load_macro(id);
    breakdownlevel = INTERFACE_BREAKDOWN_STOPSCREEN;
    return;
  } else if (input.count(INTERFACEKEY_LEAVESCREEN)) {
    breakdownlevel = INTERFACE_BREAKDOWN_STOPSCREEN;
    return;
  } else {
    menu.feed(input);
  }
  if (input.count(INTERFACEKEY_OPTIONS)) {
    breakdownlevel = INTERFACE_BREAKDOWN_STOPSCREEN;
  }
}

void MacroScreenLoad::logic() {
}

void MacroScreenLoad::render() {
  if (parent) parent->render();
  const int x1 = MAX(init.display.grid_x/2 - ((width + 2) / 2), 0);
  const int x2 = MIN(x1+width+1, init.display.grid_x-1);
  const int y1 = MAX(init.display.grid_y/2 - ((height + 2) / 2), 0);
  const int y2 = MIN(y1 + height + 1, init.display.grid_y-1);
  gps.changecolor(0,3,1);
  gps.draw_border(x1, x2, y1, y2);
  menu.render(x1+1, x2-1, y1+1, y2-1);
  // gps.renewscreen();
}

MacroScreenSave::MacroScreenSave() {
  enabler.flag |= ENABLERFLAG_RENDER;
}

void MacroScreenSave::logic() {
}

void MacroScreenSave::feed(set<InterfaceKey> &input) {
  enabler.flag|=ENABLERFLAG_RENDER;
  id.feed(input);
  if (input.count(INTERFACEKEY_SELECT)) {
    string n = id.get_text();
    if (n.length())
      enabler.save_macro(n);
    breakdownlevel = INTERFACE_BREAKDOWN_STOPSCREEN;
    return;
  }
  if (input.count(INTERFACEKEY_OPTIONS)) {
    breakdownlevel = INTERFACE_BREAKDOWN_STOPSCREEN;
  }
}

void MacroScreenSave::render() {
  if (parent) parent->render();
  const int x1 = 3,
    x2 = init.display.grid_x-4,
    y1 = init.display.grid_y/2-1,
    y2 = init.display.grid_y/2+1;
  gps.changecolor(0,3,1);
  gps.draw_border(x1, x2, y1, y2);
  id.render(x1+1,x2-1,y1+1,y2-1);
  // gps.renewscreen();
}

