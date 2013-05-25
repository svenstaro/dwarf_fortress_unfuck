#include <string>

#include "enabler.h"
#include "platform.h"
#ifndef WIN32
# include <sys/types.h>
# include <sys/stat.h>
# include <sys/time.h>
# include <signal.h>
# include <errno.h>
# include <stdio.h>
# include <string.h>
# ifdef __APPLE__
#  include "osx_messagebox.h"
# elif defined(unix)
#  include <gtk/gtk.h>
# endif
#endif

#ifndef WIN32
BOOL CreateDirectory(const char* pathname, void*)
{
  if (mkdir(pathname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
    if (errno != EEXIST) {
      std::string emsg = "mkdir(";
      emsg.append(pathname);
      emsg.append(") failed");
      perror(emsg.c_str());
    }
    return FALSE;
  } else {
    return TRUE;
  }
}

BOOL DeleteFile(const char* filename)
{
  return !unlink(filename);
}

void ZeroMemory(void* dest, int len)
{
  memset(dest, 0, len);
}

/* Returns milliseconds since 1970
 * Wraps every 24 days (assuming 32-bit signed dwords)
 */
DWORD GetTickCount()
{
  struct timeval tp;
  gettimeofday(&tp, NULL);
  return (tp.tv_sec * 1000) + (tp.tv_usec / 1000);
}

char* itoa(int value, char* result, int base)
{
  // check that the base is valid
  if (base < 2 || base > 16) { *result = 0; return result; }
	
  char* out = result;
  int quot = value;
	
  do
    {
      *out = "0123456789abcdef"[ /*std::*/abs(quot % base) ];
      ++out;
      quot /= base;
    }
  while (quot);
	
  if (value < 0) *out++ = '-';
	
  std::reverse(result, out);
  *out = 0;
  return result;
}

// Fills performanceCount with microseconds passed since 1970
// Wraps in twenty-nine thousand years or so
BOOL QueryPerformanceCounter(LARGE_INTEGER* performanceCount)
{
  struct timeval tp;
  gettimeofday(&tp, NULL);
  performanceCount->QuadPart = ((long long)tp.tv_sec * 1000000) + tp.tv_usec;
  return TRUE;
}

BOOL QueryPerformanceFrequency(LARGE_INTEGER* performanceCount)
{
  /* A constant, 10^6, as we give microseconds since 1970 in
   * QueryPerformanceCounter. */
  performanceCount->QuadPart = 1000000;
  
  return TRUE;
}

int MessageBox(HWND *dummy, const char *text, const char *caption, UINT type)
{
  bool toggle_screen = false;
  int ret = IDOK;
  if (enabler.is_fullscreen()) {
    enabler.toggle_fullscreen();
    toggle_screen = true;
  }
# ifdef __APPLE__ // Cocoa code
  if (type & MB_YESNO) {
    ret = CocoaAlertPanel(caption, text, "Yes", "No", NULL);
    ret = (ret == 0 ? IDNO : IDYES);
  } else {
    CocoaAlertPanel(caption, text, "OK", NULL, NULL);
  }
# else // GTK code
  if (getenv("DISPLAY")) {
    // Have X, will dialog
    GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                               GTK_DIALOG_DESTROY_WITH_PARENT,
                                               type & MB_YESNO ?
                                               GTK_MESSAGE_QUESTION :
                                               GTK_MESSAGE_ERROR,
                                               type & MB_YESNO ?
                                               GTK_BUTTONS_YES_NO :
                                               GTK_BUTTONS_OK,
                                               "%s", text);
    gtk_window_set_position((GtkWindow*)dialog, GTK_WIN_POS_CENTER_ALWAYS);
    gtk_window_set_title((GtkWindow*)dialog, caption);
    gint dialog_ret = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    while (gtk_events_pending())
      gtk_main_iteration();
    
    if (type & MB_YESNO) {
      switch (dialog_ret) {
      default:
      case GTK_RESPONSE_DELETE_EVENT:
      case GTK_RESPONSE_NO:
        ret = IDNO;
        break;
      case GTK_RESPONSE_YES:
        ret = IDYES;
        break;
      }
    }
  } else {
    // Use curses
    init_curses();
    erase();
    gps.force_full_display_count = 1;
    wattrset(*stdscr_p, A_NORMAL | COLOR_PAIR(1));
    
    mvwaddstr(*stdscr_p, 0, 5, caption);
    mvwaddstr(*stdscr_p, 2, 2, text);
    nodelay(*stdscr_p, false);
    if (type & MB_YESNO) {
      mvwaddstr(*stdscr_p, 5, 0, "Press 'y' or 'n'.");
      refresh();
      while (1) {
        char i = wgetch(*stdscr_p);
        if (i == 'y') {
          ret = IDYES;
          break;
        }
        else if (i == 'n') {
          ret = IDNO;
          break;
        }
      }
    }
    else {
      mvwaddstr(*stdscr_p, 5, 0, "Press any key to continue.");
      refresh();
      wgetch(*stdscr_p);
    }
    nodelay(*stdscr_p, -1);
  }
# endif
  
  if (toggle_screen) {
    enabler.toggle_fullscreen();
  }
	
  return ret;
}
#endif
