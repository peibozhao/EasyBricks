
#include "x11_event.h"
#include <X11/Xutil.h>
#include <string.h>
#include <unistd.h>
#include "common/log.h"

X11Event::X11Event() : X11Event("") {}

X11Event::X11Event(const std::string &win_name)
    : display_(nullptr), window_name_(win_name) {}

bool X11Event::Init() {
  display_ = XOpenDisplay(nullptr);
  Window root = XDefaultRootWindow(display_);
  if (window_name_.empty()) {
    window_ = root;
    return true;
  }

  Atom a = XInternAtom(display_, "_NET_CLIENT_LIST", true);
  Atom actual_type;
  int xformat;
  unsigned long num_items, bytesAfter;
  unsigned char *data = NULL;
  int status = XGetWindowProperty(display_, root, a, 0L, (~0L), false,
                                  AnyPropertyType, &actual_type, &xformat,
                                  &num_items, &bytesAfter, &data);

  if (status == Success && num_items) {
    long *array = (long *)data;
    for (unsigned long k = 0; k < num_items; k++) {
      // Get window Id:
      Window w = (Window)array[k];
      XClassHint class_hint;
      status = XGetClassHint(display_, w, &class_hint);
      if (status >= Success && class_hint.res_class == window_name_) {
        window_ = w;
        return true;
      }
    }
    XFree(data);
  }
  return false;
}

void X11Event::Click(int x, int y) {
  Window root = XDefaultRootWindow(display_);
  int root_x, root_y;
  Window child;
  XTranslateCoordinates(display_, window_, root, x, y, &root_x, &root_y,
                        &child);

  XEvent event;
  memset(&event, 0, sizeof(event));
  event.xbutton.button = Button1;
  event.xbutton.send_event = True;
  event.xbutton.same_screen = True;
  event.xbutton.window = window_;
  event.xbutton.subwindow = None;
  event.xbutton.x = x;
  event.xbutton.y = y;
  event.xbutton.root = root;
  event.xbutton.x_root = root_x;
  event.xbutton.y_root = root_y;
  event.xbutton.state = Button1Mask;
  LOG(INFO) << "Click " << event.xbutton.x << "," << event.xbutton.y << " "
            << event.xbutton.x_root << "," << event.xbutton.y_root;

  // Move
  XWarpPointer(display_, None, window_, 0, 0, 0, 0, x, y);
  XFlush(display_);
  usleep(1);
  // Press
  event.type = ButtonPress;
  if (XSendEvent(display_, window_, True, ButtonPressMask, &event) == 0) {
    return;
  }
  XFlush(display_);
  usleep(1);
  // Release
  event.type = ButtonRelease;
  if (XSendEvent(display_, window_, True, ButtonReleaseMask, &event) == 0) {
    return;
  }
  XFlush(display_);
  usleep(1);
}
