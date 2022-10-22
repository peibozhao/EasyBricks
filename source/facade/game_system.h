#pragma once

#include <functional>
#include "common/common.h"
#include "detect/detect.h"
#include "ocr/ocr.h"
#include "player/player.h"
#include "yaml-cpp/yaml.h"

class GameSystem {
public:
  typedef std::function<void(const std::vector<PlayOperation> &play_ops)>
      Callback;

public:
  GameSystem(const std::string &config_fname);

  bool Init();

  std::vector<std::string> Players();

  std::vector<std::string> Modes(const std::string &player = "");

  std::string Player();

  std::string Mode();

  bool SetPlayMode(const std::string &player, const std::string &mode);

  std::vector<PlayOperation> InputImage(ImageFormat format,
                                        const uint8_t *buffer,
                                        unsigned long buffer_size);

  void AsyncInputImage(ImageFormat format, const uint8_t *buffer,
                       unsigned long buffer_size, Callback callback);

  std::vector<PlayOperation> InputRawImage(ImageFormat format,
                                           unsigned int width,
                                           unsigned int height,
                                           const uint8_t *buffer,
                                           unsigned long buffer_size);

  void AsyncInputRawImage(ImageFormat format, unsigned int width,
                          unsigned int height, const uint8_t *buffer,
                          unsigned long buffer_size, Callback callback);

private:
  bool InitWithYaml(const YAML::Node &yaml);

  IPlayer *CreatePlayer(const std::string &config_fname);

  std::vector<PlayOperation> ProcessJPEG(const uint8_t *buffer,
                                         unsigned long buffer_size);

  std::vector<PlayOperation> ProcessRawImage(ImageFormat format,
                                             unsigned int width,
                                             unsigned int height,
                                             const uint8_t *buffer,
                                             unsigned long buffer_size);

  std::vector<PlayOperation> ProcessJPEG(const uint8_t *buffer,
                                         unsigned long buffer_size,
                                         unsigned int width,
                                         unsigned int height);

private:
  std::shared_ptr<IOcrDetect> ocr_;
  std::shared_ptr<IObjectDetect> detect_;
  std::vector<std::shared_ptr<IPlayer>> players_;
  std::shared_ptr<IPlayer> player_;

  std::string config_fpath_;
};
