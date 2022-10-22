
#include "common_player.h"
#include <array>
#include <cassert>
#include "common/log.h"

typedef std::pair<float, float> Point;
typedef std::array<float, 4> Region;

static bool CenterInRegion(const Region &region, const Point &point) {
  return point.first >= region[0] && point.first <= region[2] &&
         point.second >= region[1] && point.second <= region[3];
}

static bool SatisfyPageCondition(
    const std::vector<Element> &elements,
    const std::vector<PageKeyElement> &key_elements) {
  for (const PageKeyElement &key_element : key_elements) {
    bool has_cur = false;
    for (const Element &element : elements) {
      if (std::regex_match(element.name, key_element.pattern) &&
          CenterInRegion({key_element.x_min, key_element.y_min,
                          key_element.x_max, key_element.y_max},
                         {element.x, element.y})) {
        has_cur = true;
        break;
      }
    }
    if (!has_cur) {
      return false;
    }
  }
  return true;
}

static std::tuple<std::string, float, float> GetPatternPoint(
    const std::vector<Element> &elements, const std::regex &pattern) {
  for (const auto element : elements) {
    if (std::regex_match(element.name, pattern)) {
      return std::make_tuple(element.name, element.x, element.y);
    }
  }
  return std::make_tuple("", 0., 0.);
}

CommonPlayer::CommonPlayer(const std::string &name,
                           const std::vector<PageConfig> &page_configs,
                           const std::vector<ModeConfig> &mode_configs)
    : IPlayer(name),
      page_configs_(page_configs),
      mode_configs_(mode_configs),
      mode_(nullptr) {}

CommonPlayer::~CommonPlayer() {}

bool CommonPlayer::Init() {
  DLOG(INFO) << "Player init. " << Name();
  if (mode_configs_.empty()) {
    LOG(ERROR) << "Mode is empty";
    return false;
  }
  mode_ = &mode_configs_.back();
  for (const ModeConfig &mode_config : mode_configs_) {
    DLOG(INFO) << "Mode: " << mode_config.name;
    for (const auto &page_operation : mode_config.page_operations) {
      bool has_page = false;
      for (const PageConfig &page_config : page_configs_) {
        if (std::regex_match(page_config.name, std::get<0>(page_operation))) {
          DLOG(INFO) << "Page: " << page_config.name;
          has_page = true;
        }
      }
      if (!has_page) {
        LOG(ERROR) << "Mode has unmatched page pattern. " << mode_config.name;
        return false;
      }
    }
  }
  RegisterSimilarOprations();
  return true;
}

std::vector<PlayOperation> CommonPlayer::Play(
    const std::vector<Element> &elements) {
  for (const PageConfig &page_config : page_configs_) {
    // Detect page
    if (!SatisfyPageCondition(elements, page_config.key_elements)) {
      continue;
    }
    DLOG(INFO) << "Detect page " << page_config.name;

    std::vector<PlayOperation> ret;
    // Special page
    auto special_page_action_iter = special_page_operations_.find(
        std::make_tuple(mode_->name, page_config.name));
    if (special_page_action_iter != special_page_operations_.end()) {
      DLOG(INFO) << "Special page: "
                 << std::get<0>(special_page_action_iter->first) << " "
                 << std::get<1>(special_page_action_iter->first);
      ret = special_page_action_iter->second(elements);
    }

    // Return page actions
    auto iter = mode_->page_operations.begin();
    for (; iter != mode_->page_operations.end(); ++iter) {
      if (std::regex_match(page_config.name, std::get<0>(*iter))) {
        break;
      }
    }

    std::vector<PlayOperation> page_operations;
    if (iter != mode_->page_operations.end()) {
      page_operations = CreatePlayOperation(elements, std::get<1>(*iter));
    } else {
      DLOG(INFO) << "Other page operation";
      page_operations =
          CreatePlayOperation(elements, mode_->other_page_operations);
    }
    ret.insert(ret.end(), page_operations.begin(), page_operations.end());
    return ret;
  }

  // Page is not defined
  DLOG(INFO) << "Undefined page operation";
  return CreatePlayOperation(elements, mode_->undefined_page_operations);
}

bool CommonPlayer::SetMode(const std::string &mode_name) {
  for (const auto &mode_config : mode_configs_) {
    if (mode_config.name == mode_name) {
      mode_ = &mode_config;
      LOG(INFO) << "Player mode " << mode_->name;
      return true;
    }
  }
  LOG(ERROR) << "Unsupport mode " << mode_name;
  return false;
}

std::string CommonPlayer::Mode() { return mode_->name; }

std::vector<std::string> CommonPlayer::Modes() {
  std::vector<std::string> ret;
  std::for_each(mode_configs_.begin(), mode_configs_.end(),
                [&ret](const ModeConfig &mode_config) {
                  ret.push_back(mode_config.name);
                });
  return ret;
}

void CommonPlayer::RegisterSimilarOpration(
    const std::string &mode_name, const std::string &page_name,
    std::function<
        std::vector<PlayOperation>(const std::vector<Element> &elements)>
        func) {
  LOG(INFO) << "Register special page: " << mode_name << " " << page_name;
  special_page_operations_[std::make_tuple(mode_name, page_name)] = func;
}

std::vector<PlayOperation> CommonPlayer::CreatePlayOperation(
    const std::vector<Element> &elements,
    const std::vector<OperationConfig> &operation_configs) {
  std::vector<PlayOperation> ret;
  for (const OperationConfig &operation_config : operation_configs) {
    PlayOperation operation;
    if (operation_config.type == "click") {
      operation.type = PlayOperation::Type::SCREEN_CLICK;
      if (operation_config.pattern) {
        auto point_info = GetPatternPoint(elements, *operation_config.pattern);
        std::string click_text = std::get<0>(point_info);
        if (click_text.empty()) {
          LOG(WARNING) << "Can not find click pattern";
          continue;
        }
        operation.click.x = std::get<1>(point_info);
        operation.click.y = std::get<2>(point_info);
        DLOG(INFO) << "Click " << std::get<0>(point_info);
      } else if (operation_config.point) {
        operation.click.x = operation_config.point->first;
        operation.click.y = operation_config.point->second;
        DLOG(INFO) << "Click point " << operation.click.x << " "
                   << operation.click.y;
      }
    } else if (operation_config.type == "sleep") {
      DLOG(INFO) << "Sleep " << *operation_config.sleep_time;
      operation.type = PlayOperation::Type::SLEEP;
      operation.sleep_ms = *operation_config.sleep_time;
    } else if (operation_config.type == "over") {
      operation.type = PlayOperation::Type::OVER;
      LOG(INFO) << "Play end";
    } else {
      continue;
    }
    ret.push_back(operation);
  }
  return ret;
}
