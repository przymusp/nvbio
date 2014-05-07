/*
 * nvbio
 * Copyright (c) 2011-2014, NVIDIA CORPORATION. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the NVIDIA CORPORATION nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NVIDIA CORPORATION BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <nvbio/basic/types.h>
#include <nvbio/basic/packedstream.h>
#include <nvbio/basic/packedstream_loader.h>
#include <nvbio/basic/vector_view.h>
#include <nvbio/strings/infix.h>


namespace nvbio {

///@addtogroup Strings
///@{

///@defgroup StringPrefetchers String Prefetchers
///
/// This module implements a set of string <i>prefetchers</i> for common string types.
/// The idea behind prefetching is that on some CUDA architectures it's often useful
/// to pre-load the words of memory where strings are stored in a local-memory cache before
/// running expensive algorithms on them (especially with packed-strings).
/// This is because local-memory reads guarantee fully coalesced accesses and implement
/// efficient L1 caching.
///
///@{

///
/// A class to prefetch a string using a given caching strategy
///
/// \tparam StringType      the input string type
/// \tparam CacheTag        the cache type
///
template <typename StringType, typename CacheTag>
struct StringPrefetcher
{
    typedef StringType  input_string_type;
    typedef StringType        string_type;

    /// given a string, prefetch all its content and return a new string object
    /// wrapping the cached version
    ///
    /// \param string       input string
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    const string_type& load(const input_string_type& string) { return string; }

    /// given a string, prefetch the contents of a substring and return a new string object
    /// wrapping the cached version
    ///
    /// \param string           input string
    /// \param range            range of the substring to load
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    const string_type& load(
        const input_string_type& string,
        const uint2              range) { return string; }
};

///
/// A class to prefetch a packed string using a local-memory cache
///
/// \tparam StorageIterator     the underlying packed string storage iterator
/// \tparam SYMBOL_SIZE_T       the size of the packed symbols, in bits
/// \tparam BIG_ENDIAN_T        the endianness of the packing
/// \tparam CACHE_SIZE          the local-memory cache size, in words
///
template <typename StorageIterator, uint32 SYMBOL_SIZE_T, bool BIG_ENDIAN_T, uint32 CACHE_SIZE>
struct StringPrefetcher<
    vector_view< PackedStreamIterator< PackedStream<StorageIterator,uint8,SYMBOL_SIZE_T,BIG_ENDIAN_T> > >,
    lmem_cache_tag<CACHE_SIZE> >
{
    typedef vector_view< PackedStreamIterator< PackedStream<StorageIterator,uint8,SYMBOL_SIZE_T,BIG_ENDIAN_T> > >   input_string_type;
    typedef PackedStringLoader<StorageIterator,SYMBOL_SIZE_T,BIG_ENDIAN_T,lmem_cache_tag<CACHE_SIZE> >              loader_type;
    typedef vector_view<typename loader_type::iterator>                                                             string_type;

    /// given a string, prefetch all its content and return a new string object
    /// wrapping the cached version
    ///
    /// \param string       input string
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    string_type load(const input_string_type& string)
    {
        return string_type(
            string.size(),
            loader.load( string.base().container().stream(),
                         string.base().index(),
                         string.size() ) );
    }

    /// given a string, prefetch the contents of a substring and return a new string object
    /// wrapping the cached version
    ///
    /// \param string           input string
    /// \param range            range of the substring to load
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    string_type load(
        const input_string_type& string,
        const uint2              range)
    {
        return string_type(
            string.size(),
            loader.load( string.base().container().stream(),
                         string.base().index(),
                         string.size(),
                         range,
                         false ) );
    }

    loader_type loader;
};

///
/// A class to prefetch an infix built on top of a PackedStreamIterator using a local-memory cache
///
/// \tparam StorageIterator     the underlying packed string storage iterator
/// \tparam SYMBOL_SIZE_T       the size of the packed symbols, in bits
/// \tparam BIG_ENDIAN_T        the endianness of the packing
/// \tparam CACHE_SIZE          the local-memory cache size, in words
///
template <typename InfixCoordType, typename StorageIterator, uint32 SYMBOL_SIZE_T, bool BIG_ENDIAN_T, uint32 CACHE_SIZE>
struct StringPrefetcher<
    Infix< PackedStreamIterator< PackedStream<StorageIterator,uint8,SYMBOL_SIZE_T,BIG_ENDIAN_T> >,
           InfixCoordType >,
    lmem_cache_tag<CACHE_SIZE> >
{
    typedef Infix< PackedStreamIterator< PackedStream<StorageIterator,uint8,SYMBOL_SIZE_T,BIG_ENDIAN_T> >,
                   InfixCoordType>                                                                                  input_string_type;
    typedef PackedStringLoader<StorageIterator,SYMBOL_SIZE_T,BIG_ENDIAN_T,lmem_cache_tag<CACHE_SIZE> >              loader_type;
    typedef vector_view<typename loader_type::iterator>                                                             string_type;

    /// given a string, prefetch all its content and return a new string object
    /// wrapping the cached version
    ///
    /// \param string       input string
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    string_type load(const input_string_type& string)
    {
        return string_type(
            string.size(),
            loader.load( string.m_string.container().stream(),
                         string.m_string.index() + string.range().x,
                         string.size() ) );
    }

    /// given a string, prefetch the contents of a substring and return a new string object
    /// wrapping the cached version
    ///
    /// \param string           input string
    /// \param range            range of the substring to load
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    string_type load(
        const input_string_type& string,
        const uint2              range)
    {
        return string_type(
            string.size(),
            loader.load( string.m_string.container().stream(),
                         string.m_string.index() + string.range().x,
                         string.size(),
                         range,
                         false ) );
    }

    loader_type loader;
};

///
/// A class to prefetch an infix built on top of a vector_view<PackedStreamIterator> using a local-memory cache
///
/// \tparam StorageIterator     the underlying packed string storage iterator
/// \tparam SYMBOL_SIZE_T       the size of the packed symbols, in bits
/// \tparam BIG_ENDIAN_T        the endianness of the packing
/// \tparam CACHE_SIZE          the local-memory cache size, in words
///
template <typename InfixCoordType, typename StorageIterator, uint32 SYMBOL_SIZE_T, bool BIG_ENDIAN_T, uint32 CACHE_SIZE>
struct StringPrefetcher<
    Infix< vector_view< PackedStreamIterator< PackedStream<StorageIterator,uint8,SYMBOL_SIZE_T,BIG_ENDIAN_T> > >,
           InfixCoordType >,
    lmem_cache_tag<CACHE_SIZE> >
{
    typedef Infix< vector_view< PackedStreamIterator< PackedStream<StorageIterator,uint8,SYMBOL_SIZE_T,BIG_ENDIAN_T> > >,
                   InfixCoordType>                                                                                  input_string_type;
    typedef PackedStringLoader<StorageIterator,SYMBOL_SIZE_T,BIG_ENDIAN_T,lmem_cache_tag<CACHE_SIZE> >              loader_type;
    typedef vector_view<typename loader_type::iterator>                                                             string_type;

    /// given a string, prefetch all its content and return a new string object
    /// wrapping the cached version
    ///
    /// \param string       input string
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    string_type load(const input_string_type& string)
    {
        return string_type(
            string.size(),
            loader.load( string.m_string.base().container().stream(),
                         string.m_string.base().index() + string.range().x,
                         string.size() ) );
    }

    /// given a string, prefetch the contents of a substring and return a new string object
    /// wrapping the cached version
    ///
    /// \param string           input string
    /// \param range            range of the substring to load
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    string_type load(
        const input_string_type& string,
        const uint2              range)
    {
        return string_type(
            string.size(),
            loader.load( string.m_string.base().container().stream(),
                         string.m_string.base().index() + string.range().x,
                         string.size(),
                         range,
                         false ) );
    }

    loader_type loader;
};

///@} StringPrefetchers
///@} Strings

} // namespace nvbio