#include "dtc.h"
#include "isotp.h"
#include "uds.h"

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace uds;

namespace {

void printUsage(const char* prog) {
    std::cout
        << "UDS/ISO-TP 诊断报文解析工具\n\n"
        << "用法:\n"
        << "  " << prog << " --uds \"22 F1 90\"\n"
        << "      直接解析一条 UDS 报文（已完成 ISO-TP 重组）\n\n"
        << "  " << prog << " --can \"10 0E 22 F1 90 01 02 03 21 04 05 06 07 08 09 0A 22 0B 0C 00 00 00 00 00\"\n"
        << "      输入经典 CAN 帧（每帧 8 字节，可连续拼接），先做 ISO-TP 重组再解析\n\n"
        << "  " << prog << " --dtc \"59 02 FF 01 00 02 04 20 00 2F C1 21 00 2B\"\n"
        << "      解析 0x19 0x02 (reportDTCByStatusMask) 响应并列出故障码\n\n"
        << "字节之间用空格分隔，可带 0x 前缀。\n";
}

bool parseHexBytes(const std::string& text, std::vector<uint8_t>& out) {
    out.clear();
    std::string joined;
    for (char c : text) {
        if (c != ' ') {
            joined += c;
        }
    }

    std::string cleaned;
    for (size_t i = 0; i < joined.size();) {
        if (i + 1 < joined.size() && joined[i] == '0' &&
            (joined[i + 1] == 'x' || joined[i + 1] == 'X')) {
            i += 2;
            continue;
        }
        cleaned += joined[i++];
    }

    if (cleaned.empty() || cleaned.size() % 2 != 0) {
        return false;
    }
    for (size_t i = 0; i < cleaned.size(); i += 2) {
        const std::string byteStr = cleaned.substr(i, 2);
        char* end = nullptr;
        const long v = std::strtol(byteStr.c_str(), &end, 16);
        if (end != byteStr.c_str() + 2 || v < 0 || v > 255) {
            return false;
        }
        out.push_back(static_cast<uint8_t>(v));
    }
    return true;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        printUsage(argv[0]);
        return 1;
    }

    const std::string mode = argv[1];
    const std::string hexText = argv[2];
    std::vector<uint8_t> bytes;
    if (!parseHexBytes(hexText, bytes)) {
        std::cerr << "错误: 无效的十六进制输入\n";
        return 1;
    }

    if (mode == "--uds") {
        const UdsMessage msg = parseUds(bytes);
        std::cout << describeUds(msg) << "\n";
        return 0;
    }

    if (mode == "--dtc") {
        std::vector<DtcRecord> dtcs;
        std::string err;
        if (!parseDtcReport(bytes, dtcs, err)) {
            std::cerr << "DTC 解析失败: " << err << "\n";
            return 1;
        }
        std::cout << "DTC 数量: " << dtcs.size() << "\n";
        for (const auto& d : dtcs) {
            std::cout << "  " << d.code << "  status=0x"
                      << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                      << static_cast<int>(d.status);
            if (!d.statusBits.empty()) {
                std::cout << "  [";
                for (size_t i = 0; i < d.statusBits.size(); ++i) {
                    if (i) std::cout << ", ";
                    std::cout << d.statusBits[i];
                }
                std::cout << "]";
            }
            std::cout << "\n";
        }
        return 0;
    }

    if (mode == "--can") {
        if (bytes.size() % 8 != 0) {
            std::cerr << "错误: --can 模式要求字节数是 8 的倍数（每帧 8 字节）\n";
            return 1;
        }
        IsoTpReassembler reassembler;
        std::vector<uint8_t> message;
        for (size_t i = 0; i < bytes.size(); i += 8) {
            std::vector<uint8_t> frame(bytes.begin() + i, bytes.begin() + i + 8);
            const IsoTpStatus st = reassembler.feed(frame, message);
            if (st == IsoTpStatus::Error) {
                std::cerr << "ISO-TP 错误: " << reassembler.lastError() << "\n";
                return 1;
            }
            if (st == IsoTpStatus::Complete) {
                std::cout << "[ISO-TP] 重组完成，报文长度 " << message.size() << " 字节\n";
                std::cout << describeUds(parseUds(message)) << "\n";
                return 0;
            }
        }
        std::cout << "[ISO-TP] 报文未完成（缺少后续连续帧）\n";
        return 0;
    }

    printUsage(argv[0]);
    return 1;
}
