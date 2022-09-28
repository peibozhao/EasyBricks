#include "game_system.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <exception>
#include <filesystem>
#include "common/log.h"
#include "ocr/paddle_ocr.h"
#include "player/blhx_player.h"
#include "player/common_player.h"
#include "turbojpeg.h"

static std::tuple<std::string, unsigned short> GetServerInfo(
    const YAML::Node &config_yaml) {
  std::string host_name = config_yaml["host"].as<std::string>();
  uint16_t port = config_yaml["port"].as<unsigned short>();
  struct hostent *host = gethostbyname(host_name.c_str());
  if (host == nullptr) {
    LOG(ERROR) << "Get ip failed " << host_name.c_str();
    return std::make_tuple("", port);
  }
  return std::make_tuple(inet_ntoa(*(struct in_addr *)host->h_addr_list[0]),
                         port);
}

static PageKeyElement GetKeyElementConfig(const YAML::Node &yaml_node) {
  PageKeyElement ret;
  ret.pattern = yaml_node["pattern"].as<std::string>();
  if (yaml_node["x_range"].IsDefined()) {
    ret.x_min = yaml_node["x_range"][0].as<float>();
    ret.x_max = yaml_node["x_range"][1].as<float>();
  } else {
    ret.x_min = 0.f;
    ret.x_max = 1.f;
  }
  if (yaml_node["y_range"].IsDefined()) {
    ret.y_min = yaml_node["y_range"][0].as<float>();
    ret.y_max = yaml_node["y_range"][1].as<float>();
  } else {
    ret.y_min = 0.f;
    ret.y_max = 1.f;
  }
  return ret;
}

static ModeConfig GetModeConfig(const YAML::Node &yaml_node,
                                const std::vector<ModeConfig> &mode_configs) {
  ModeConfig ret;
  ret.name = yaml_node["name"].as<std::string>();

  for (const auto &inherit_yaml : yaml_node["inherit"]) {
    std::string inherit_name = inherit_yaml.as<std::string>();
    std::for_each(mode_configs.begin(), mode_configs.end(),
                  [&ret, inherit_name](const ModeConfig &mode_config) {
                    if (mode_config.name == inherit_name) {
                      ret.page_operations.insert(
                          ret.page_operations.end(),
                          mode_config.page_operations.begin(),
                          mode_config.page_operations.end());
                    }
                  });
  }

  auto get_operation_config = [](const YAML::Node &operation_yaml) {
    OperationConfig operation_config;
    std::string operation_type = operation_yaml["type"].as<std::string>();
    operation_config.type = operation_type;
    if (operation_type == "click") {
      if (operation_yaml["pattern"].IsDefined()) {
        operation_config.pattern = operation_yaml["pattern"].as<std::string>();
      } else if (operation_yaml["point"].IsDefined()) {
        operation_config.point =
            std::make_pair(operation_yaml["point"][0].as<float>(),
                           operation_yaml["point"][1].as<float>());
      }
    } else if (operation_type == "sleep") {
      operation_config.sleep_time = operation_yaml["time"].as<int>();
    }
    return operation_config;
  };

  for (const YAML::Node &defined_page_yaml : yaml_node["page_operations"]) {
    std::string page_pattern = defined_page_yaml["page"].as<std::string>();
    std::vector<OperationConfig> operation_configs;
    for (const YAML::Node &operation_yaml : defined_page_yaml["operations"]) {
      OperationConfig operation_config = get_operation_config(operation_yaml);
      operation_configs.push_back(operation_config);
    }
    ret.page_operations.push_back(
        std::make_tuple(std::regex(page_pattern), operation_configs));
  }

  if (yaml_node["other_page_operations"].IsDefined()) {
    std::vector<OperationConfig> operation_configs;
    const YAML::Node &other_page_yaml = yaml_node["other_page_operations"];
    for (const YAML::Node &operation_yaml : other_page_yaml["operations"]) {
      OperationConfig operation_config = get_operation_config(operation_yaml);
      ret.other_page_operations.push_back(operation_config);
    }
  }

  if (yaml_node["undefined_page_operations"].IsDefined()) {
    std::vector<OperationConfig> operation_configs;
    const YAML::Node &undefined_page_yaml =
        yaml_node["undefined_page_operations"];
    for (const YAML::Node &operation_yaml : undefined_page_yaml["operations"]) {
      OperationConfig operation_config = get_operation_config(operation_yaml);
      ret.undefined_page_operations.push_back(operation_config);
    }
  }
  return ret;
}

