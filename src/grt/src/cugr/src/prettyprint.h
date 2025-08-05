//
// Pretty printing for C++ STL containers
//
// Usage: operator<< will "just work" for most STL containers, for example,
//      std::vector<int> nums = {1, 2, 10};
//      std::cout << nums << std::endl;
//     This code piece will show "1 2 10".
//

#pragma once

#include <memory>
#include <set>
#include <unordered_set>
#include <sstream>

namespace utils {

// SFINAE has_begin_end

template <typename T, typename = void>
struct has_begin_end : std::false_type {};
template <typename T>
struct has_begin_end<T, decltype((void)std::begin(std::declval<T>()), (void)std::end(std::declval<T>()))>
    : std::true_type {};

// Holds the delimiter values for a specific character type

template <typename TChar>
struct delimiters_values {
    using char_type = TChar;
    const char_type *prefix;
    const char_type *delimiter;
    const char_type *postfix;
};

// Defines the delimiter values for a specific container and character type

template <typename T, typename TChar = char>
struct delimiters {
    using type = delimiters_values<TChar>;
    static const type values;
};

// Functor to print containers. You can use this directly if you want
// to specificy a non-default delimiters type. The printing logic can
// be customized by specializing the nested template.

template <typename T,
          typename TChar = char,
          typename TCharTraits = std::char_traits<TChar>,
          typename TDelimiters = delimiters<T, TChar>>
struct print_container_helper {
    using delimiters_type = TDelimiters;
    using ostream_type = std::basic_ostream<TChar, TCharTraits>;

    template <typename U>
    struct printer {
        static void print_body(const U &c, ostream_type &stream) {
            auto it = std::begin(c);
            const auto the_end = std::end(c);

            if (it != the_end) {
                for (;;) {
                    stream << *it;

                    if (++it == the_end) break;

                    if (delimiters_type::values.delimiter != NULL) stream << delimiters_type::values.delimiter;
                }
            }
        }
    };

    print_container_helper(const T &container) : container_(container) {}

    inline void operator()(ostream_type &stream) const {
        if (delimiters_type::values.prefix != NULL) stream << delimiters_type::values.prefix;

        printer<T>::print_body(container_, stream);

        if (delimiters_type::values.postfix != NULL) stream << delimiters_type::values.postfix;
    }

private:
    const T &container_;
};

// Specialization for pairs

template <typename T, typename TChar, typename TCharTraits, typename TDelimiters>
template <typename T1, typename T2>
struct print_container_helper<T, TChar, TCharTraits, TDelimiters>::printer<std::pair<T1, T2>> {
    using ostream_type = typename print_container_helper<T, TChar, TCharTraits, TDelimiters>::ostream_type;

    static void print_body(const std::pair<T1, T2> &c, ostream_type &stream) {
        stream << c.first;
        if (print_container_helper<T, TChar, TCharTraits, TDelimiters>::delimiters_type::values.delimiter != NULL)
            stream << print_container_helper<T, TChar, TCharTraits, TDelimiters>::delimiters_type::values.delimiter;
        stream << c.second;
    }
};

// Specialization for tuples

template <typename T, typename TChar, typename TCharTraits, typename TDelimiters>
template <typename... Args>
struct print_container_helper<T, TChar, TCharTraits, TDelimiters>::printer<std::tuple<Args...>> {
    using ostream_type = typename print_container_helper<T, TChar, TCharTraits, TDelimiters>::ostream_type;
    using element_type = std::tuple<Args...>;

    template <std::size_t I>
    struct Int {};

    static void print_body(const element_type &c, ostream_type &stream) { tuple_print(c, stream, Int<0>()); }

    static void tuple_print(const element_type &, ostream_type &, Int<sizeof...(Args)>) {}

    static void tuple_print(const element_type &c,
                            ostream_type &stream,
                            typename std::conditional<sizeof...(Args) != 0, Int<0>, std::nullptr_t>::type) {
        stream << std::get<0>(c);
        tuple_print(c, stream, Int<1>());
    }

