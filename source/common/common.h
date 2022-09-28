#pragma once

template <typename T>
struct Rectangle {
  T x, y;
  T width, height;

  Rectangle() { x = y = width = height = T(); }

  Rectangle(T x_center, T y_center, T w, T h) {
    x = x_center;
    y = y_center;
    width = w;
    height = h;
  }
};

typedef Rectangle<int> RectangleI;
typedef Rectangle<float> RectangleF;

enum class ImageFormat {
  RGB_888_PACKED = 1,
  RGBA_8888_PACKED = 2,
  JPEG = 100,
};

