#ifndef KEYBINDINGSCREEN_H
#define KEYBINDINGSCREEN_H

#include "interface.h"
#include "ViewBase.h"
#include "enabler.h"

#include <set>
#include <string>

class KeybindingScreen : public viewscreenst {
  enum { mode_main, mode_keyL, mode_keyR, mode_macro, mode_register } mode;
  enum keyR_type { sel_add, sel_rep_none, sel_rep_slow, sel_rep_fast, sel_event };
  enum main_selector { sel_macros, sel_just_exit, sel_save_exit, sel_first_group };

  struct keyR_selector {
    keyR_type sel;
    EventMatch event; // Uninitialized if sel != sel_event
  };
  
  widgets::menu<int> main; // Representing main_selector
  widgets::menu<InterfaceKey> keyL;
  widgets::menu<keyR_selector> keyR;
  widgets::menu<std::string> macro;
  widgets::menu<MatchType> keyRegister;
  
  void render_main();
  void render_macro();
  void render_key();
  void render_register();

  void reset_keyR();
  
  void enter_key(int group);
  void enter_macros();
  
public:
  KeybindingScreen(); 
  virtual void feed(std::set<InterfaceKey> &events);
  virtual void render();
  virtual void help();
  virtual void logic();
  virtual char is_option_screen() { return 2; }
};

class MacroScreenLoad : public viewscreenst {
  widgets::menu<string> menu;
  int width, height;
  
 public:
  MacroScreenLoad();
  virtual void logic();
  virtual void render();
  virtual void feed(std::set<InterfaceKey> &events);
  virtual char is_option_screen() { return 1; }
};

class MacroScreenSave : public viewscreenst {
  widgets::textbox id;
public:
  MacroScreenSave();
  virtual void logic();
  virtual void render();
  virtual void feed(std::set<InterfaceKey> &events);
  virtual char is_option_screen() { return 1; }
};


#endif
