/*
 *
 * BitcoinLikeWalletFactory
 * ledger-core
 *
 * Created by Pierre Pollastri on 31/01/2017.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Ledger
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */
#include "BitcoinLikeWalletFactory.hpp"
#include "../networks.hpp"
#include <leveldb/db.h>

#include <api/BlockchainExplorerEngines.hpp>
#include <api/BlockchainObserverEngines.hpp>
#include <api/ConfigurationDefaults.hpp>
#include <api/KeychainEngines.hpp>
#include <api/SynchronizationEngines.hpp>

#include <wallet/currencies.hpp>

#include <wallet/common/ExplorerTransactionBroadcaster.hpp>
#include <wallet/bitcoin/BitcoinLikeWallet.hpp>
#include <wallet/bitcoin/explorers/BitcoinLikeExplorer.hpp>
#include <wallet/bitcoin/factories/keystores/BitcoinLikeP2PKHKeychainFactory.h>
#include <wallet/bitcoin/factories/keystores/BitcoinLikeP2SHKeychainFactory.h>
#include <wallet/bitcoin/factories/AccountSynchronizerFactory.hpp>
#include <wallet/bitcoin/observers/LedgerApiBitcoinLikeBlockchainObserver.h>
#include <wallet/pool/WalletPool.hpp>

#define STRING(key, def) entry.configuration->getString(key).value_or(def)

namespace ledger {
    namespace core {
        namespace bitcoin {
            std::shared_ptr<AbstractWallet> BitcoinLikeWalletFactory::build(const WalletDatabaseEntry &entry) {
                auto pool = getPool();
                pool->logger()->info("Building wallet instance '{}' for {} with parameters: {}", entry.name, entry.currencyName, entry.configuration->dump());
                // Get currency
                auto currency = getPool()->getCurrency(entry.currencyName);
                if (currency.isEmpty())
                    throw make_exception(api::ErrorCode::UNSUPPORTED_CURRENCY, "Unsupported currency '{}'.", entry.currencyName);
                // Configure keychain
                auto keychainFactory = _keychainFactories.find(STRING(api::Configuration::KEYCHAIN_ENGINE, api::KeychainEngines::BIP32_P2PKH));
                if (keychainFactory == _keychainFactories.end()) {
                    throw make_exception(api::ErrorCode::UNKNOWN_KEYCHAIN_ENGINE, "Engine '{}' is not a supported keychain engine.", STRING(api::Configuration::KEYCHAIN_ENGINE, "undefined"));
                }
                // Configure explorer
                auto explorer = getExplorer(entry.configuration);
                if (explorer == nullptr)
                    throw make_exception(api::ErrorCode::UNKNOWN_BLOCKCHAIN_EXPLORER_ENGINE, "Engine '{}' is not a supported explorer engine.", STRING(api::Configuration::BLOCKCHAIN_EXPLORER_ENGINE, "undefined"));
                // Configure observer
                auto observer = getObserver(entry.currencyName, entry.configuration);
                if (observer == nullptr)
                    pool->logger()->warn("Observer engine '{}' is not supported. Wallet {} was created anyway. Real time events won't be handled by this instance.", STRING(api::Configuration::BLOCKCHAIN_OBSERVER_ENGINE, "undefined"), entry.name);
                // Configure synchronizer
                auto engine = entry.configuration->getString(api::Configuration::SYNCHRONIZATION_ENGINE)
                    .value_or(api::SynchronizationEngines::BLOCKCHAIN_EXPLORER_SYNCHRONIZATION);
                if (engine != api::SynchronizationEngines::BLOCKCHAIN_EXPLORER_SYNCHRONIZATION) {
                    throw make_exception(api::ErrorCode::UNKNOWN_SYNCHRONIZATION_ENGINE, "Engine '{}' is not a supported synchronization engine.", STRING(api::Configuration::SYNCHRONIZATION_ENGINE, "undefined"));
                }

                // TODO: put genesis block to network parameters
                BitcoinLikeNetwork::Block genesisBlock;
                genesisBlock.height = 0;
                genesisBlock.hash = "000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f";
                auto synchronizerFactory = std::make_shared<AccountSynchronizerFactory>(
                    pool->getDispatcher()->getThreadPoolExecutionContext("synchronizers"),
                    explorer,
                    genesisBlock);
                // Sets the derivation scheme
                DerivationScheme scheme(STRING(api::Configuration::KEYCHAIN_DERIVATION_SCHEME, "44'/<coin_type>'/<account>'/<node>/<address>"));
                // Build wallet
                auto transactionBroadcaster = createBroadcaster(pool->getConfiguration());
                return std::make_shared<BitcoinLikeWallet>(
                    entry.name,
                    transactionBroadcaster,
                    observer,
                    keychainFactory->second,
                    synchronizerFactory,
                    pool,
                    currency.getValue(),
                    entry.configuration,
                    scheme
                    );
            }

