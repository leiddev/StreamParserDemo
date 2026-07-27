#include <string>
#include <thread>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <memory>
#include "A55A55AAParser.h"
#include "SerialParserAdapter.h"


int main(int argc, char* argv[]) {
    asio::io_context io;

    // 1. Configure serial port parameters
#ifdef _WIN32
    std::string port_name = "COM1";  // Change to your serial port
#else
    std::string port_name = "/dev/ttyS1";  // Change to your serial port
#endif
    SerialConfig config;
    config.baud_rate      = 115200;
    config.character_size = 8;
    config.parity         = SerialConfig::Parity::None;
    config.stop_bits      = SerialConfig::StopBits::One;
    config.flow_control   = SerialConfig::FlowControl::None;

    try {
        // 2. Create specific protocol parser
        auto parser = std::make_unique<A55A55AAParser>(port_name, 300, 2048);

        parser->setFrameCallback(
            [](const std::string& tag, const uint8_t* data, size_t len) {
                std::cout << "[" << tag << "] Received complete frame, length: " << len << std::endl;
                // Can push frame to business queue or process directly here
            });

        parser->setErrorCallback(
            [](const std::string& tag, StreamParser::ErrorCode code, const std::string& detail) {
                std::cerr << "[" << tag << "] [ParseError] code=" << static_cast<int>(code)
                    << " detail=" << detail << std::endl;
            });

        // 3. Inject configuration + parser into adapter
        SerialParserAdapter adapter(io, port_name, config, std::move(parser));

        // 4. Test Case 1: async_write with callback
        std::vector<uint8_t> data = { 0x01, 0x02, 0x03 };
        adapter.async_write(
            std::make_shared<std::vector<uint8_t>>(std::move(data)),
            [](WriteResult r, size_t n,
               const asio::error_code& ec, const std::string& detail) {
                if (r == WriteResult::Ok) {
                    std::cout << "[async_write] sent " << n << " bytes" << std::endl;
                } else {
                    std::cerr << "[async_write] failed: " << detail << std::endl;
                }
            });

        adapter.build_and_async_write({ 0xAA, 0xBB, 0xCC },
            [](WriteResult r, size_t n,
               const asio::error_code& ec, const std::string& detail) {
                if (r == WriteResult::Ok) {
                    std::cout << "[build_and_async_write] sent " << n << " bytes" << std::endl;
                } else {
                    std::cerr << "[build_and_async_write] failed: " << detail << std::endl;
                }
            });

#ifdef __linux
        // Verify close/reopen behavior on Linux only:
        // On Windows COM ports the OS retains the handle briefly after
        // close(), causing immediate reopen to fail with access_denied.
        adapter.close();
        adapter.open();
#endif

        io.run();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}
