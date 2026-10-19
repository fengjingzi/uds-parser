#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace uds {

enum class IsoTpStatus {
    Incomplete,
    Complete,
    Error,
};

// 将经典 CAN 帧（8 字节数据域）重组为完整的 ISO-TP（ISO 15765-2）报文
class IsoTpReassembler {
public:
    IsoTpStatus feed(const std::vector<uint8_t>& frame, std::vector<uint8_t>& outMessage);

    void reset();
    bool busy() const { return expectingContinuation_; }
    const std::string& lastError() const { return lastError_; }

private:
    enum class FrameType { Single, First, Consecutive, FlowControl, Invalid };
    FrameType classify(uint8_t pci) const;

    std::vector<uint8_t> buffer_;
    uint32_t expectedLength_ = 0;
    uint8_t expectedSequence_ = 0;
    bool expectingContinuation_ = false;
    std::string lastError_;
};

} // namespace uds