static TJPF AdaptTJPixelFormat(ImageFormat format) {
  switch (format) {
    case ImageFormat::RGBA_8888_PACKED:
      return TJPF::TJPF_RGBA;
    case ImageFormat::RGB_888_PACKED:
      return TJPF::TJPF_RGB;
    default:
      throw std::logic_error("Unsupport image format");
  }
}

std::vector<Element> TextObjectToElement(
    const std::vector<TextBox> &text_boxes,
    const std::vector<ObjectBox> &object_boxes, unsigned int width,
    unsigned int height) {
  std::vector<Element> ret;
  std::transform(text_boxes.begin(), text_boxes.end(), std::back_inserter(ret),
                 [width, height](const TextBox &text_box) {
                   return Element(
                       text_box.text,
                       static_cast<float>(text_box.region.x) / width,
                       static_cast<float>(text_box.region.y) / height);
                 });
  std::transform(
      object_boxes.begin(), object_boxes.end(), std::back_inserter(ret),
      [width, height](const ObjectBox &object_box) {
        return Element(object_box.class_name,
                       static_cast<float>(object_box.region.x) / width,
                       static_cast<float>(object_box.region.y) / height);
      });
  return ret;
}

GameSystem::GameSystem(const std::string &fname)
    : ocr_(nullptr), detect_(nullptr), player_(nullptr) {
  config_fpath_ = std::filesystem::absolute(fname).string();
  config_fpath_ = fname;
}

bool GameSystem::Init() {
  YAML::Node config_yaml(YAML::LoadFile(config_fpath_));
  return InitWithYaml(config_yaml);
}

std::vector<std::string> GameSystem::Players() {
  std::vector<std::string> ret;
  std::for_each(players_.begin(), players_.end(),
                [&ret](std::shared_ptr<IPlayer> player) {
                  ret.push_back(player->Name());
                });
  return ret;
}

std::vector<std::string> GameSystem::Modes(const std::string &player) {
  std::shared_ptr<IPlayer> player_ptr = nullptr;
  if (player.empty()) {
    player_ptr = player_;
  } else {
    auto iter = std::find_if(
        players_.begin(), players_.end(),
        [&player](std::shared_ptr<IPlayer> p) { return p->Name() == player; });
    if (iter != players_.end()) {
      player_ptr = *iter;
    }
  }
  if (player_ptr == nullptr) {
    return {};
  }
  return player_ptr->Modes();
}

bool GameSystem::SetPlayMode(const std::string &player,
                             const std::string &mode) {
  LOG(INFO) << "SetPlayMode " << player << " " << mode;
  auto iter = std::find_if(
      players_.begin(), players_.end(),
      [&player](std::shared_ptr<IPlayer> p) { return p->Name() == player; });
  if (iter == players_.end()) {
    LOG(ERROR) << "No such player " << player;
    return false;
  }
  std::shared_ptr<IPlayer> player_ptr = *iter;
  if (!player_ptr->SetMode(mode)) {
    LOG(ERROR) << "No such mode " << mode;
    return false;
  }
  player_ = player_ptr;
  return true;
}

std::vector<PlayOperation> GameSystem::InputImage(ImageFormat format,
                                                  const uint8_t *buffer,
                                                  unsigned long buffer_size) {
  LOG(INFO) << "Input image";
  switch (format) {
    case ImageFormat::JPEG:
      return ProcessJPEG(buffer, buffer_size);
    default:
      throw std::logic_error("Unsupport ImageFormat");
  }
}

