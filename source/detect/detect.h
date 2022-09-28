#pragma once

#include <string>
#include <vector>
#include "common/common.h"

struct ObjectBox {
  std::string class_name;
  RectangleI region;
};

class IObjectDetect {
public:
  virtual ~IObjectDetect() {}

  virtual bool Init() { return true; }

  /// @brief
  ///
  /// @param buffer Image buffer. Formation depends on implementation
  ///
  /// @return
  virtual std::vector<ObjectBox> Detect(const uint8_t *buffer,
                                        unsigned long buffer_size) = 0;
};
