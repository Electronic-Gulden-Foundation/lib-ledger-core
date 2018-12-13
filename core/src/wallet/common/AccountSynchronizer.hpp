#pragma once
#include <wallet/NetworkTypes.hpp>
#include <wallet/AccountSynchronizer.hpp>
#include <async/FutureUtils.hpp>

namespace spdlog {
    class logger;
}

namespace ledger {
    namespace core {
        class Keychain;

        template<typename T>
        class BlockchainDatabase;

        template<typename Network>
        class ExplorerV2;

        namespace api {
            class ExecutionContext;
        };
        namespace common {
            template<typename T>
            class BlocksSynchronizer;

            struct SynchronizerConfiguration {
                SynchronizerConfiguration(
                    uint32_t _maxPossibleUnstableBlocks,
                    uint32_t _maxNumberOfAddressesInRequest,
                    uint32_t _discoveryGapSize,
                    uint32_t _maxTransactionPerResponce,
                    const std::string& _genesisBlockHash) 
                    : maxPossibleUnstableBlocks(_maxPossibleUnstableBlocks)
                    , maxNumberOfAddressesInRequest(_maxNumberOfAddressesInRequest)
                    , discoveryGapSize(_discoveryGapSize)
                    , maxTransactionPerResponce(_maxTransactionPerResponce)
                    , genesisBlockHash(_genesisBlockHash) {
                }
                const uint32_t maxPossibleUnstableBlocks;
                const uint32_t maxNumberOfAddressesInRequest; // (batch size)
                const uint32_t discoveryGapSize; // address discovery
                const uint32_t maxTransactionPerResponce; // ignored in explorer v2
                const std::string genesisBlockHash; // hash of genesis block
            };

            struct HashHeight {
                std::string hash;
                uint32_t height;
            };

            template<typename NetworkType>
            class AccountSynchronizer :
                public core::AccountSynchronizer<NetworkType>,
                public std::enable_shared_from_this<AccountSynchronizer<NetworkType>> {
            public:
                typedef typename NetworkType::Block Block;

                AccountSynchronizer(
                    const std::shared_ptr<api::ExecutionContext>& executionContext,
                    const std::shared_ptr<ExplorerV2<NetworkType>>& explorer,
                    const std::shared_ptr<BlockchainDatabase<NetworkType>>& stableBlocksDb,
                    const std::shared_ptr<BlockchainDatabase<NetworkType>>& unstableBlocksDb,
                    const std::shared_ptr<BlockchainDatabase<NetworkType>>& pendingTransactions,
                    const std::shared_ptr<Keychain>& receiveKeychain,
                    const std::shared_ptr<Keychain>& changeKeychain,
                    const std::shared_ptr<spdlog::logger>& logger,
                    const SynchronizerConfiguration& synchronizerConfig
                );

                std::shared_ptr<ProgressNotifier<Unit>> synchronize() override;

                bool isSynchronizing() const;

                Future<Unit> synchronizeBlocks();
                Future<Unit> synchronizePendingTransactions();

            private:
                std::shared_ptr<api::ExecutionContext> _executionContext;
                std::shared_ptr<ExplorerV2<NetworkType>> _explorer;
                // blocks that would not be reverted
                std::shared_ptr<BlockchainDatabase<NetworkType>> _stableBlocksDb;
                // blocks that may be reverted
                std::shared_ptr<BlockchainDatabase<NetworkType>> _unstableBlocksDb;
                std::shared_ptr<BlockchainDatabase<NetworkType>> _pendingTransactionsDb;
                std::shared_ptr<Keychain> _receiveKeychain;
                std::shared_ptr<Keychain> _changeKeychain;
                std::shared_ptr<spdlog::logger> _logger;
                std::shared_ptr<ProgressNotifier<Unit>> _notifier;
                std::shared_ptr<BlocksSynchronizer<NetworkType>> _stableBlocksSynchronizer;
                std::shared_ptr<BlocksSynchronizer<NetworkType>> _unstableBlocksSynchronizer;
                
                const SynchronizerConfiguration _config;

                mutable std::recursive_mutex _lock; // recursive in case somebody will use ImmediateExecutionContext 
            };
        }
    }
}

extern template ledger::core::common::AccountSynchronizer<ledger::core::BitcoinLikeNetwork>;
extern template std::shared_ptr<ledger::core::common::AccountSynchronizer<ledger::core::BitcoinLikeNetwork>>;