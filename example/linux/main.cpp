
#include <chrono>
#include <iostream>
#include <thread>
#include "common/log.h"
#include "facade/game_system.h"
#include "x11_event.h"
#include "x11_screencap.h"
#include "yaml-cpp/yaml.h"

#include <unistd.h>
#include <fstream>

int main(int argc, char *argv[]) {
  std::string config_fname = argv[1];
  YAML::Node root_yaml = YAML::LoadFile(config_fname);
  std::string cap_win = root_yaml["capture"]["window_class"].as<std::string>();
  std::string op_win = root_yaml["operation"]["window_class"].as<std::string>();

  X11ScreenCapture cap(cap_win);
  if (!cap.Init()) {
    std::cout << "X11 capture init failed" << std::endl;
    return -1;
  }
  GameSystem system(config_fname);
  if (!system.Init()) {
    std::cout << "System init failed" << std::endl;
    return -1;
  }
  X11Event event(op_win);
  if (!event.Init()) {
    std::cout << "X11 event init failed" << std::endl;
    return -1;
  }

  while (true) {
    sleep(1);

    ImageFormat format;
    unsigned int width, height;
    std::vector<uint8_t> buffer;
    cap.CaptureRawImage(format, width, height, buffer);

    std::string fname(std::to_string(width) + "x" + std::to_string(height) +
                      ".rgb");
    std::ofstream ofs(fname, std::ios::binary);
    ofs.write((char *)buffer.data(), buffer.size());
    ofs.close();

    std::vector<PlayOperation> play_ops = system.InputRawImage(
        format, width, height, buffer.data(), buffer.size());

    LOG(INFO) << "Operations size " << play_ops.size();
    for (PlayOperation play_op : play_ops) {
      switch (play_op.type) {
        case PlayOperation::Type::SCREEN_CLICK: {
          LOG(INFO) << "Click";
          event.Click(play_op.click.x, play_op.click.y);
          break;
        }
        case PlayOperation::Type::SLEEP: {
          LOG(INFO) << "Sleep" << play_ops.size();
          std::this_thread::sleep_for(
              std::chrono::milliseconds(play_op.sleep_ms));
          break;
        }
        case PlayOperation::Type::OVER: {
          LOG(INFO) << "Game over";
          return -1;
        }
      }
    }
  }

  return 0;
}
