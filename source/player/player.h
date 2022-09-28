#pragma once

#include <string>
#include <vector>

/// @brief Element in page
struct Element {
  std::string name;
  float x, y;

  Element() : name(""), x(0.), y(0.) {}
  Element(const std::string &n, float ix, float iy) : name(n), x(ix), y(iy) {}
};

struct PlayOperation {
  enum class Type {
    SCREEN_CLICK = 1,  // Click
    SLEEP,             // Wait
    OVER               // Play mode over
  };

  struct Click {
    float x, y;
    Click(float x_center = 0, float y_center = 0) : x(x_center), y(y_center) {}
  };

  Type type;
  union {
    Click click;
    int sleep_ms;
  };

  PlayOperation() {}
};

class IPlayer {
public:
  virtual ~IPlayer() {}

  IPlayer(const std::string &name) { name_ = name; }

  virtual bool Init() { return true; };

  /// @brief
  virtual std::vector<PlayOperation> Play(
      const std::vector<Element> &elements) = 0;

  /// @brief Get player name
  virtual std::string Name() { return name_; }

  virtual bool SetMode(const std::string &mode_name) { return false; }

  virtual std::string GetMode() { return ""; }

  virtual std::vector<std::string> Modes() { return {}; }

private:
  std::string name_;
};
