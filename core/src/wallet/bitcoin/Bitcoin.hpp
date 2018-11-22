#pragma once

#include <string>
#include <chrono>
#include <vector>
#include "utils/Option.hpp"
#include "math/BigInt.h"
#include <wallet/common/Block.hpp>

namespace ledger {
    namespace core {
        namespace bitcoin {
            struct Input {
                uint64_t index;
                Option<BigInt> value;
                Option<std::string> previousTxHash;
                Option<uint32_t> previousTxOutputIndex;
                Option<std::string> address;
                Option<std::string> signatureScript;
                Option<std::string> coinbase;
                uint32_t sequence;
                Input() {
                    sequence = 0xFFFFFFFF;
                };
            };

            struct Output {
                uint64_t index;
                std::string transactionHash;
                BigInt value;
                Option<std::string> address;
                std::string script;
                Output() = default;
                std::string time;
            };

            struct Transaction {
                uint32_t  version;
                std::string hash;
                std::chrono::system_clock::time_point receivedAt;
                uint64_t lockTime;
                Option<Block> block;
                std::vector<Input> inputs;
                std::vector<Output> outputs;
                Option<BigInt> fees;
                uint64_t confirmations;
                Transaction() {
                    version = 1;
                    confirmations = -1;
                }
            };
        };
    }
}
