#pragma once

#include <algorithm>
#include <cereal/types/common.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/set.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/string.hpp>
#include <map>
#include <math/BigInt.h>
#include <set>

namespace ledger {
    namespace core {
        namespace bitcoin {
            /// An UTXO key, indexing a certain amount of satoshis (bitcoin fraction) in the
            /// blockchain.
            ///
            /// You typically find a UTXOKey attached (std::pair) with a UTXOValue.
            typedef std::pair<std::string, uint32_t> UTXOKey;

            /// An UTXO value, giving the amount of satoshis received on a given address.
            struct UTXOValue {
                /// Amount.
                BigInt amount;
                /// Address that was used.
                std::string address;

                UTXOValue(const BigInt& satoshis, const std::string& address);
            };

            /// An UTXO source list.
            ///
            /// Such a set will contain a list of UTXO that are available for use in this source and
            /// a list of UTXOs that have been sent in the source but are unknown (they might come
            /// from other sources).
            struct UTXOSourceList {
                std::map<UTXOKey, UTXOValue> available; ///< Available UTXOs.
                std::set<UTXOKey> spent; ///< Spent UTXOs we don�t know / can�t resolve (yet).
                UTXOSourceList() = default;
                UTXOSourceList(std::map<UTXOKey, UTXOValue>&& available, std::set<UTXOKey>&& spent);
                UTXOSourceList(const std::map<UTXOKey, UTXOValue>& available, const std::set<UTXOKey>& spent);
            };
        }
    }
}
