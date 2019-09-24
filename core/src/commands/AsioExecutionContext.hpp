#pragma once
#include "api/ExecutionContext.hpp"
#include <asio.hpp>
#include <queue>
#include <mutex>

namespace ledger {
    namespace core {
        namespace api {
            class Runnable;
        };

    class AsioExecutionContext : public api::ExecutionContext {
    public:
        void execute(const std::shared_ptr<ledger::core::api::Runnable> & runnable) override;

        void delay(const std::shared_ptr<ledger::core::api::Runnable> & runnable, int64_t millis) override;

        void start();
        void stop();
    public:
        asio::io_service _io_service;
    private:
        std::queue<std::shared_ptr<ledger::core::api::Runnable>> _q;
        std::mutex _lock;
        bool _shouldStop{false};
        std::unique_ptr<std::thread> _executionThread;
    };

    }
}
