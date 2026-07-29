# StreamParserDemo

基于 ASIO 的流式数据解析框架，支持串口、TCP、UDP 多种通信方式。

## 项目概述

StreamParserDemo 是一个轻量级的流数据解析框架，封装了 ASIO 网络库，提供统一的异步 I/O 接口。框架采用**适配器模式**，将底层通信细节与上层业务逻辑解耦，用户只需关注协议解析本身。

### 核心特性

- **统一接口**：所有适配器（串口/TCP/UDP）提供一致的 API 设计
- **异步 I/O**：基于 ASIO 的异步读写，不阻塞调用线程
- **帧解析**：内置状态机支持多种协议的帧边界检测
- **超时检测**：周期性检查帧接收超时
- **线程安全**：写操作通过 `asio::post` 转发到 io_context 线程

## 目录结构

```
StreamParserDemo/
├── inc/                          # 头文件目录
│   ├── StreamParser.h           # 解析器基类
│   ├── A55A55AAParser.h         # A5 5A ... 55 AA 协议解析器
│   ├── SerialParserAdapter.h    # 串口适配器
│   ├── SerialTypes.h            # 串口相关类型定义
│   ├── TcpClientParserAdapter.h # TCP 客户端适配器
│   ├── TcpSessionParserAdapter.h# TCP 会话适配器
│   ├── TcpServer.h             # TCP 服务器
│   ├── TcpTypes.h              # TCP 相关类型定义
│   ├── UdpParserAdapter.h       # UDP 适配器
│   └── UdpTypes.h              # UDP 相关类型定义
├── serial_main.cc               # 串口通信示例
├── tcp_server_main.cc           # TCP 服务器示例
├── tcp_client_main.cc           # TCP 客户端示例
└── udp_main.cc                  # UDP 通信示例
```

## 核心组件

### 1. StreamParser（解析器基类）

所有协议解析器的基类，管理帧解析的通用逻辑：

```cpp
class StreamParser {
public:
    // 设置帧完成回调
    void setFrameCallback(FrameCallback cb);
    // 设置错误回调
    void setErrorCallback(ErrorCallback cb);
    // 喂入数据（自动处理粘包/拆包）
    void feed(const uint8_t* data, size_t size);
    // 周期性调用检测超时（建议 20~50ms）
    void checkTimeout();
    // 重置解析器状态
    void reset();
    // 构建发送帧
    virtual std::vector<uint8_t> buildFrame(const std::vector<uint8_t>& payload) = 0;
};
```

### 2. A55A55AAParser（协议解析器）

实现 A5 5A ... 55 AA 格式的协议解析：

```
帧格式：
| 字段      | 长度 | 说明                    |
|-----------|------|------------------------|
| 帧头      | 2    | 固定 0xA5 0x5A         |
| 帧长度    | 2    | 大端序，为 payload + 校验和 + 尾部长度 |
| 载荷      | N    | 可变长度                |
| 校验和    | 1    | 帧头 + 长度 + 载荷的异或校验 |
| 帧尾      | 2    | 固定 0x55 0xAA          |
```

### 3. 适配器层

| 适配器 | 说明 |
|--------|------|
| SerialParserAdapter | 串口通信，支持波特率、数据位、校验位等配置 |
| TcpClientParserAdapter | TCP 客户端，支持连接/断开回调、keepalive |
| TcpSessionParserAdapter | TCP 会话，配合 TcpServer 管理多连接 |
| UdpParserAdapter | UDP 通信，支持绑定本地端口和发送目标端口 |

## 快速开始

### 编译项目

```bash
# 使用 g++ 编译（Linux/macOS）
g++ -std=c++14 -I./inc -I./3rdparty/asio/include \
    serial_main.cc -o SerialDemo -lpthread

# 使用 CMake（推荐）
mkdir build && cd build
cmake ..
make
```

### 运行示例

#### 串口通信

```bash
# 使用默认配置（COM1, 115200）
./SerialDemo

# 指定端口和波特率
./SerialDemo COM3 9600
./SerialDemo /dev/ttyUSB0 115200
```

#### TCP 服务器

```bash
# 默认监听 9000 端口
./TcpServerDemo

# 指定端口
./TcpServerDemo 8080
```

#### TCP 客户端

```bash
# 连接默认服务器
./TcpClientDemo

# 指定服务器地址
./TcpClientDemo 127.0.0.1 8080
```

#### UDP 通信

```bash
# 仅监听本地端口
./UdpDemo 8000

# 监听并发送到远程端点
./UdpDemo 8000 127.0.0.1 9000
```

## 使用指南

### 基本使用流程

1. **创建 io_context**

```cpp
asio::io_context io;
```

2. **创建协议解析器**

```cpp
auto parser = std::make_unique<A55A55AAParser>(tag, 300, 2048);
parser->setFrameCallback([](const std::string& tag, const uint8_t* data, size_t len) {
    // 处理接收到的完整帧
});
parser->setErrorCallback([](const std::string& tag, StreamParser::ErrorCode code, 
                            const std::string& detail) {
    // 处理解析错误
});
```

3. **创建适配器**

```cpp
SerialConfig config;
config.baud_rate = 115200;
config.parity = SerialConfig::Parity::None;

auto adapter = std::make_unique<SerialParserAdapter>(io, port_name, config, std::move(parser));
```

4. **异步发送数据**

```cpp
std::vector<uint8_t> payload = {0x01, 0x02, 0x03};
adapter->build_and_async_write(payload, [](SerialWriteResult r, size_t n,
                                          const asio::error_code& ec,
                                          const std::string& detail) {
    if (r == SerialWriteResult::Ok) {
        std::cout << "发送成功: " << n << " 字节" << std::endl;
    }
});
```

5. **启动事件循环**

```cpp
io.run();
```

### 自定义协议解析器

继承 `StreamParser` 并实现 `processByte` 方法：

```cpp
class MyParser : public StreamParser {
protected:
    void processByte(uint8_t byte) override {
        switch (m_state) {
        case 0:  // 等待帧头
            if (byte == 0xAA) {
                m_buf.clear();
                m_buf.push_back(byte);
                m_state = 1;
            }
            break;
        case 1:  // 接收数据
            m_buf.push_back(byte);
            if (m_buf.size() >= expected_length) {
                completeFrame();  // 通知上层
            }
            break;
        }
    }
};
```

## 错误处理

### 帧解析错误码

| 错误码 | 说明 |
|--------|------|
| Timeout | 帧接收超时 |
| InvalidHeader | 无效的帧头 |
| InvalidTailer | 无效的帧尾 |
| ChecksumFail | 校验和不匹配 |
| BufferOverflow | 缓冲区溢出 |
| ProtocolError | 其他协议错误 |

### 写操作结果码

| 结果码 | 说明 |
|--------|------|
| Ok | 发送成功 |
| PortClosed | 端口未打开 |
| Cancelled | 操作被取消 |
| IOError | I/O 错误 |
| InvalidArg | 无效参数 |

## 依赖

- C++14 编译器
- ASIO（已包含在 3rdparty 目录中）

## 许可证

本项目仅供学习和参考使用。
