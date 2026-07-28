/*
 * Leid 2026.7
 */
#pragma once
#ifndef _TCP_SERVER_H_
#define _TCP_SERVER_H_

#include <asio.hpp>
#include <functional>
#include <memory>
#include <iostream>
#include "TcpTypes.h"
#include "TcpSessionParserAdapter.h"

// TcpServer: TCP acceptor that listens on a port and manages incoming connections.
// Uses a SessionFactory to create per-connection TcpSessionParserAdapter instances.
class TcpServer {
public:
    TcpServer(asio::io_context& io,
              const TcpConfig& config,
              std::function<std::shared_ptr<TcpSessionParserAdapter>(asio::ip::tcp::socket)> factory)
        : io_(io)
        , acceptor_(io)
        , config_(config)
        , factory_(std::move(factory))
    {}

    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;
    TcpServer(TcpServer&&) = delete;
    TcpServer& operator=(TcpServer&&) = delete;

    void listen() {
        asio::error_code ec;
        asio::ip::address addr = asio::ip::address::from_string(config_.bind_address, ec);
        if (ec) {
            addr = asio::ip::address_v4::any();
        }
        asio::ip::tcp::endpoint endpoint(addr, config_.port);
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
        acceptor_.bind(endpoint, ec);
        if (ec) {
            std::cerr << "TcpServer bind error: " << ec.message() << std::endl;
            return;
        }
        acceptor_.listen(config_.backlog, ec);
        if (ec) {
            std::cerr << "TcpServer listen error: " << ec.message() << std::endl;
            return;
        }
        std::cout << "TcpServer listening on " << config_.bind_address
                  << ":" << config_.port << std::endl;
        do_accept();
    }

    void stop() noexcept {
        if (!acceptor_.is_open()) return;
        asio::error_code ec;
        acceptor_.cancel(ec);
        acceptor_.close(ec);
        std::cout << "TcpServer stopped." << std::endl;
    }

    ~TcpServer() {
        stop();
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            [this](const asio::error_code& ec, asio::ip::tcp::socket socket) {
                if (ec) {
                    if (ec != asio::error::operation_aborted) {
                        std::cerr << "TcpServer accept error: " << ec.message() << std::endl;
                    }
                    return;
                }
                auto adapter = factory_(std::move(socket));
                if (adapter) {
                    adapter->start();
                }
                do_accept();
            });
    }

    asio::io_context& io_;
    asio::ip::tcp::acceptor acceptor_;
    TcpConfig config_;
    std::function<std::shared_ptr<TcpSessionParserAdapter>(asio::ip::tcp::socket)> factory_;
};

#endif  // !_TCP_SERVER_H_
