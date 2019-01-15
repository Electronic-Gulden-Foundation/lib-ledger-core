/*
 *
 * PreferencesBackend
 * ledger-core
 *
 * Created by Pierre Pollastri on 10/01/2017.
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
#include "PreferencesBackend.hpp"
#include "../utils/Exception.hpp"
#include "../utils/LambdaRunnable.hpp"
#include <leveldb/write_batch.h>
#include <cstring>
#include <leveldb/env.h>
#include <iterator>

namespace ledger {
    namespace core {
        namespace {
            // number of iteration to perform for PBKDF2
            const auto PBKDF2_ITERS = 10000; // see https://pages.nist.gov/800-63-3/sp800-63b.html#sec5
        }

        PreferencesChange::PreferencesChange(PreferencesChangeType t, std::vector<uint8_t> k, std::vector<uint8_t> v)
            : type(t), key(k), value(v) {
        }

        PreferencesEncryption::PreferencesEncryption(std::shared_ptr<api::RandomNumberGenerator> rng, const std::string& password, const std::string& salt)
            : rng(rng), password(password), salt(salt) {
        }

        std::unordered_map<std::string, std::weak_ptr<leveldb::DB>> PreferencesBackend::LEVELDB_INSTANCE_POOL;
        std::mutex PreferencesBackend::LEVELDB_INSTANCE_POOL_MUTEX;

        PreferencesBackend::PreferencesBackend(const std::string &path,
                                               const std::shared_ptr<api::ExecutionContext>& writingContext,
                                               const std::shared_ptr<api::PathResolver> &resolver,
                                               Option<PreferencesEncryption> encryption) {
            _context = writingContext;
            _db = obtainInstance(resolver->resolvePreferencesPath(path));

            // if an encryption setup was passed, initialize the AES cipher for future use
            if (encryption.hasValue()) {
                _cipher = AESCipher(encryption->rng, encryption->password, encryption->salt, PBKDF2_ITERS);
            }
        }

        std::shared_ptr<leveldb::DB> PreferencesBackend::obtainInstance(const std::string &path) {
            std::lock_guard<std::mutex> lock(LEVELDB_INSTANCE_POOL_MUTEX);
            auto it = LEVELDB_INSTANCE_POOL.find(path);
            if (it != LEVELDB_INSTANCE_POOL.end()) {
                auto db = it->second.lock();
                if (db != nullptr)
                    return db;
            }
            leveldb::DB *db;
            leveldb::Options options;
            options.create_if_missing = true;
            auto status = leveldb::DB::Open(options, path, &db);
            if (!status.ok()) {
                throw Exception(api::ErrorCode::UNABLE_TO_OPEN_LEVELDB, status.ToString());
            }
            auto instance = std::shared_ptr<leveldb::DB>(db);
            std::weak_ptr<leveldb::DB> weakInstance = instance;
            LEVELDB_INSTANCE_POOL[path] = weakInstance;
            return instance;
        }

        void PreferencesBackend::commit(const std::vector<PreferencesChange> &changes) {
            auto db = _db;
            leveldb::WriteBatch batch;
            leveldb::WriteOptions options;
            options.sync = true;
            for (auto& item : changes) {
                leveldb::Slice k((const char *)item.key.data(), item.key.size());
                if (item.type == PreferencesChangeType::PUT_TYPE) {
                    if (_cipher.hasValue()) {
                        auto encrypted = encrypt_preferences_change(item);
                        leveldb::Slice v((const char *)encrypted.data(), encrypted.size());
                        batch.Put(k, v);
                    } else {
                        leveldb::Slice v((const char *)item.value.data(), item.value.size());
                        batch.Put(k, v);
                    }
                } else {
                    batch.Delete(k);
                }
            }
            db->Write(options, &batch);
        }

        optional<std::string> PreferencesBackend::get(const std::vector<uint8_t>& key) {
            leveldb::Slice k((const char *)key.data(), key.size());
            std::string value;

            auto status = _db->Get(leveldb::ReadOptions(), k, &value);
            if (status.ok()) {
                if (_cipher.hasValue()) {
                    auto ciphertext = std::vector<uint8_t>(std::begin(value), std::end(value));
                    auto plaindata = decrypt_preferences_change(ciphertext);
                    auto plaintext = std::string(std::begin(plaindata), std::end(plaindata));

                    return optional<std::string>(plaintext);
                } else {
                    return optional<std::string>(value);
                }
            } else {
                return optional<std::string>();
            }
        }

        void PreferencesBackend::iterate(const std::vector<uint8_t> &keyPrefix,
                                         std::function<bool (leveldb::Slice &&, leveldb::Slice &&)> f) {
            std::unique_ptr<leveldb::Iterator> it(_db->NewIterator(leveldb::ReadOptions()));
            leveldb::Slice start((const char *) keyPrefix.data(), keyPrefix.size());
            std::vector<uint8_t> limitRaw(keyPrefix.begin(), keyPrefix.end());

            limitRaw[limitRaw.size() - 1] += 1;
            leveldb::Slice limit((const char *) limitRaw.data(), limitRaw.size());

            for (it->Seek(start); it->Valid(); it->Next()) {
                if (it->key().compare(limit) > 0) {
                    break;
                }

                if (_cipher.hasValue()) {
                    // decrypt the value on the fly
                    auto value = it->value().ToString();
                    auto ciphertext = std::vector<uint8_t>(std::begin(value), std::end(value));
                    auto plaindata = decrypt_preferences_change(ciphertext);
                    auto plaintext = std::string(std::begin(plaindata), std::end(plaindata));
                    leveldb::Slice slice(plaintext);

                    if (!f(it->key(), std::move(slice))) {
                        break;
                    }
                } else {
                    if (!f(it->key(), it->value())) {
                        break;
                    }
                }
            }
        }

        std::shared_ptr<Preferences> PreferencesBackend::getPreferences(const std::string &name) {
            return std::make_shared<Preferences>(*this, std::vector<uint8_t>(name.data(), name.data() + name.size()));
        }

        std::vector<uint8_t> PreferencesBackend::encrypt_preferences_change(const PreferencesChange& change) {
          auto input = BytesReader(change.value);
          auto output = BytesWriter();
          _cipher->encrypt(input, output);

          return output.toByteArray();
        }

        std::vector<uint8_t> PreferencesBackend::decrypt_preferences_change(const std::vector<uint8_t>& data) {
          auto input = BytesReader(data);
          auto output = BytesWriter();
          _cipher->decrypt(input, output);

          return output.toByteArray();
        }
    }
}
