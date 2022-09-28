
#include <X11/Xlib.h>
#include <vector>
#include <string>
#include "../../source/common/common.h"

class X11ScreenCapture {
public:
  X11ScreenCapture();

  X11ScreenCapture(const std::string &win_name);

  bool Init();

  void CaptureRawImage(ImageFormat &format, unsigned int &width,
                       unsigned int &height, std::vector<uint8_t> &buffer);

private:
  std::string window_name_;
  Display *display_;
  Window window_;
};
