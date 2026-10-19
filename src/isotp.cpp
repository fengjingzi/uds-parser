#include "isotp.h"

namespace uds {

IsoTpReassembler::FrameType IsoTpReassembler::classify(uint8_t pci) const {
    switch (pci >> 4) {
        case 0x0: return FrameType::Single;
        case 0x1: return FrameType::First;
        case 0x2: return FrameType::Consecutive;
        case 0x3: return FrameType::FlowControl;
        default:  return FrameType::Invalid;
    }
}

void IsoTpReassembler::reset() {
    buffer_.clear();
    expectedLength_ = 0;
    expectedSequence_ = 0;
    expectingContinuation_ = false;
    lastError_.clear();
}

IsoTpStatus IsoTpReassembler::feed(const std::vector<uint8_t>& frame,
                                   std::vector<uint8_t>& outMessage) {
    if (frame.empty()) {
        lastError_ = "empty frame";
        return IsoTpStatus::Error;
    }

    const uint8_t pci = frame[0];
    const FrameType type = classify(pci);

    switch (type) {
        case FrameType::Single: {
            if (expectingContinuation_) {
                lastError_ = "single frame received while a multi-frame message is in progress";
                reset();
                return IsoTpStatus::Error;
            }
            const uint8_t length = pci & 0x0F;
            if (length == 0 || length > 7) {
                lastError_ = "invalid single-frame length";
                return IsoTpStatus::Error;
            }
            if (frame.size() - 1 < length) {
                lastError_ = "single frame truncated";
                return IsoTpStatus::Error;
            }
            outMessage.assign(frame.begin() + 1, frame.begin() + 1 + length);
            return IsoTpStatus::Complete;
        }

        case FrameType::First: {
            if (expectingContinuation_) {
                lastError_ = "first frame received while a multi-frame message is in progress";
                reset();
                return IsoTpStatus::Error;
            }
            if (frame.size() < 2) {
                lastError_ = "first frame too short";
                return IsoTpStatus::Error;
            }
            expectedLength_ = static_cast<uint32_t>((pci & 0x0F) << 8) | frame[1];
            if (expectedLength_ < 8 || expectedLength_ > 4095) {
                lastError_ = "invalid first-frame length";
                return IsoTpStatus::Error;
            }
            buffer_.assign(frame.begin() + 2, frame.end());
            expectedSequence_ = 1;
            if (buffer_.size() >= expectedLength_) {
                outMessage.assign(buffer_.begin(), buffer_.begin() + expectedLength_);
                reset();
                return IsoTpStatus::Complete;
            }
            expectingContinuation_ = true;
            return IsoTpStatus::Incomplete;
        }

        case FrameType::Consecutive: {
            if (!expectingContinuation_) {
                lastError_ = "consecutive frame received without a preceding first frame";
                return IsoTpStatus::Error;
            }
            const uint8_t seq = pci & 0x0F;
            if (seq != expectedSequence_) {
                lastError_ = "unexpected consecutive-frame sequence number";
                reset();
                return IsoTpStatus::Error;
            }
            buffer_.insert(buffer_.end(), frame.begin() + 1, frame.end());
            expectedSequence_ = static_cast<uint8_t>((expectedSequence_ + 1) & 0x0F);
            if (buffer_.size() >= expectedLength_) {
                outMessage.assign(buffer_.begin(), buffer_.begin() + expectedLength_);
                reset();
                return IsoTpStatus::Complete;
            }
            return IsoTpStatus::Incomplete;
        }

        case FrameType::FlowControl:
            lastError_ = "flow-control frame is not expected on the receiving side";
            return IsoTpStatus::Error;

        case FrameType::Invalid:
        default:
            lastError_ = "invalid PCI byte";
            return IsoTpStatus::Error;
    }
}

} // namespace uds
