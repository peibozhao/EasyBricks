#pragma once

#include <string>
#include <vector>
#include "common/common.h"

struct TextBox {
  std::string text;
  RectangleI region;
};

class IOcrDetect {
public:
  virtual ~IOcrDetect() {}

  virtual bool Init() { return true; }

  /// @brief
  ///
  /// @param buffer Image buffer. Formation depends on implementation
  /// @param buffer_size
  ///
  /// @return
  virtual std::vector<TextBox> Detect(const uint8_t *buffer,
                                      unsigned long buffer_size) = 0;
};
