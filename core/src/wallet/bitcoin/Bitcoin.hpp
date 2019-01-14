#pragma once

#include <string>
#include <chrono>
#include <vector>
#include "utils/Option.hpp"
#include "math/BigInt.h"
#include <cereal/types/vector.hpp>
#include <cereal/types/common.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/chrono.hpp>

namespace ledger {
    namespace core {
        namespace bitcoin {
            static auto optionBigIntToUint64 = [](const Option<BigInt>& bigInt) -> Option<uint64_t> { 
                return bigInt.map<uint64_t>([](const BigInt& value) { return value.toUint64(); });
            };

            static auto optionUint64ToBigInt = [](const Option<uint64_t>& value) -> Option<BigInt> {
                return value.map<BigInt>([](const uint64_t& val) { return BigInt(static_cast<unsigned long long>(val)); });
            };

            struct Block {
                std::string hash;
                uint32_t height;
                std::chrono::system_clock::time_point createdAt;

                template <class Archive>
                void load(Archive & archive) {
                    archive(hash, height, createdAt);
                }

                template <class Archive>
                void save(Archive & archive) const {
                    archive(hash, height, createdAt);
                }
            };

            struct Input {
                uint64_t index;
                Option<BigInt> value;
                Option<std::string> previousTxHash;
                Option<uint32_t> previousTxOutputIndex;
                Option<std::string> address;
                Option<std::string> signatureScript;
                Option<std::string> coinbase;
                uint32_t sequence { 0xFFFFFFFF };

                template <class Archive>
                void load(Archive & archive) {
                    Option<uint64_t> val;
                    archive(
                        index,
                        val,
                        previousTxHash,
                        previousTxOutputIndex,
                        address,
                        signatureScript,
                        coinbase,
                        sequence);
                    value = optionUint64ToBigInt(val);
                }

                template <class Archive>
                void save(Archive & archive) const {
                    archive(
                        index,
                        optionBigIntToUint64(value),
                        previousTxHash,
                        previousTxOutputIndex,
                        address,
                        signatureScript,
                        coinbase,
                        sequence);
                }
            };

            struct Output {
                uint64_t index;
                std::string transactionHash;
                BigInt value;
                Option<std::string> address;
                std::string script;
                std::string time;

                template <class Archive>
                void load(Archive & archive) {
                    uint64_t val;
                    archive(
                        index,
                        transactionHash,
                        val,
                        address,
                        script,
                        time);
                    value = BigInt(val);
                }

                template <class Archive>
                void save(Archive & archive) const {
                    archive(
                        index,
                        transactionHash,
                        value.toUint64(),
                        address,
                        script,
                        time);
                }
            };

            struct Transaction {
                uint32_t  version{1};
                std::string hash;
                std::chrono::system_clock::time_point receivedAt;
                uint64_t lockTime;
                Option<Block> block;
                std::vector<Input> inputs;
                std::vector<Output> outputs;
                Option<BigInt> fees;
                uint64_t confirmations{1};

                template<class Archive>
                void save(Archive & archive) const {
                    archive(
                        version,
                        hash,
                        receivedAt,
                        lockTime,
                        block,
                        inputs,
                        outputs,
                        optionBigIntToUint64(fees),
                        confirmations);
                }

                template<class Archive>
                void load(Archive & archive) {
                    Option<uint64_t> fee;
                    archive(
                        version,
                        hash,
                        receivedAt,
                        lockTime,
                        block,
                        inputs,
                        outputs,
                        fee,
                        confirmations);
                    fees = optionUint64ToBigInt(fee);
                }
            };
        };
    }
}
