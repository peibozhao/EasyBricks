#pragma once

#include <X11/Xlib.h>
#include <string>

class X11Event {
public:
  X11Event();

  X11Event(const std::string &win_name);

  bool Init();

  void Click(int x, int y);

private:
  Display *display_;
  Window window_;

  std::string window_name_;
};
