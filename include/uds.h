#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace uds {

struct UdsMessage {
    uint8_t sid = 0;
    bool isResponse = false;
    bool isNegative = false;
    uint8_t requestSid = 0;
    uint8_t nrc = 0;
    std::vector<uint8_t> payload;
};

UdsMessage parseUds(const std::vector<uint8_t>& message);
std::string serviceName(uint8_t sid);
std::string describeUds(const UdsMessage& msg);
std::string nrcName(uint8_t nrc);

} // namespace uds
