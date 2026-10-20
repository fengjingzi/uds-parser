#include "uds.h"

#include <iomanip>
#include <map>
#include <sstream>

namespace uds {

namespace {

const std::map<uint8_t, std::string>& serviceNames() {
    static const std::map<uint8_t, std::string> names = {
        {0x10, "DiagnosticSessionControl"},
        {0x11, "ECUReset"},
        {0x14, "ClearDiagnosticInformation"},
        {0x19, "ReadDTCInformation"},
        {0x22, "ReadDataByIdentifier"},
        {0x23, "ReadMemoryByAddress"},
        {0x27, "SecurityAccess"},
        {0x28, "CommunicationControl"},
        {0x2E, "WriteDataByIdentifier"},
        {0x2F, "InputOutputControlByIdentifier"},
        {0x31, "RoutineControl"},
        {0x34, "RequestDownload"},
        {0x35, "RequestUpload"},
        {0x36, "TransferData"},
        {0x37, "RequestTransferExit"},
        {0x3D, "WriteMemoryByAddress"},
        {0x3E, "TesterPresent"},
        {0x85, "ControlDTCSetting"},
        {0xA9, "Authentication"},
    };
    return names;
}

} // namespace

std::string serviceName(uint8_t sid) {
    const bool isResponse = (sid & 0x40) != 0;
    const uint8_t base = isResponse ? static_cast<uint8_t>(sid - 0x40) : sid;
    auto it = serviceNames().find(base);
    const std::string name = (it != serviceNames().end()) ? it->second : "Unknown";
    return name + (isResponse ? " (Response)" : " (Request)");
}

UdsMessage parseUds(const std::vector<uint8_t>& message) {
    UdsMessage msg;
    if (message.empty()) {
        return msg;
    }
    msg.sid = message[0];
    if (msg.sid == 0x7F && message.size() >= 3) {
        msg.isNegative = true;
        msg.isResponse = true;
        msg.requestSid = message[1];
        msg.nrc = message[2];
        msg.payload.assign(message.begin() + 3, message.end());
    } else {
        msg.isResponse = (msg.sid & 0x40) != 0;
        msg.payload.assign(message.begin() + 1, message.end());
    }
    return msg;
}

std::string nrcName(uint8_t nrc) {
    static const std::map<uint8_t, std::string> nrcs = {
        {0x10, "generalReject"},
        {0x11, "serviceNotSupported"},
        {0x12, "subFunctionNotSupported"},
        {0x13, "incorrectMessageLengthOrInvalidFormat"},
        {0x14, "responseTooLong"},
        {0x21, "busyRepeatRequest"},
        {0x22, "conditionsNotCorrect"},
        {0x24, "requestSequenceError"},
        {0x25, "noResponseFromSubnetComponent"},
        {0x26, "failurePreventsExecutionOfRequestedAction"},
        {0x31, "requestOutOfRange"},
        {0x33, "securityAccessDenied"},
        {0x35, "invalidKey"},
        {0x36, "exceedNumberOfAttempts"},
        {0x37, "requiredTimeDelayNotExpired"},
        {0x70, "uploadDownloadNotAccepted"},
        {0x71, "transferDataSuspended"},
        {0x72, "generalProgrammingFailure"},
        {0x73, "wrongBlockSequenceCounter"},
        {0x78, "requestCorrectlyReceivedResponsePending"},
        {0x7E, "subFunctionNotSupportedInActiveSession"},
        {0x7F, "serviceNotSupportedInActiveSession"},
    };
    auto it = nrcs.find(nrc);
    return (it != nrcs.end()) ? it->second : "UnknownNRC";
}

std::string describeUds(const UdsMessage& msg) {
    std::ostringstream oss;
    if (msg.isNegative) {
        oss << "Negative Response\n"
            << "  Request: 0x" << std::hex << std::uppercase << static_cast<int>(msg.requestSid)
            << " " << serviceName(msg.requestSid) << "\n"
            << "  NRC: 0x" << std::hex << std::uppercase << static_cast<int>(msg.nrc)
            << " (" << nrcName(msg.nrc) << ")";
        return oss.str();
    }
    oss << "Service: 0x" << std::hex << std::uppercase << static_cast<int>(msg.sid)
        << " " << serviceName(msg.sid);
    if (!msg.payload.empty()) {
        oss << "\n  Payload (" << std::dec << msg.payload.size() << " bytes):";
        for (uint8_t b : msg.payload) {
            oss << " " << std::hex << std::setw(2) << std::setfill('0') << std::uppercase
                << static_cast<int>(b);
        }
    }
    return oss.str();
}

} // namespace uds
