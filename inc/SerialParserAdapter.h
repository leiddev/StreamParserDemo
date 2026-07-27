/*
 * Leid 2026.7
 */
#pragma once
#ifndef _SERIAL_PARSER_ADAPTER_H_
#define _SERIAL_PARSER_ADAPTER_H_

#include <asio.hpp>
#include <memory>
#include <array>
#include <iostream>
#include <chrono>
#include <string>

// -------------------------------------------------
// 1. Serial port configuration struct
// -------------------------------------------------
struct SerialConfig {
    unsigned int baud_rate      = 115200;
    unsigned int character_size = 8;
    
    enum class Parity { None, Odd, Even };
    Parity parity = Parity::None;

    enum class StopBits { One, OnePointFive, Two };
    StopBits stop_bits = StopBits::One;

    enum class FlowControl { None, Software, Hardware };
    FlowControl flow_control = FlowControl::None;
};

// -------------------------------------------------
// 2. SerialParserAdapter (depends only on base class StreamParser)
// -------------------------------------------------
class SerialParserAdapter {
public:
    /**
     * @param io          Asio io_context
     * @param port_name   Serial device name ("/dev/ttyUSB0" or "COM3")
     * @param config      Serial port configuration
     * @param parser      Pre-constructed parser (ownership transferred, depends only on base class)
     */
    SerialParserAdapter(asio::io_context& io,
                        const std::string& port_name,
                        const SerialConfig& config,
                        std::unique_ptr<StreamParser> parser)
        : port_(io)
        , timer_(io)
        , parser_(std::move(parser))
        , port_name_(port_name)
        , config_(config)
    {
        do_open();
    }

    // Disable copy
    SerialParserAdapter(const SerialParserAdapter&) = delete;
    SerialParserAdapter& operator=(const SerialParserAdapter&) = delete;

    // Disable move
    SerialParserAdapter(SerialParserAdapter&&) = delete;
    SerialParserAdapter& operator=(SerialParserAdapter&&) = delete;

    // Optional: modify some settings at runtime
    void set_baud_rate(unsigned int baud) {
        port_.set_option(asio::serial_port_base::baud_rate(baud));
    }

    void open() {
        do_open();
    }

    void close() noexcept {
        if (!port_.is_open())
            return;

        closing_ = true;

        asio::error_code ec;
        timer_.cancel(ec);
        port_.cancel(ec);
        port_.close(ec);

        if (parser_) {
            parser_->reset();
        }
    }

    ~SerialParserAdapter() {
        close();
    }

private:
    void do_open() {
        if (port_.is_open()) {
            asio::error_code ec;
            port_.cancel(ec);
            port_.close(ec);
        }

        port_.open(port_name_);
        apply_config(config_);
        closing_ = false;

        if (parser_) {
            parser_->reset();
        }

        start_read();
        start_timeout_timer();
    }

    void apply_config(const SerialConfig& cfg) {
        using spb = asio::serial_port_base;   // Alias for brevity

        port_.set_option(spb::baud_rate(cfg.baud_rate));
        port_.set_option(spb::character_size(cfg.character_size));

        // Parity
        switch (cfg.parity) {
            case SerialConfig::Parity::None:
                port_.set_option(spb::parity(spb::parity::none));
                break;
            case SerialConfig::Parity::Odd:
                port_.set_option(spb::parity(spb::parity::odd));
                break;
            case SerialConfig::Parity::Even:
                port_.set_option(spb::parity(spb::parity::even));
                break;
        }

        // Stop bits
        switch (cfg.stop_bits) {
            case SerialConfig::StopBits::One:
                port_.set_option(spb::stop_bits(spb::stop_bits::one));
                break;
            case SerialConfig::StopBits::OnePointFive:
                port_.set_option(spb::stop_bits(spb::stop_bits::onepointfive));
                break;
            case SerialConfig::StopBits::Two:
                port_.set_option(spb::stop_bits(spb::stop_bits::two));
                break;
        }

        // Flow control
        switch (cfg.flow_control) {
            case SerialConfig::FlowControl::None:
                port_.set_option(spb::flow_control(spb::flow_control::none));
                break;
            case SerialConfig::FlowControl::Software:
                port_.set_option(spb::flow_control(spb::flow_control::software));
                break;
            case SerialConfig::FlowControl::Hardware:
                port_.set_option(spb::flow_control(spb::flow_control::hardware));
                break;
        }
    }

    void start_read() {
        port_.async_read_some(asio::buffer(buf_),
            [this](const asio::error_code& ec, size_t bytes) {
                if (!ec) {
                    parser_->feed(buf_.data(), bytes);
                    start_read();
                } else if (ec != asio::error::operation_aborted) {
                    std::cerr << "Serial read error: " << ec.message() << std::endl;
                }
            });
    }

    void start_timeout_timer() {
        timer_.expires_after(std::chrono::milliseconds(30));  // Recommended 20~50ms
        timer_.async_wait([this](const asio::error_code& ec) {
            if (!ec && !closing_) {
                parser_->checkTimeout();
                start_timeout_timer();
            }
        });
    }

    asio::serial_port             port_;
    asio::steady_timer            timer_;
    std::unique_ptr<StreamParser> parser_;   // Only holds base class
    std::array<uint8_t, 512>      buf_;
    bool                          closing_ = false;
    std::string                   port_name_;
    SerialConfig                  config_;
};

#endif  // !_SERIAL_PARSER_ADAPTER_H_
