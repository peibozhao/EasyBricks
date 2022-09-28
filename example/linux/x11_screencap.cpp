
#include "x11_screencap.h"
#include <X11/Xutil.h>
#include <iostream>

X11ScreenCapture::X11ScreenCapture() : X11ScreenCapture("") {}

X11ScreenCapture::X11ScreenCapture(const std::string &win_name)
    : window_name_(win_name), display_(nullptr) {}

bool X11ScreenCapture::Init() {
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

void X11ScreenCapture::CaptureRawImage(ImageFormat &format, unsigned int &width,
                                       unsigned int &height,
                                       std::vector<uint8_t> &buffer) {
  XWindowAttributes attr;
  Status status = XGetWindowAttributes(display_, window_, &attr);

  format = ImageFormat::RGB_888_PACKED;
  buffer.clear();
  XImage *image = XGetImage(display_, window_, 0, 0, attr.width, attr.height,
                            AllPlanes, ZPixmap);
  int red_mask = image->red_mask;
  int green_mask = image->green_mask;
  int blue_mask = image->blue_mask;

  width = image->width;
  height = image->height;
  for (int y = 0; y < image->height; ++y) {
    for (int x = 0; x < image->width; ++x) {
      unsigned long pixel = XGetPixel(image, x, y);
      int blue = pixel & blue_mask;
      int green = (pixel & green_mask) >> 8;
      int red = (pixel & red_mask) >> 16;
      buffer.push_back(red);
      buffer.push_back(green);
      buffer.push_back(blue);
    }
  }
}
