// AUTOGENERATED FILE - DO NOT MODIFY!
// This file generated by Djinni from wallet_pool.djinni

#import <Foundation/Foundation.h>
@class LGBitcoinLikeWallet;
@class LGCryptoCurrencyDescription;
@class LGEthereumLikeWallet;
@class LGLogger;
@class LGWalletCommonInterface;
@protocol LGBitcoinPublicKeyProvider;
@protocol LGEthereumPublicKeyProvider;
@protocol LGGetBitcoinLikeWalletCallback;
@protocol LGGetEthreumLikeWalletCallback;


@interface LGWalletPool : NSObject

- (nonnull NSArray<LGWalletCommonInterface *> *)getAllWallets;

- (nonnull NSArray<LGBitcoinLikeWallet *> *)getAllBitcoinLikeWallets;

- (nonnull NSArray<LGEthereumLikeWallet *> *)getAllEthereumLikeWallets;

- (void)getOrCreateBitcoinLikeWallet:(nullable id<LGBitcoinPublicKeyProvider>)publicKeyProvider
                            currency:(nullable LGCryptoCurrencyDescription *)currency
                            callback:(nullable id<LGGetBitcoinLikeWalletCallback>)callback;

- (void)getOrCreateEthereumLikeWallet:(nullable id<LGEthereumPublicKeyProvider>)publicKeyProvider
                             currency:(nullable LGCryptoCurrencyDescription *)currency
                             callback:(nullable id<LGGetEthreumLikeWalletCallback>)callback;

- (nonnull NSArray<LGCryptoCurrencyDescription *> *)getAllSupportedCryptoCurrencies;

- (nullable LGLogger *)getLogger;

- (void)close;

@end