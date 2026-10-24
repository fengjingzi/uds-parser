# UDS 诊断报文解析工具

一个使用 C++17 编写的命令行工具，用于解析汽车诊断报文（UDS over CAN）。
实现了 ISO-TP（ISO 15765-2）多帧重组、ISO 14229 服务解析以及 DTC 故障码解析。

## 功能

- **ISO-TP 多帧重组**：支持单帧（SF）、首帧（FF）+ 连续帧（CF）的拆包重组，
  校验连续帧序列号；
- **UDS 服务解析**：识别常用服务（10/14/19/22/27/3E 等），区分请求、响应与负响应；
- **DTC 故障码解析**：解析 0x19 0x02（reportDTCByStatusMask）响应，
  将 3 字节故障码转换为 P/C/B/U 标准码（如 P0420），
  并逐位解释 statusOfDTC 状态字节；
- **负响应码解释**：将 NRC 映射为可读名称（如 0x31 → requestOutOfRange）。

## 构建

依赖：CMake ≥ 3.10，支持 C++17 的编译器（GCC/Clang/MSVC）。

```bash
mkdir build
cd build
cmake ..
make
```

## 使用示例

### 1. 直接解析一条 UDS 报文

```bash
./uds_parser --uds "22 F1 90"
```

输出：

```
Service: 0x22 ReadDataByIdentifier (Request)
  Payload (2 bytes): F1 90
```

### 2. 负响应解析

```bash
./uds_parser --uds "7F 22 31"
```

输出：

```
Negative Response
  Request: 0x22 ReadDataByIdentifier (Request)
  NRC: 0x31 (requestOutOfRange)
```

### 3. ISO-TP 多帧重组

下面的例子是一条 14 字节的报文，被拆成首帧 + 2 个连续帧：

```bash
./uds_parser --can "10 0E 22 F1 90 01 02 03 21 04 05 06 07 08 09 0A 22 0B 0C 00 00 00 00 00"
```

输出：

```
[ISO-TP] 重组完成，报文长度 14 字节
Service: 0x22 ReadDataByIdentifier (Request)
  Payload (13 bytes): F1 90 01 02 03 04 05 06 07 08 09 0A 0B
```

### 4. DTC 故障码解析

```bash
./uds_parser --dtc "59 02 FF 01 00 02 04 20 00 2F C1 21 00 2B"
```

输出：

```
DTC 数量: 2
  P0420  status=0x2F  [testFailed, testFailedThisOperationCycle, pendingDTC, confirmedDTC, testFailedSinceLastClear]
  U0121  status=0x2B  [testFailed, testFailedThisOperationCycle, confirmedDTC, testFailedSinceLastClear]
```

## 协议说明

### ISO-TP（ISO 15765-2）

经典 CAN 每帧数据域只有 8 字节。ISO-TP 把超过 7 字节的诊断报文拆成多帧传输。
首字节是 PCI（Protocol Control Information），高 4 位决定帧类型：

| 帧类型 | PCI 高 4 位 | 说明 |
|---|---|---|
| 单帧 SF | 0 | 低 4 位是数据长度（1~7），适用于 ≤7 字节的报文 |
| 首帧 FF | 1 | 低 4 位 + 下一字节组成 12 位总长度（8~4095） |
| 连续帧 CF | 2 | 低 4 位是序列号（1~15，循环到 0） |
| 流控帧 FC | 3 | 接收方发送，用于控制发送节奏（本工具不处理发送侧） |

### UDS（ISO 14229）

重组的报文首字节是 SID（Service Identifier）：

- 请求 SID 范围 0x10~0x3E；
- 正响应 SID = 请求 SID + 0x40；
- 负响应 SID 固定为 0x7F，格式为 `7F <请求SID> <NRC>`。

### DTC 状态字节（statusOfDTC）

| 位 | 名称 | 含义 |
|---|---|---|
| bit0 | testFailed | 当前测试失败 |
| bit1 | testFailedThisOperationCycle | 本操作循环内测试失败 |
| bit2 | pendingDTC | 待确认故障 |
| bit3 | confirmedDTC | 已确认故障 |
| bit4 | testNotCompletedSinceLastClear | 上次清除后测试未完成 |
| bit5 | testFailedSinceLastClear | 上次清除后测试失败 |
| bit6 | testNotCompletedThisOperationCycle | 本循环测试未完成 |
| bit7 | warningIndicatorRequested | 请求点亮故障指示灯 |

## 目录结构

```
uds-parser/
├── CMakeLists.txt
├── include/
│   ├── isotp.h      # ISO-TP 重组
│   ├── uds.h        # UDS 服务解析
│   └── dtc.h        # DTC 故障码解析
├── src/
│   ├── main.cpp     # 命令行入口
│   ├── isotp.cpp
│   ├── uds.cpp
│   └── dtc.cpp
└── tests/
    └── test_parser.cpp  # 单元测试
```

## 测试

```bash
cd build
ctest --output-on-failure
```

或直接运行测试程序：

```bash
./test_parser
```

## 设计说明

- 分层设计：`isotp`（传输层）→ `uds`（服务层）→ `dtc`（应用数据），
  各层职责单一，便于扩展新的服务或 DTC 格式；
- 使用 CMake 构建、`ctest` 集成测试；
- 无第三方依赖，仅使用 C++ 标准库。
