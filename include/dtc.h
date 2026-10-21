#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace uds {

struct DtcRecord {
    std::string code;
    uint8_t status = 0;
    std::vector<std::string> statusBits;
};

std::string decodeDtc(const std::vector<uint8_t>& dtcBytes);
std::vector<std::string> decodeStatus(uint8_t status);
bool parseDtcReport(const std::vector<uint8_t>& message,
                    std::vector<DtcRecord>& outDtc,
                    std::string& err);

} // namespace uds
