
#pragma once

#include <array>
#include <map>
#include <vector>

float Sigmoid(float x);

std::vector<std::vector<float>>
NMS(const std::vector<std::vector<float>> &boxes, float thresh);

float CalcIoU(const std::vector<float> &lhs, const std::vector<float> &rhs);

float RectArea(const std::vector<float> &rect);

bool HandleSystemResult(int sys_ret);

std::string Base64Encode(const uint8_t *data, unsigned long data_size);

std::vector<uint8_t> Base64Decode(const std::string &data);

float CalcRadian(const std::vector<float> &p1, const std::vector<float> &o,
                 const std::vector<float> &p2);

bool ReadUtil(int fd, void *buffer, uint32_t buffer_size);
