#include <Helpers.hpp>

namespace ledger {
    namespace core {
        namespace tests {
            BitcoinLikeNetwork::Block toBlock(const BL& b) {
                BitcoinLikeNetwork::Block block;
                block.hash = b.hash;
                block.height = b.height;
                return block;
            }

            BitcoinLikeNetwork::Transaction toTran(const std::string& blockHash, const TR& tr) {
                BitcoinLikeNetwork::Transaction tran;
                tran.hash = blockHash + "TR{";
                for (auto& in : tr.inputs) {
                    BitcoinLikeNetwork::Input input;
                    input.address = in;
                    tran.inputs.push_back(input);
                    tran.hash += in + ",";
                }
                tran.hash += "}->{";
                for (auto& out : tr.outputs) {
                    BitcoinLikeNetwork::Output output;
                    output.address = out.first;
                    output.value = BigInt(out.second);
                    tran.outputs.push_back(output);
                    tran.hash += out.first + ",";
                }
                tran.hash += "}";
                return tran;
            }

            BitcoinLikeNetwork::Transaction toTran(const BL& b, const TR& tr) {
                BitcoinLikeNetwork::Block block = toBlock(b);
                BitcoinLikeNetwork::Transaction tran = toTran(block.hash, tr);
                tran.block = block;
                return tran;
            }

            BitcoinLikeNetwork::FilledBlock toFilledBlock(const BL& block) {
                BitcoinLikeNetwork::FilledBlock fb;
                fb.header = toBlock(block);
                std::vector<BitcoinLikeNetwork::Transaction> transactions;
                for (auto& tr : block.transactions) {
                    fb.transactions.push_back(toTran(block, tr));
                }
                return fb;
            }

            std::vector<BitcoinLikeNetwork::FilledBlock> toFilledBlocks(const std::vector<BL>& blocks) {
                std::vector<BitcoinLikeNetwork::FilledBlock> res;
                for (auto& b : blocks) {
                    res.push_back(toFilledBlock(b));
                }
                return res;
            }

            void FakeExplorer::setBlockchain(const std::vector<BitcoinLikeNetwork::FilledBlock>& blockchain) {
                _transactions.clear();
                _blockHashes.clear();
                for (auto& fb : blockchain) {
                    _blockHashes[fb.header.hash] = fb.header.height;
                    for (auto& tr : fb.transactions) {
                        _transactions.push_back(tr);
                    }
                }
                static std::function<bool(const Tran& l, const Tran& r)> blockLess = [](const Tran& l, const Tran& r)->bool {return l.block.getValue().height < r.block.getValue().height; };
                std::sort(_transactions.begin(), _transactions.end(), blockLess);
            }

            void FakeExplorer::setTruncationLevel(uint32_t numberOfTransactionsAllowed) {
                _numberOfTransactionsAllowed = numberOfTransactionsAllowed;
            }

            Future<FakeExplorer::TransactionBulk> FakeExplorer::getTransactions(const std::vector<std::string>& addresses, Option<std::string> fromBlockHash, Option<void*> session) {
                uint32_t fromBlockHeight = 0;
                if (fromBlockHash.hasValue())
                {
                    auto it = _blockHashes.find(fromBlockHash.getValue());
                    if (it == _blockHashes.end()) {
                        return Future<TransactionBulk>::failure(Exception(api::ErrorCode::BLOCK_NOT_FOUND, "Very sorry"));
                    }
                    fromBlockHeight = it->second;
                }
                TransactionBulk result;
                result.second = false;
                static std::function<bool(const BitcoinLikeNetwork::Transaction& l, const BitcoinLikeNetwork::Transaction& r)> blockLess =
                    [](const BitcoinLikeNetwork::Transaction& l, const BitcoinLikeNetwork::Transaction& r)->bool {
                    return l.block.getValue().height < r.block.getValue().height; };
                BitcoinLikeNetwork::Transaction fff;
                Block bbb;
                bbb.height = fromBlockHeight;
                fff.block = bbb;
                auto lower = std::lower_bound(_transactions.begin(), _transactions.end(), fff, blockLess);
                for (auto x = lower; x != _transactions.end(); x++) {
                    auto&tr = *x;
                    bool hasAddress = false;
                    for (auto& in : tr.inputs) {
                        if (in.address.hasValue() && (std::find(addresses.begin(), addresses.end(), in.address.getValue()) != addresses.end())) {
                            hasAddress = true;
                            break;
                        }
                    }
                    if (!hasAddress) {
                        for (auto& out : tr.outputs) {
                            if (out.address.hasValue() && (std::find(addresses.begin(), addresses.end(), out.address.getValue()) != addresses.end())) {
                                hasAddress = true;
                                break;
                            }
                        }
                    }
                    if (hasAddress) {
                        if (result.first.size() == _numberOfTransactionsAllowed) {
                            result.second = true; // truncated
                            break;
                        }
                        result.first.push_back(tr);
                    }
                }
                // to not execute in stack, but simulate really async environment
                return Future<TransactionBulk>::successful(result);
            };


            FakeKeyChain::FakeKeyChain(uint32_t alreadyUsed, uint32_t seed)
                : _alreadyUsed(alreadyUsed)
                , _seed(seed) {
            };

            uint32_t FakeKeyChain::getNumberOfUsedAddresses() {
                return _alreadyUsed;
            }

            std::vector<std::string> FakeKeyChain::getAddresses(uint32_t startIndex, uint32_t count) {
                std::vector<std::string> res;
                for (uint32_t i = startIndex; i < startIndex + count; ++i) {
                    res.push_back(boost::lexical_cast<std::string>(i + _seed));
                }
                return res;
            }

            void FakeKeyChain::markAsUsed(const std::string& address) {
                try {
                    auto addr = boost::lexical_cast<uint32_t>(address);
                    _alreadyUsed = std::max(_alreadyUsed, addr - _seed + 1);
                }
                catch (const boost::bad_lexical_cast &) {}; // ignore
            }
        }
    }
}