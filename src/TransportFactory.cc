/*
 * TransportFactory.cc - Transport facade entry point.
 *
 * Leid 2026.10
 */
#include <memory>
#include <string>
#include <utility>

#include "A55A55AAParser.h"
#include "StreamParserLog.h"
#include "TransportImpl.h"

namespace sp {

ParserFactory DefaultParserFactory(uint32_t frame_timeout_ms, std::size_t max_frame_bytes) {
    return [frame_timeout_ms, max_frame_bytes](const std::string& tag) {
        return std::unique_ptr<StreamParser>(
            new A55A55AAParser(tag, frame_timeout_ms, max_frame_bytes));
    };
}

std::unique_ptr<Transport> CreateTransport(const TransportConfig& cfg,
                                           ParserFactory          parser_factory,
                                           TransportHandler       handler) {
    switch (cfg.kind) {
        case TransportKind::Serial:
            return MakeSerialTransport(cfg, std::move(parser_factory), std::move(handler));
        case TransportKind::TcpServer:
            return MakeTcpServerTransport(cfg, std::move(parser_factory), std::move(handler));
        case TransportKind::TcpClient:
            return MakeTcpClientTransport(cfg, std::move(parser_factory), std::move(handler));
        case TransportKind::Udp:
            return MakeUdpTransport(cfg, std::move(parser_factory), std::move(handler));
    }
    SP_LOG_ERROR("transport", "unknown TransportKind " << static_cast<int>(cfg.kind));
    return nullptr;
}

}  // namespace sp