void GameSystem::AsyncInputImage(ImageFormat format, const uint8_t *buffer,
                                 unsigned long buffer_size, Callback callback) {
}

std::vector<PlayOperation> GameSystem::InputRawImage(
    ImageFormat format, unsigned int width, unsigned int height,
    const uint8_t *buffer, unsigned long buffer_size) {
  LOG(INFO) << "Input raw image. " << width << " " << height;
  return ProcessRawImage(format, width, height, buffer, buffer_size);
}

void GameSystem::AsyncInputRawImage(ImageFormat format, unsigned int width,
                                    unsigned int height, const uint8_t *buffer,
                                    unsigned long buffer_size,
                                    Callback callback) {
  throw std::logic_error("Unimplement");
}

bool GameSystem::InitWithYaml(const YAML::Node &yaml) {
  // Ocr
  const YAML::Node &ocr_yaml(yaml["ocr"]);
  if (ocr_yaml["type"].as<std::string>() == "paddleocr") {
    auto server_info = GetServerInfo(ocr_yaml);
    if (ocr_yaml["recv_timeout"].IsDefined()) {
      ocr_.reset(new PaddleOcr(std::get<0>(server_info),
                               std::get<1>(server_info),
                               ocr_yaml["recv_timeout"].as<int>()));
    } else {
      ocr_.reset(
          new PaddleOcr(std::get<0>(server_info), std::get<1>(server_info)));
    }
  }
  if (!ocr_ || !ocr_->Init()) {
    LOG(ERROR) << "ocr init failed";
    return false;
  }
  LOG(INFO) << "ocr init success";

  // Player
  const YAML::Node &player_yaml(yaml["player"]);
  std::string player_name = player_yaml["name"].as<std::string>();
  std::filesystem::path player_config_prefix_path =
      std::filesystem::path(config_fpath_).parent_path();
  for (const YAML::Node &player_config_yaml : player_yaml["configs"]) {
    std::string player_config_fname =
        player_config_prefix_path / player_config_yaml.as<std::string>();

    std::shared_ptr<IPlayer> player(CreatePlayer(player_config_fname));
    if (!player || !player->Init()) {
      LOG(ERROR) << "Player init failed. " << player_config_fname;
      return false;
    }
    players_.push_back(player);
    LOG(INFO) << "Player init success. " << player->Name();

    if (player->Name() == player_name) {
      player_ = player;
    }
  }
  if (player_ == nullptr) {
    LOG(ERROR) << "No such player. " << player_name;
    return false;
  } else if (!player_->SetMode(player_yaml["mode"].as<std::string>())) {
    LOG(ERROR) << "Player SetMode failed.";
    return false;
  }

  return true;
}

IPlayer *GameSystem::CreatePlayer(const std::string &config_fname) {
  static const auto parse_page_configs = [](const YAML::Node player_yaml) {
    std::vector<PageConfig> page_configs;
    for (const YAML::Node &page_yaml : player_yaml["pages"]) {
      PageConfig page_config;
      page_config.name = page_yaml["name"].as<std::string>();

      for (const YAML::Node &page_condition_yaml : page_yaml["conditions"]) {
        PageKeyElement key_element = GetKeyElementConfig(page_condition_yaml);
        page_config.key_elements.push_back(key_element);
      }
      page_configs.push_back(page_config);
    }
    return page_configs;
  };
  static const auto parse_mode_configs = [](const YAML::Node player_yaml) {
    std::vector<ModeConfig> mode_configs;
    for (const YAML::Node &mode_yaml : player_yaml["modes"]) {
      ModeConfig mode_config = GetModeConfig(mode_yaml, mode_configs);
      mode_configs.push_back(mode_config);
    }
    return mode_configs;
  };

  LOG(INFO) << "Load player " << config_fname;
  IPlayer *ret = nullptr;
  YAML::Node player_yaml = YAML::LoadFile(config_fname);
  std::string player_name = player_yaml["name"].as<std::string>();
  std::string player_type = player_yaml["type"].as<std::string>();
  if (player_type == "common") {
    std::vector<PageConfig> page_configs = parse_page_configs(player_yaml);
    std::vector<ModeConfig> mode_configs = parse_mode_configs(player_yaml);
    ret = new CommonPlayer(player_name, page_configs, mode_configs);
  } else if (player_type == "blhx") {
    std::vector<PageConfig> page_configs = parse_page_configs(player_yaml);
    std::vector<ModeConfig> mode_configs = parse_mode_configs(player_yaml);
    ret = new BlhxPlayer(player_name, page_configs, mode_configs);
  }
  return ret;
}

