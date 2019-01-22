/*
 *
 * keychain_test_helper
 * ledger-core
 *
 * Created by El Khalil Bellakrid on 08/07/2018.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Ledger
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


#include <gtest/gtest.h>
#include <src/wallet/ethereum/keychains/EthereumLikeKeychain.hpp>
#include <src/ethereum/EthereumLikeExtendedPublicKey.h>
#include <src/ethereum/EthereumLikeAddress.h>
#include <src/utils/DerivationPath.hpp>
#include <src/utils/optional.hpp>
#include "keychain_test_helper.h"
#include "../BaseFixture.h"
#include <iostream>
using namespace std;
class EthereumKeychains : public BaseFixture {
public:
    void testEthKeychain(const KeychainTestData &data, std::function<void (EthereumLikeKeychain&)> f) {
        auto backend = std::make_shared<ledger::core::PreferencesBackend>(
                "/preferences/tests.db",
                dispatcher->getMainExecutionContext(),
                resolver
        );
        auto configuration = std::make_shared<DynamicObject>();
        dispatcher->getMainExecutionContext()->execute(ledger::qt::make_runnable([=]() {
            EthereumLikeKeychain keychain(
                    configuration,
                    data.currency,
                    0,
                    ledger::core::EthereumLikeExtendedPublicKey::fromBase58(data.currency,
                                                                            data.xpub,
                                                                            optional<std::string>(data.derivationPath)),
                    backend->getPreferences("keychain")
            );
            f(keychain);
            dispatcher->stop();
        }));
        dispatcher->waitUntilStopped();
    };
};

TEST_F(EthereumKeychains, KeychainDerivation) {
    testEthKeychain(ETHEREUM_DATA, [] (EthereumLikeKeychain& keychain) {
        auto ethDerivedAddress = keychain.getAllObservableAddresses(0, 0)[0];
        EXPECT_EQ(ethDerivedAddress->getDerivationPath().value_or(""), "0/0");
        EXPECT_EQ(ethDerivedAddress->toEIP55(), "0xE8F7Dc1A12F180d49c80D1c3DbEff48ee38bD1DA");
    });
}

TEST_F(EthereumKeychains, EthereumAddressValidation) {
    auto address = "0x8f7A0aFAAEE372EEFd020056FC552BD87DD75D73";
    auto ethAddress = ledger::core::EthereumLikeAddress::fromEIP55(address, ledger::core::currencies::ETHEREUM);
    EXPECT_EQ(ethAddress->toEIP55(), address);

}


TEST_F(EthereumKeychains, EthereumAddressValidationFromXpub) {
    auto extKey = ledger::core::EthereumLikeExtendedPublicKey::fromBase58(ETHEREUM_DATA.currency,
                                                                          ETHEREUM_DATA.xpub,
                                                                          optional<std::string>(ETHEREUM_DATA.derivationPath));
    EXPECT_EQ(extKey->toBase58(), ETHEREUM_DATA.xpub);

    auto derivedPubKey = "xpub6DrvMc6me5H6sV3Wrva6thZyhxMZ7WMyB8nMWLe3T5xr79bBsDJn2zgSQiVWEbU5XfoLMEz7oZT9G49AoCcxYNrz2dVBrySzUw4k9GTNyoW";
    auto derivedExtKey = extKey->derive(ledger::core::DerivationPath("0"));
    EXPECT_EQ(derivedExtKey->toBase58(), derivedPubKey);

    auto address = "0xE8F7Dc1A12F180d49c80D1c3DbEff48ee38bD1DA";
    auto derivedAddress = extKey->derive("0/0");
    EXPECT_EQ(derivedAddress->toEIP55(), address);
}

TEST_F(EthereumKeychains, EthereumChildAddressValidationFromPubKeyAndChainCode) {
    auto path = "44'/60'/0'";
    auto pubKey = "035dd2992d954b3d232037aba9cc7fc08c2155e4f3616aa1290edc9cc09f8d64f0";
    auto chainCode = "6a4e60e6fbd45355d840ff7a18bc7cb628318f1ba6fbcfb0c07626d8ea768aca";
    auto ethXpub = ledger::core::EthereumLikeExtendedPublicKey::fromRaw(ledger::core::currencies::ETHEREUM,
                                                                        optional<std::vector<uint8_t >>(),
                                                                        hex::toByteArray(pubKey),
                                                                        hex::toByteArray(chainCode),
                                                                        path);
    auto address = "0xE8F7Dc1A12F180d49c80D1c3DbEff48ee38bD1DA";
    auto derive0 = ethXpub->derive("0/0");
    EXPECT_EQ(derive0->toEIP55(), address);

}

const std::vector<std::vector<std::string>> derivationTestData = {
        {
                "44'/<coin_type>'/<account>'/<node>/<address>",
                "44'/60'/0'/0/0",
                "04d1dc4a3180fe2d56a1f02a68b053e59022ce5e107eae879ebef66a46d4ffe04dc3994facd376abcbab49c421599824a2600ee30e8520878e65581f598e2c497a",
                "2d560fcaaedb929eea27d316dec7961eee884259e6483fdf192704db7582ca14",
                "0xAc6603e97e774Cd34603293b69bBBB1980acEeaA"
        },
        {
                "44'/<coin_type>'/<account>'/<node>/<address>",
                "44'/60'/1'/0/0",
                "04c6dab3de86f6e44a3f54bcd204ea63dfef4e728fac050068f7fa391e0a623735258165fb5bad2a583110cb482c5d47f649ca49efc4997df77d01d0132ce4d082",
                "2d560fcaaedb929eea27d316dec7961eee884259e6483fdf192704db7582ca14",
                "0x8AB03601CFD6B5eda60c2ABFe4A2277F543b7f5d"
        },
        {
                "44'/<coin_type>'/0'/<account>",
                "44'/60'/0'/0",
                "045ff91ffa3506fa2dce2175f2ef30821e89bba5e9581d348d34b976acd37d83aa1d4491cef5282ff02dcb7d98ca885bfdf72b473165ef952d9912540e89735b13",
                "3cb96430fa5528cd8ec4cbc4184645466f3df040fa780354c2151f0b906f0bb3",
                "0x7F916511864686e5a9952f1d66595e1A90520670"
        },
        {
                "44'/<coin_type>'/0'/<account>",
                "44'/60'/0'/1",
                "04beb03c024dd2d199fe2c137c9dc2c89345a2578f2c65fad3aae0e970e90a352d18725b1eec1b3dbe00d62be83bb48b74138dcc7d86a16c6610ed203d4e09aa33",
                "4d0565d7f8ea65680c4c148635385eecfd10c8453f47986942197bcebf1a5ae8",
                "0x179B50609c17AC28c25Df0Abe0E1A2Fdc75dcF56"
        },
        {
                "44'/<coin_type>'/0'/<account>'",
                "44'/60'/0'/0'",
                "04d2ee4bb49221f9f1662e4791748e68354c26d7d5290ad518c86c4d714c785e6533e0286d3803b0ddde3287eb6f31f77792fdf7323f76152c14069805f23121d2",
                "ddf5a9cf1fdf4746a4495cf36328c7e2af31d18dd0a8f8302f3e13c900f4bfb9",
                "0x390De614378307a6d85cD0e68460378A745295b1"
        }

};

TEST_F(EthereumKeychains, EthereumAddressValidationFromPubKeyAndChainCode) {

    for (auto &elem : derivationTestData) {
        //So we know what we are dealing with
        auto derivationScheme = elem[0];
        auto path = elem[1];
        auto publicKey = elem[2];
        auto chainCode = elem[3];
        auto expectedAddress = elem[4];

        auto config = DynamicObject::newInstance();
        config->putString(api::Configuration::KEYCHAIN_DERIVATION_SCHEME,derivationScheme);
        auto ethXpub = ledger::core::EthereumLikeExtendedPublicKey::fromRaw(ledger::core::currencies::ETHEREUM,
                                                                            optional<std::vector<uint8_t >>(),
                                                                            hex::toByteArray(publicKey),
                                                                            hex::toByteArray(chainCode),
                                                                            path);
        auto derivedAddress = ethXpub->derive("");
        EXPECT_EQ(derivedAddress->toEIP55(), expectedAddress);
    }

}