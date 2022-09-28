#pragma once

#include <chrono>
#include <iostream>
#include <sstream>

#ifdef __ANDROID__
#include <android/log.h>

class Logger {
public:
  Logger() {}

  ~Logger() {
    __android_log_print(ANDROID_LOG_INFO, "EasyBricks", "%s\n",
                        ss.str().c_str());
  }

  std::ostream &stream() { return ss; }

private:
  std::stringstream ss;
};

#define LOG(...) Logger().stream()
#define DLOG(...) Logger().stream()

#else
#include "glog/logging.h"
#endif

/// @brief Print cost time
class TimeLog {
public:
  TimeLog(const std::string &label = "") {
    label_ = label;
    printed_ = false;
    start_time_ = std::chrono::system_clock::now();
  }

  ~TimeLog() {
    if (!printed_) Tok();
  }

  void Tok() {
    auto end_time = std::chrono::system_clock::now();
    DLOG(INFO) << label_ << " cost "
               << std::chrono::duration_cast<std::chrono::milliseconds>(
                      end_time - start_time_)
                      .count();
    printed_ = true;
  }

private:
  std::chrono::system_clock::time_point start_time_;
  std::string label_;
  bool printed_;
};
