/*
 *
 * DynamicAbstractFactory
 * ledger-core
 *
 * Created by Alexis Le Provost on 16/09/2019.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ledger
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

#pragma once 

namespace ledger
{
namespace core
{

template <typename Key, typename T>
typename AbstractFactoryGenerator<Key, T>::value_type AbstractFactoryGenerator<Key, T>::add(
    key_type const& key, value_type const& factory)
{
    auto it = factories_.find(key);
 
    if (it == std::end(factories_)) {
        return nullptr;
    }

    auto old_factory = std::move(it->second);
    it->second = factory;

    return old_factory;
}

template <typename Key, typename T>
void AbstractFactoryGenerator<Key, T>::remove(key_type const& key)
{
    auto it = factories_.find(key);

    if (it != std::end(factories_)) {
        factories_.erase(it);
    }
}

template <typename Key, typename T>
typename AbstractFactoryGenerator<Key, T>::value_type AbstractFactoryGenerator<Key, T>::make(key_type const& key) const
{
    auto it = factories_.find(key);

    if (it == std::end(factories_)) {
        return nullptr;
    }

    return it->second->shared_from_this();
}

} // namespace core
} // namespace ledger