    template <std::size_t N>
    static void tuple_print(const element_type &c, ostream_type &stream, Int<N>) {
        if (print_container_helper<T, TChar, TCharTraits, TDelimiters>::delimiters_type::values.delimiter != NULL)
            stream << print_container_helper<T, TChar, TCharTraits, TDelimiters>::delimiters_type::values.delimiter;

        stream << std::get<N>(c);

        tuple_print(c, stream, Int<N + 1>());
    }
};

// Prints a print_container_helper to the specified stream.

template <typename T, typename TChar, typename TCharTraits, typename TDelimiters>
inline std::basic_ostream<TChar, TCharTraits> &operator<<(
    std::basic_ostream<TChar, TCharTraits> &stream,
    const print_container_helper<T, TChar, TCharTraits, TDelimiters> &helper) {
    helper(stream);
    return stream;
}

// Basic is_container template; specialize to derive from std::true_type for all desired container types

template <typename T>
struct is_container : std::integral_constant<bool, has_begin_end<T>::value> {};

template <typename... T>
struct is_container<std::pair<T...>> : std::true_type {};

template <typename... T>
struct is_container<std::tuple<T...>> : std::true_type {};

// Default delimiters

template <typename T>
struct delimiters<T, char> {
    static constexpr delimiters_values<char> values = {"[", ", ", "]"};
};

// Delimiters for (unordered_)(multi)set

template <typename... T>
struct delimiters<std::set<T...>> {
    static constexpr delimiters_values<char> values = {"{", ", ", "}"};
};

template <typename... T>
struct delimiters<std::multiset<T...>> {
    static constexpr delimiters_values<char> values = {"{", ", ", "}"};
};

template <typename... T>
struct delimiters<std::unordered_set<T...>> {
    static constexpr delimiters_values<char> values = {"{", ", ", "}"};
};

template <typename... T>
struct delimiters<std::unordered_multiset<T...>> {
    static constexpr delimiters_values<char> values = {"{", ", ", "}"};
};

// Delimiters for pair and tuple

template <typename... T>
struct delimiters<std::pair<T...>> {
    static constexpr delimiters_values<char> values = {"(", ", ", ")"};
};

template <typename... T>
struct delimiters<std::tuple<T...>> {
    static constexpr delimiters_values<char> values = {"(", ", ", ")"};
};

// Type-erasing helper class for easy use of custom delimiters.
// Requires TCharTraits = std::char_traits<TChar> and TChar = char or wchar_t, and MyDelims needs to be defined for
// TChar. Usage: "cout << pretty_print::custom_delims<MyDelims>(x)".

struct custom_delims_base {
    virtual ~custom_delims_base() {}
    virtual std::ostream &stream(std::ostream &) = 0;
};

template <typename T, typename Delims>
struct custom_delims_wrapper : custom_delims_base {
    custom_delims_wrapper(const T &t_) : t(t_) {}

    std::ostream &stream(std::ostream &s) {
        return s << print_container_helper<T, char, std::char_traits<char>, Delims>(t);
    }

private:
    const T &t;
};

template <typename Delims>
struct custom_delims {
    template <typename Container>
    custom_delims(const Container &c) : base(new custom_delims_wrapper<Container, Delims>(c)) {}

    std::unique_ptr<custom_delims_base> base;
};

template <typename TChar, typename TCharTraits, typename Delims>
inline std::basic_ostream<TChar, TCharTraits> &operator<<(std::basic_ostream<TChar, TCharTraits> &s,
                                                          const custom_delims<Delims> &p) {
    return p.base->stream(s);
}

}  // namespace utils

// Main magic entry point: An overload snuck into namespace std.
// Can we do better?

namespace std {
// Prints a container to the stream using default delimiters

template <typename T, typename TChar, typename TCharTraits>
inline typename enable_if<::utils::is_container<T>::value, basic_ostream<TChar, TCharTraits> &>::type operator<<(
    basic_ostream<TChar, TCharTraits> &stream, const T &container) {
    return stream << ::utils::print_container_helper<T, TChar, TCharTraits>(container);
}

template <typename T>
string to_string_with_precision(const T a_value, const int n = 6) {
    ostringstream out;
    out.precision(n);
    out << fixed << a_value;
    return out.str();
}

}  // namespace std