std::vector<PlayOperation> GameSystem::ProcessJPEG(const uint8_t *buffer,
                                                   unsigned long buffer_size) {
  // Ocr detect
  std::vector<TextBox> text_boxes;
  if (ocr_) {
    TimeLog ocr_time_log("OCR");
    text_boxes = ocr_->Detect(buffer, buffer_size);
  }

  // Object detect
  std::vector<ObjectBox> obj_boxes;
  if (detect_) {
    TimeLog ocr_time_log("Detect");
    obj_boxes = detect_->Detect(buffer, buffer_size);
  }

  tjhandle tj = tjInitDecompress();
  int width, height, sample, colorspace;
  if (tjDecompressHeader3(tj, buffer, buffer_size, &width, &height, &sample,
                          &colorspace) != 0) {
    LOG(INFO) << "Image decompress header failed.";
    return {};
  }
  tjDestroy(tj);
  LOG(INFO) << "Image width " << width << " height " << height;

  // Player
  std::vector<PlayOperation> play_operations =
      player_->Play(TextObjectToElement(text_boxes, {}, width, height));
  // Transform ratio position to absolute position
  std::for_each(play_operations.begin(), play_operations.end(),
                [width, height](PlayOperation &play_op) {
                  if (play_op.type == PlayOperation::Type::SCREEN_CLICK) {
                    play_op.click.x *= width;
                    play_op.click.y *= height;
                  }
                });
  return play_operations;
}

std::vector<PlayOperation> GameSystem::ProcessRawImage(
    ImageFormat format, unsigned int width, unsigned int height,
    const uint8_t *buffer, unsigned long buffer_size) {
  tjhandle tj = tjInitCompress();
  unsigned long jpeg_size;
  unsigned char *jpeg_buffer = NULL;
  if (tjCompress2(tj, buffer, width, 0, height, AdaptTJPixelFormat(format),
                  &jpeg_buffer, &jpeg_size, TJSAMP::TJSAMP_420, 100, 0) != 0) {
    LOG(ERROR) << "Image compress failed";
    return {};
  }
  tjDestroy(tj);

  std::vector<PlayOperation> play_operations =
      ProcessJPEG(jpeg_buffer, jpeg_size, width, height);

  tjFree(jpeg_buffer);
  return play_operations;
}

std::vector<PlayOperation> GameSystem::ProcessJPEG(const uint8_t *buffer,
                                                   unsigned long buffer_size,
                                                   unsigned int width,
                                                   unsigned int height) {
  // Ocr detect
  std::vector<TextBox> text_boxes;
  if (ocr_) {
    TimeLog ocr_time_log("OCR");
    text_boxes = ocr_->Detect(buffer, buffer_size);
  }

  // Object detect
  std::vector<ObjectBox> obj_boxes;
  if (detect_) {
    TimeLog ocr_time_log("Detect");
    obj_boxes = detect_->Detect(buffer, buffer_size);
  }

  // Player
  std::vector<PlayOperation> play_operations =
      player_->Play(TextObjectToElement(text_boxes, {}, width, height));
  // Transform ratio position to absolute position
  std::for_each(play_operations.begin(), play_operations.end(),
                [width, height](PlayOperation &play_op) {
                  if (play_op.type == PlayOperation::Type::SCREEN_CLICK) {
                    play_op.click.x *= width;
                    play_op.click.y *= height;
                  }
                });

  return play_operations;
}
