// AUTOGENERATED FILE - DO NOT MODIFY!
// This file generated by Djinni from bitcoin_like_wallet.djinni

#import "LGBitcoinLikeTransactionRequest.h"
#include "BitcoinLikeTransactionRequest.hpp"

static_assert(__has_feature(objc_arc), "Djinni requires ARC to be enabled for this file");

@class LGBitcoinLikeTransactionRequest;

namespace djinni_generated {

struct BitcoinLikeTransactionRequest
{
    using CppType = ::ledger::core::api::BitcoinLikeTransactionRequest;
    using ObjcType = LGBitcoinLikeTransactionRequest*;

    using Boxed = BitcoinLikeTransactionRequest;

    static CppType toCpp(ObjcType objc);
    static ObjcType fromCpp(const CppType& cpp);
};

}  // namespace djinni_generated