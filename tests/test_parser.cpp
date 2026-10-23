#include "dtc.h"
#include "isotp.h"
#include "uds.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace uds;

static int g_run = 0;
static int g_pass = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        ++g_run;                                                             \
        if (cond) {                                                          \
            ++g_pass;                                                        \
        } else {                                                             \
            std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);      \
        }                                                                    \
    } while (0)

static void test_isotp_single_frame() {
    IsoTpReassembler r;
    std::vector<uint8_t> msg;
    const std::vector<uint8_t> frame = {0x03, 0x22, 0xF1, 0x90, 0x00, 0x00, 0x00, 0x00};
    CHECK(r.feed(frame, msg) == IsoTpStatus::Complete);
    CHECK(msg == (std::vector<uint8_t>{0x22, 0xF1, 0x90}));
}

static void test_isotp_multi_frame() {
    IsoTpReassembler r;
    std::vector<uint8_t> msg;

    const std::vector<uint8_t> ff = {0x10, 0x0E, 0x22, 0xF1, 0x90, 0x01, 0x02, 0x03};
    CHECK(r.feed(ff, msg) == IsoTpStatus::Incomplete);

    const std::vector<uint8_t> cf1 = {0x21, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
    CHECK(r.feed(cf1, msg) == IsoTpStatus::Incomplete);

    const std::vector<uint8_t> cf2 = {0x22, 0x0B, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00};
    CHECK(r.feed(cf2, msg) == IsoTpStatus::Complete);

    CHECK(msg.size() == 14);
    CHECK(msg.front() == 0x22);
    CHECK(msg.back() == 0x0B);
}

static void test_isotp_sequence_error() {
    IsoTpReassembler r;
    std::vector<uint8_t> msg;
    const std::vector<uint8_t> cf = {0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    CHECK(r.feed(cf, msg) == IsoTpStatus::Error);
}

static void test_uds_service_names() {
    CHECK(serviceName(0x22) == "ReadDataByIdentifier (Request)");
    CHECK(serviceName(0x62) == "ReadDataByIdentifier (Response)");
}

static void test_uds_negative_response() {
    const std::vector<uint8_t> m = {0x7F, 0x22, 0x31};
    const UdsMessage msg = parseUds(m);
    CHECK(msg.isNegative);
    CHECK(msg.requestSid == 0x22);
    CHECK(msg.nrc == 0x31);
    CHECK(nrcName(0x31) == "requestOutOfRange");
}

static void test_dtc_decode() {
    CHECK(decodeDtc({0x04, 0x20, 0x00}) == "P0420");
    CHECK(decodeDtc({0xC1, 0x21, 0x00}) == "U0121");
}

static void test_status_decode() {
    const auto bits = decodeStatus(0x2F);
    CHECK(bits.size() == 5);
}

static void test_dtc_report() {
    const std::vector<uint8_t> resp = {
        0x59, 0x02, 0xFF, 0x01, 0x00, 0x02,
        0x04, 0x20, 0x00, 0x2F,
        0xC1, 0x21, 0x00, 0x2B,
    };
    std::vector<DtcRecord> dtcs;
    std::string err;
    CHECK(parseDtcReport(resp, dtcs, err));
    CHECK(dtcs.size() == 2);
    if (dtcs.size() == 2) {
        CHECK(dtcs[0].code == "P0420");
        CHECK(dtcs[0].status == 0x2F);
        CHECK(dtcs[1].code == "U0121");
    }
}

int main() {
    test_isotp_single_frame();
    test_isotp_multi_frame();
    test_isotp_sequence_error();
    test_uds_service_names();
    test_uds_negative_response();
    test_dtc_decode();
    test_status_decode();
    test_dtc_report();

    std::printf("%d/%d tests passed\n", g_pass, g_run);
    return g_pass == g_run ? 0 : 1;
}
