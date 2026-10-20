#include "dtc.h"

namespace uds {

std::string decodeDtc(const std::vector<uint8_t>& b) {
    if (b.size() < 3) {
        return "?";
    }
    static const char prefix[4] = {'P', 'C', 'B', 'U'};
    const uint8_t p = (b[0] >> 6) & 0x03;
    char out[8];
    out[0] = prefix[p];
    out[1] = static_cast<char>('0' + ((b[0] >> 4) & 0x03));
    out[2] = static_cast<char>('0' + (b[0] & 0x0F));
    out[3] = static_cast<char>('0' + ((b[1] >> 4) & 0x0F));
    out[4] = static_cast<char>('0' + (b[1] & 0x0F));
    const uint8_t d5 = (b[2] >> 4) & 0x0F;
    const uint8_t d6 = b[2] & 0x0F;
    int n = 5;
    if (d5 != 0 || d6 != 0) {
        out[5] = static_cast<char>('0' + d5);
        out[6] = static_cast<char>('0' + d6);
        n = 7;
    }
    out[n] = '\0';
    return std::string(out);
}

std::vector<std::string> decodeStatus(uint8_t status) {
    std::vector<std::string> bits;
    const struct { uint8_t mask; const char* name; } table[] = {
        {0x01, "testFailed"},
        {0x02, "testFailedThisOperationCycle"},
        {0x04, "pendingDTC"},
        {0x08, "confirmedDTC"},
        {0x10, "testNotCompletedSinceLastClear"},
        {0x20, "testFailedSinceLastClear"},
        {0x40, "testNotCompletedThisOperationCycle"},
        {0x80, "warningIndicatorRequested"},
    };
    for (const auto& e : table) {
        if (status & e.mask) {
            bits.emplace_back(e.name);
        }
    }
    return bits;
}

bool parseDtcReport(const std::vector<uint8_t>& msg,
                    std::vector<DtcRecord>& outDtc,
                    std::string& err) {
    outDtc.clear();
    if (msg.size() < 6) {
        err = "message too short for a 19 02 response";
        return false;
    }
    if (msg[0] != 0x59) {
        err = "not a ReadDTCInformation response (expected SID 0x59)";
        return false;
    }
    if (msg[1] != 0x02) {
        err = "only subFunction 0x02 (reportDTCByStatusMask) is supported";
        return false;
    }
    const uint8_t formatId = msg[3];
    if (formatId != 0x01) {
        err = "unsupported DTC format identifier (only 0x01 ISO15031-6 is supported)";
        return false;
    }
    const uint16_t dtcCount = static_cast<uint16_t>((msg[4] << 8) | msg[5]);
    size_t idx = 6;
    for (uint16_t i = 0; i < dtcCount; ++i) {
        if (idx + 4 > msg.size()) {
            err = "truncated DTC list";
            return false;
        }
        DtcRecord rec;
        rec.code = decodeDtc({msg[idx], msg[idx + 1], msg[idx + 2]});
        rec.status = msg[idx + 3];
        rec.statusBits = decodeStatus(rec.status);
        outDtc.push_back(rec);
        idx += 4;
    }
    return true;
}

} // namespace uds