            BitcoinLikeWalletFactory::BitcoinLikeWalletFactory(const api::Currency &currency,
                const std::shared_ptr<WalletPool> &pool)
                : AbstractWalletFactory(currency, pool) {
                _keychainFactories = {
                    {api::KeychainEngines::BIP32_P2PKH, std::make_shared<BitcoinLikeP2PKHKeychainFactory>()},
                    {api::KeychainEngines::BIP49_P2SH, std::make_shared<BitcoinLikeP2SHKeychainFactory>()}
                };
            }

            std::shared_ptr<ExplorerV2<BitcoinLikeNetwork>>
                BitcoinLikeWalletFactory::getExplorer(const std::shared_ptr<api::DynamicObject> &configuration) {
                auto pool = getPool();
                auto engine = configuration->getString(api::Configuration::BLOCKCHAIN_EXPLORER_ENGINE)
                    .value_or(api::BlockchainExplorerEngines::LEDGER_API);
                std::shared_ptr<bitcoin::BitcoinLikeExplorer> explorer = nullptr;
                if (engine == api::BlockchainExplorerEngines::LEDGER_API) {
                    auto http = pool->getHttpClient(
                        configuration->getString(
                            api::Configuration::BLOCKCHAIN_EXPLORER_API_ENDPOINT
                        ).value_or(api::ConfigurationDefaults::BLOCKCHAIN_DEFAULT_API_ENDPOINT)
                    );
                    auto context = pool->getDispatcher()->getSerialExecutionContext(api::BlockchainObserverEngines::LEDGER_API);
                    auto& networkParams = getCurrency().bitcoinLikeNetworkParameters.value();
                    explorer = std::make_shared<bitcoin::BitcoinLikeExplorer>(context, http, networkParams, configuration);
                }
                return explorer;
            }

            std::shared_ptr<common::ExplorerTransactionBroadcaster<BitcoinLikeNetwork>>
                BitcoinLikeWalletFactory::createBroadcaster(const std::shared_ptr<api::DynamicObject> &configuration) {
                auto pool = getPool();
                auto engine = configuration->getString(api::Configuration::BLOCKCHAIN_EXPLORER_ENGINE)
                    .value_or(api::BlockchainExplorerEngines::LEDGER_API);
                if (engine != api::BlockchainExplorerEngines::LEDGER_API) {
                    return nullptr;
                }
                auto http = pool->getHttpClient(
                    configuration->getString(
                        api::Configuration::BLOCKCHAIN_EXPLORER_API_ENDPOINT
                    ).value_or(api::ConfigurationDefaults::BLOCKCHAIN_DEFAULT_API_ENDPOINT)
                );
                auto context = pool->getDispatcher()->getSerialExecutionContext(api::BlockchainObserverEngines::LEDGER_API);
                auto& networkParams = getCurrency().bitcoinLikeNetworkParameters.value();
                return std::make_shared<common::ExplorerTransactionBroadcaster<BitcoinLikeNetwork>>(context, http, networkParams, configuration);
            }

            std::shared_ptr<BitcoinLikeBlockchainObserver>
                BitcoinLikeWalletFactory::getObserver(const std::string &currencyName,
                    const std::shared_ptr<api::DynamicObject> &configuration) {
                auto it = _runningObservers.begin();
                while (it != _runningObservers.end()) {
                    auto observer = it->lock();
                    if (observer != nullptr) {
                        if (observer->match(configuration)) {
                            return observer;
                        }
                        it++;
                    }
                    else {
                        it = _runningObservers.erase(it);
                    }
                }

                auto pool = getPool();
                auto engine = configuration->getString(api::Configuration::BLOCKCHAIN_OBSERVER_ENGINE)
                    .value_or(api::BlockchainObserverEngines::LEDGER_API);
                std::shared_ptr<BitcoinLikeBlockchainObserver> observer;
                if (engine == api::BlockchainObserverEngines::LEDGER_API) {
                    auto ws = pool->getWebSocketClient();
                    auto context = pool->getDispatcher()->getSerialExecutionContext(api::BlockchainObserverEngines::LEDGER_API);
                    auto logger = pool->logger();
                    const auto& currency = getCurrency();
                    observer = std::make_shared<LedgerApiBitcoinLikeBlockchainObserver>(
                        context,
                        ws,
                        configuration,
                        logger,
                        currency
                        );
                }
                if (observer)
                    _runningObservers.push_back(observer);
                return observer;
            }
        }
    }
}