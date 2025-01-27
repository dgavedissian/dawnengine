/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2019 (git@dga.dev)
 */
#pragma once

// Mark this header as a system header
#if defined(DW_GCC) || defined(DW_CLANG)
#pragma GCC system_header
#elif defined(DW_MSVC)
#pragma warning(push, 0)
#endif

#include <tuple>
#include <vector>
#include <list>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <deque>
#include <queue>
#include <optional>

#include <tl/expected.hpp>
#include <mpark/variant.hpp>
#include <concurrentqueue.h>

// Re-enable warnings
#if defined(DW_MSVC)
#pragma warning(pop)
#endif

namespace dw {
// If using GCC or Clang, create a hash wrapper to work around std::hash<T> not working for enum
// classes.
#if (defined(DW_LIBSTDCPP) && (DW_LIBSTDCPP < 6100)) || (defined(DW_LIBCPP) && (DW_LIBCPP < 3500))
template <typename T, typename Enable = void> struct HashFunction {
    typedef typename std::hash<T>::argument_type argument_type;
    typedef typename std::hash<T>::result_type result_type;
    inline result_type operator()(argument_type const& s) const {
        return std::hash<T>()(s);
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
template <typename T> using Deque = std::deque<T>;
template <typename T> using Queue = std::queue<T>;
template <typename K, typename T> using Map = std::map<K, T>;
template <typename K, typename T> using MultiMap = std::multimap<K, T>;
template <typename K, typename T> using HashMap = std::unordered_map<K, T, HashFunction<K>>;
template <typename K, typename T> using HashMultiMap = std::unordered_multimap<K, T, HashFunction<K>>;
template <typename K> using Set = std::set<K>;
template <typename K> using HashSet = std::unordered_set<K, HashFunction<K>>;
template <typename T1, typename T2> using Pair = std::pair<T1, T2>;
template <typename... Ts> using Tuple = std::tuple<Ts...>;
template <typename... T> using Variant = mpark::variant<T...>;
template <typename T> using Option = std::optional<T>;
template <typename T, typename E = String> using Result = tl::expected<T, E>;
template <typename E> using Unexpected = tl::unexpected<E>;
template <typename T> using ConcurrentQueue = moodycamel::ConcurrentQueue<T>;

template <typename T, typename... Ts>
inline constexpr bool holdsAlternative(const Variant<Ts...> &v) noexcept {
    return mpark::holds_alternative<T>(v);
}

using mpark::visit;
using mpark::get;

template <typename T1, typename T2> Pair<T1, T2> makePair(T1&& a, T2&& b) {
    return std::pair<T1, T2>(std::forward<T1>(a), std::forward<T2>(b));
}

template <typename... T> Tuple<T...> makeTuple(T&&... args) {
    return std::tuple<T...>(std::forward<T>(args)...);
}

template <typename E> Unexpected<typename std::decay<E>::type> makeError(E&& error) {
    return Unexpected<typename std::decay<E>::type>(std::forward<E>(error));
}
}  // namespace dw
