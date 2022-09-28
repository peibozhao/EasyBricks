#pragma once

#include <atomic>
#include <map>
#include <mutex>
#include <optional>
#include <regex>
#include "player.h"

/// @brief Key element in one page
struct PageKeyElement {
  std::regex pattern;
  float x_min, x_max;
  float y_min, y_max;
};

/// @brief Page's input configuration
struct PageConfig {
  std::string name;  // Page name
  std::vector<PageKeyElement> key_elements;
};

/// @brief Operation's input configuration
struct OperationConfig {
  std::string type;

  std::optional<std::regex> pattern;             // Click element in page
  std::optional<std::pair<float, float>> point;  // Click point
  std::optional<int> sleep_time;                 // Sleep
};

/// @brief Mode's input configuration in one game
struct ModeConfig {
  std::string name;  // Mode name
  std::vector<std::tuple<std::regex, std::vector<OperationConfig>>>
      page_operations;  // Operations in specified page
  std::vector<OperationConfig> other_page_operations;
  std::vector<OperationConfig>
      undefined_page_operations;  // Operations in undefined page
};

/// @brief Indentify page with key elements, then perform specified operations
class CommonPlayer : public IPlayer {
public:
  CommonPlayer(const std::string &name,
               const std::vector<PageConfig> &page_configs,
               const std::vector<ModeConfig> &mode_configs);

  ~CommonPlayer() override;

  bool Init() override;

  std::vector<PlayOperation> Play(
      const std::vector<Element> &elements) override;

  bool SetMode(const std::string &mode_name) override;

  std::string GetMode() override;

  std::vector<std::string> Modes() override;

protected:
  virtual void RegisterSimilarOprations() {}

  /// @brief It is too many to list all operations in similar pages
  void RegisterSimilarOpration(const std::string &mode_name,
                               const std::string &page_name,
                               std::function<std::vector<PlayOperation>(
                                   const std::vector<Element> &elements)>
                                   func);

private:
  std::vector<PlayOperation> CreatePlayOperation(
      const std::vector<Element> &elements,
      const std::vector<OperationConfig> &operation_configs);

private:
  const std::vector<PageConfig> page_configs_;
  const std::vector<ModeConfig> mode_configs_;

  const ModeConfig *mode_;

  // (Mode, Page) -> Operations
  std::map<std::tuple<std::string, std::string>,
           std::function<std::vector<PlayOperation>(
               const std::vector<Element> &elements)>>
      special_page_operations_;
};
