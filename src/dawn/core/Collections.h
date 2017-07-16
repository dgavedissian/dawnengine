/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2017 (git@dga.me.uk)
 */
#pragma once

#include <tuple>
#include <vector>
#include <list>
#include <map>
#include <unordered_map>

#if DW_PLATFORM == DW_WIN32
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
#include <mapbox/variant.hpp>
#if DW_PLATFORM == DW_WIN32
#pragma warning(pop)
#endif

namespace dw {

// If using GCC, create a hash wrapper to work around std::hash<T> not working for enum
// classes.
#if defined(DW_GCC)
template <typename T, typename Enable = void> struct HashFunction {
    typedef typename std::hash<T>::argument_type argument_type;
    typedef typename std::hash<T>::result_type result_type;
    inline result_type operator()(argument_type const& s) const {
        std::hash<T> standard_hash;
        return standard_hash(s);
    }
};
template <typename E> struct HashFunction<E, std::enable_if_t<std::is_enum<E>::value>> {
    typedef E argument_type;
    typedef std::size_t result_type;
    inline result_type operator()(argument_type const& s) const {
        return static_cast<result_type>(s);
    }
};
#else
template <typename T> using HashFunction = std::hash<T>;
#endif

template <typename T, int N> using Array = std::array<T, N>;
template <typename T> using Vector = std::vector<T>;
template <typename T> using List = std::list<T>;
template <typename K, typename T> using Map = std::map<K, T>;
template <typename K, typename T> using HashMap = std::unordered_map<K, T, HashFunction<K>>;
template <typename T1, typename T2> using Pair = std::pair<T1, T2>;
template <typename... T> using Tuple = std::tuple<T...>;
template <typename... T> using Variant = mapbox::util::variant<T...>;

template <typename F, typename V> void VariantApplyVisitor(F&& f, V const& v) {
    return mapbox::util::apply_visitor(std::forward<F>(f), v);
}

template <typename F, typename V> void VariantApplyVisitor(F&& f, V& v) {
    return mapbox::util::apply_visitor(std::forward<F>(f), v);
}

template <typename T1, typename T2> Pair<T1, T2> makePair(T1&& a, T2&& b) {
    return std::pair<T1, T2>(std::forward<T1>(a), std::forward<T2>(b));
}

template <typename... T> Tuple<T...> makeTuple(T&&... args) {
    return std::tuple<T...>(std::forward<T>(args)...);
}
}  // namespace dw
