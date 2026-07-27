#include <string>
#include <thread>
#include <chrono>
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

        // 3. Inject configuration + parser
        SerialParserAdapter adapter(io, port_name, config, std::move(parser));

        // To switch protocol, just change this line:
        // auto parser = std::make_unique<ModbusRtuParser>("COM3", 500, 4096);
        // SerialParserAdapter adapter(io, "COM3", config, std::move(parser));

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
