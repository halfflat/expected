#pragma once

// C++17 version of C++23 std::expected

#include <initializer_list>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

namespace backport {

namespace detail {

template <typename V, typename U, std::size_t index = (std::variant_size_v<std::remove_cv_t<std::remove_reference_t<U>>>-1)>
constexpr V convert_variant(U&& u) {
    if (u.index()==index) return V{std::in_place_index<index>, std::get<index>(std::forward<U>(u))};
    if constexpr (index>0) return convert_variant<V, U, index-1>(std::forward<U>(u));
    else throw std::bad_variant_access{};
}

} // detail


// bad_accepted_access exceptions

template <typename E = void>
struct bad_expected_access;

template <>
struct bad_expected_access<void>: std::exception {
    bad_expected_access() = default;
    virtual const char* what() const noexcept override { return "bad expected access"; }
};

template <typename E>
struct bad_expected_access: public bad_expected_access<void> {
    explicit bad_expected_access(E error): error_(error) {}

    E& error() & { return error_; }
    const E& error() const& { return error_; }
    E&& error() && { return std::move(error_); }
    const E&& error() const&& { return std::move(error_); }

private:
    E error_;
};


// unexpect tag type

struct unexpect_t { explicit unexpect_t() = default; };
inline constexpr unexpect_t unexpect{};


// unexpected class

template <typename E>
struct unexpected {
    template <typename F>
    friend struct unexpected;

    constexpr unexpected(const unexpected&) = default;
    constexpr unexpected(unexpected&&) = default;

    // emplacing constructors

    template <typename F,
        typename = std::enable_if_t<std::is_constructible_v<E, F>>,
        typename = std::enable_if_t<!std::is_same_v<std::in_place_t, std::remove_cv_t<std::remove_reference_t<F>>>>,
        typename = std::enable_if_t<!std::is_same_v<unexpected, std::remove_cv_t<std::remove_reference_t<F>>>>
    >
    constexpr explicit unexpected(F&& f):
        error_(std::forward<F>(f)) {}

    template <typename... As>
    constexpr explicit unexpected(std::in_place_t, As&&... as):
        error_(std::forward<As>(as)...) {}

    template <typename X, typename... As>
    constexpr explicit unexpected(std::in_place_t, std::initializer_list<X> il, As&&... as):
        error_(il, std::forward<As>(as)...) {}

    // access

    constexpr E& error() & { return error_; }
    constexpr const E& error() const& { return error_; }
    constexpr E&& error() && { return std::move(error_); }
    constexpr const E&& error() const&& { return std::move(error_); }

    // comparison

    template <typename F>
    friend constexpr bool operator==(const unexpected x, const backport::unexpected<F>& y) { return x.error()==y.error(); }

    template <typename F>
    friend constexpr bool operator!=(const unexpected x, const backport::unexpected<F>& y) { return x.error()!=y.error(); }

    // swap

    constexpr void swap(unexpected& other) noexcept(std::is_nothrow_swappable_v<E>) {
        using std::swap;
        swap(error_, other.error_);
    }

    friend constexpr void swap(unexpected& a, unexpected& b) noexcept(noexcept(a.swap(b))) { a.swap(b); }

private:
    E error_;
};

template <typename E>
unexpected(E) -> unexpected<E>;


// expected class

template <typename T, typename E, bool = std::is_void_v<T>>
struct expected;

namespace detail {

template <typename A, typename B>
struct is_constructible_from_any_cref:
    std::bool_constant<
        std::is_constructible_v<A, B> ||
        std::is_constructible_v<A, const B> ||
        std::is_constructible_v<A, B&> ||
        std::is_constructible_v<A, const B&>> {};

template <typename A, typename B>
struct is_convertible_from_any_cref:
    std::bool_constant<
        std::is_convertible_v<A, B> ||
        std::is_convertible_v<const A, B> ||
        std::is_convertible_v<A&, B> ||
        std::is_convertible_v<const A&, B>
    > {};

template <typename A, typename B>
inline constexpr bool is_constructible_from_any_cref_v = is_constructible_from_any_cref<A, B>::value;

template <typename A, typename B>
inline constexpr bool is_convertible_from_any_cref_v = is_convertible_from_any_cref<A, B>::value;

template <typename A>
struct is_unexpected: std::false_type {};

template <typename A>
struct is_unexpected<unexpected<A>>: std::true_type {};

template <typename A>
inline constexpr bool is_unexpected_v = is_unexpected<A>::value;

template <typename A>
struct is_expected: std::false_type {};

template <typename A, typename B>
struct is_expected<expected<A, B>>: std::true_type {};

template <typename A>
inline constexpr bool is_expected_v = is_expected<A>::value;

} // namespace detail


// expected class non-void case

template <typename T, typename E>
struct expected<T, E, false> {
    using value_type = T;
    using error_type = E;
    using unexpected_type = unexpected<E>;

    template <typename U>
    using rebind = expected<U, error_type>;

    template <typename, typename, bool>
    friend struct expected;

    constexpr expected() noexcept(std::is_nothrow_default_constructible_v<data_type>) = default;

    constexpr expected(const expected&) = default;
    constexpr expected(expected&&) noexcept(std::is_nothrow_move_constructible_v<data_type>) = default;

    // implicit copy construction from a different expected type
    template <
        typename U,
        typename F,
        typename Ucref = std::add_lvalue_reference_t<const U>,
        typename Fcref = const F&,
        std::enable_if_t<
            std::is_constructible_v<T, Ucref> &&
            std::is_constructible_v<E, Fcref> &&
            (std::is_same_v<bool, std::remove_cv_t<T>> ||
                (!detail::is_constructible_from_any_cref_v<T, expected<U, F>> &&
                !detail::is_convertible_from_any_cref_v<expected<U, F>, T> &&
                !detail::is_constructible_from_any_cref_v<unexpected<E>, expected<U, F>>)),
            int
        > = 0,
        std::enable_if_t<
          std::is_convertible_v<Ucref, T> && std::is_convertible_v<Fcref, E>,
          int
        > = 0
    >
    constexpr expected(const expected<U, F>& other):
        data_(detail::convert_variant<decltype(data_)>(other.data_)) {}

    // explicit copy construction from a different expected type
    template <
        typename U,
        typename F,
        typename Ucref = std::add_lvalue_reference_t<const U>,
        typename Fcref = const F&,
        std::enable_if_t<
            std::is_constructible_v<T, Ucref> &&
            std::is_constructible_v<E, Fcref> &&
            (std::is_same_v<bool, std::remove_cv_t<T>> ||
                (!detail::is_constructible_from_any_cref_v<T, expected<U, F>> &&
                !detail::is_convertible_from_any_cref_v<expected<U, F>, T> &&
                !detail::is_constructible_from_any_cref_v<unexpected<E>, expected<U, F>>)),
            int
        > = 0,
        std::enable_if_t<
            !std::is_convertible_v<Ucref, T> || !std::is_convertible_v<Fcref, E>,
            int
        > = true
    >
    constexpr explicit expected(const expected<U, F>& other):
        data_(detail::convert_variant<decltype(data_)>(other.data_)) {}

    // implicit move construction from a different expected type
    template <
        typename U,
        typename F,
        std::enable_if_t<
            std::is_constructible_v<T, U> &&
            std::is_constructible_v<E, F> &&
            (std::is_same_v<bool, std::remove_cv_t<T>> ||
                (!detail::is_constructible_from_any_cref_v<T, expected<U, F>> &&
                !detail::is_convertible_from_any_cref_v<expected<U, F>, T> &&
                !detail::is_constructible_from_any_cref_v<unexpected<E>, expected<U, F>>)),
            int
        > = 0,
        std::enable_if_t<
            std::is_convertible_v<U, T> && std::is_convertible_v<F, E>,
            int
        > = 0
    >
    constexpr expected(expected<U, F>&& other):
        data_(detail::convert_variant<decltype(data_)>(std::move(other.data_))) {}

    // explicit move construction from a different expected type
    template <
        typename U,
        typename F,
        std::enable_if_t<
            std::is_constructible_v<T, U> &&
            std::is_constructible_v<E, F> &&
            (std::is_same_v<bool, std::remove_cv_t<T>> ||
                (!detail::is_constructible_from_any_cref_v<T, expected<U, F>> &&
                !detail::is_convertible_from_any_cref_v<expected<U, F>, T> &&
                !detail::is_constructible_from_any_cref_v<unexpected<E>, expected<U, F>>)),
            int
        > = 0,
        std::enable_if_t<
            !std::is_convertible_v<U, T> || !std::is_convertible_v<F, E>,
            int
        > = 0
    >
    constexpr explicit expected(expected<U, F>&& other):
        data_(detail::convert_variant<decltype(data_)>(std::move(other.data_))) {}

    // implicit construction from compatible value type
    template <
        typename U,
        std::enable_if_t<
            std::is_constructible_v<T, U> &&
            !std::is_same_v<std::in_place_t, std::remove_cv_t<std::remove_reference_t<U>>> &&
            !std::is_same_v<expected, std::remove_cv_t<std::remove_reference_t<U>>> &&
            !detail::is_unexpected_v<std::remove_cv_t<std::remove_reference_t<U>>> &&
            !(std::is_same_v<bool, std::remove_cv_t<std::remove_reference_t<T>>> && detail::is_expected_v<std::remove_cv_t<std::remove_reference_t<U>>>),
            int
        > = 0,
        std::enable_if_t<std::is_convertible_v<U, T>, int> = 0
    >
    constexpr expected(U&& value):
        data_(std::in_place_index<0>, std::forward<U>(value)) {}

    // explicit construction from compatible value type
    template <
        typename U,
        std::enable_if_t<
            std::is_constructible_v<T, U> &&
            !std::is_same_v<std::in_place_t, std::remove_cv_t<std::remove_reference_t<U>>> &&
            !std::is_same_v<expected, std::remove_cv_t<std::remove_reference_t<U>>> &&
            !detail::is_unexpected_v<std::remove_cv_t<std::remove_reference_t<U>>> &&
            !(std::is_same_v<bool, std::remove_cv_t<std::remove_reference_t<T>>> && detail::is_expected_v<std::remove_cv_t<std::remove_reference_t<U>>>),
            int
        > = 0,
        std::enable_if_t<!std::is_convertible_v<U, T>, int> = 0
    >
    constexpr explicit expected(U&& value):
        data_(std::in_place_index<0>, std::forward<U>(value)) {}

    // implicit copy construction from compatible unexpected type
    template <
        typename F,
        std::enable_if_t<std::is_constructible_v<E, const F&>, int> = 0,
        std::enable_if_t<std::is_convertible_v<F, E>, int> = 0
    >
    constexpr expected(const unexpected<F>& unexp):
        data_(std::in_place_index<1>, unexp.error()) {}

    // explicit copy construction from compatible unexpected type
    template <
        typename F,
        std::enable_if_t<std::is_constructible_v<E, const F&>, int> = 0,
        std::enable_if_t<!std::is_convertible_v<F, E>, int> = 0
    >
    constexpr explicit expected(const unexpected<F>& unexp):
        data_(std::in_place_index<1>, unexp.error()) {}

    // implicit move construction from compatible unexpected type
    template <
        typename F,
        std::enable_if_t<std::is_constructible_v<E, const F&>, int> = 0,
        std::enable_if_t<std::is_convertible_v<const F&, E>, int> = 0
    >
    constexpr expected(unexpected<F>&& unexp):
        data_(std::in_place_index<1>, std::move(unexp.error())) {}

    // explicit move construction from compatible unexpected type
    template <
        typename F,
        std::enable_if_t<std::is_constructible_v<E, F>, int> = 0,
        std::enable_if_t<!std::is_convertible_v<F, E>, int> = 0
    >
    constexpr explicit expected(unexpected<F>&& unexp):
        data_(std::in_place_index<1>, std::move(unexp.error())) {}

    // constructors using in_place_t, unexpect_t
    template <typename... As,
              typename = std::enable_if_t<std::is_constructible_v<T, As...>>>
    constexpr explicit expected(std::in_place_t, As&&... as):
        data_(std::in_place_index<0>, std::forward<As>(as)...) {}

    template <typename X,
              typename... As,
              typename = std::enable_if_t<std::is_constructible_v<T, std::initializer_list<X>&, As...>>>
    constexpr explicit expected(std::in_place_t, std::initializer_list<X> il, As&&... as):
        data_(std::in_place_index<0>, il, std::forward<As>(as)...) {}

    template <typename... As,
              typename = std::enable_if_t<std::is_constructible_v<E, As...>>>
    constexpr explicit expected(unexpect_t, As&&... as):
        data_(std::in_place_index<1>, std::forward<As>(as)...) {}

    template <typename X, typename... As, typename = std::enable_if_t<std::is_constructible_v<E, std::initializer_list<X>&, As...>>>
    constexpr explicit expected(unexpect_t, std::initializer_list<X> il, As&&... as):
        data_(std::in_place_index<1>, il, std::forward<As>(as)...) {}

    // assignment

    constexpr expected& operator=(const expected&) = default;

    constexpr expected& operator=(expected&&)
        noexcept(std::is_nothrow_move_constructible_v<T> &&
                 std::is_nothrow_move_constructible_v<E> &&
                 std::is_nothrow_move_assignable_v<T> &&
                 std::is_nothrow_move_assignable_v<E>) = default;


    template <
        typename U,
        std::enable_if_t<!std::is_same_v<expected, std::remove_cv_t<std::remove_reference_t<U>>>, int> = 0,
        std::enable_if_t<!detail::is_unexpected_v<std::remove_cv_t<std::remove_reference_t<U>>>, int> = 0,
        std::enable_if_t<std::is_constructible_v<T, U>, int> = 0,
        std::enable_if_t<std::is_assignable_v<T&, U>, int> = 0
    >
    constexpr expected& operator=(U&& other) {
        if (has_value()) std::get<0>(data_) = std::forward<U>(other);
        else data_.template emplace<0>(std::forward<U>(other));
        return *this;
    }

    template <
        typename G,
        std::enable_if_t<std::is_constructible_v<E, const G&>, int> = 0,
        std::enable_if_t<std::is_assignable_v<E&, const G&>, int> =0
    >
    constexpr expected& operator=(const unexpected<G>& unexp) {
        if (!has_value()) std::get<1>(data_) = unexp.error();
        else data_.template emplace<1>(unexp.error());
        return *this;
    }

    template <
        typename G,
        std::enable_if_t<std::is_constructible_v<E, G>, int> = 0,
        std::enable_if_t<std::is_assignable_v<E&, G>, int> =0
    >
    constexpr expected& operator=(unexpected<G>&& unexp) {
        if (!has_value()) std::get<1>(data_) = std::move(unexp).error();
        else data_.template emplace<1>(std::move(unexp).error());
        return *this;
    }

    // access methods

    explicit operator bool() const noexcept { return data_.index()==0; }
    bool has_value() const noexcept { return data_.index()==0; }

    E& error() & { return std::get<1>(data_); }
    const E& error() const& { return std::get<1>(data_); }
    E&& error() && { return std::get<1>(std::move(data_)); }
    const E&& error() const&& { return std::get<1>(std::move(data_)); }

    T* operator->() noexcept { return std::get_if<0>(data_); }
    const T* operator->() const noexcept { return std::get_if<0>(data_); }

    T& operator*() & noexcept { return std::get<0>(data_); }
    const T& operator*() const& noexcept { return std::get<0>(data_); }
    T&& operator*() && noexcept { return std::get<0>(std::move(data_)); }
    const T&& operator*() const&& noexcept { return std::get<0>(std::move(data_)); }

    T& value() & {
        return *this? std::get<0>(data_): throw bad_expected_access(std::as_const(error()));
    }

    const T& value() const& {
        return *this? std::get<0>(data_): throw bad_expected_access(std::as_const(error()));
    }

    T&& value() && {
        return *this? std::get<0>(std::move(data_)): throw bad_expected_access(std::move(error()));
    }

    const T&& value() const&& {
        return *this? std::get<0>(std::move(data_)): throw bad_expected_access(std::move(error()));
    }

    template <typename U>
    constexpr T value_or(U&& alt) const& {
        if (has_value()) return **this;
        return std::forward<U>(alt);
    }

    template <typename U>
    constexpr T value_or(U&& alt) && {
        if (has_value()) return std::move(**this);
        return std::forward<U>(alt);
    }

    template <typename U>
    constexpr E error_or(U&& alt) const& {
        if (has_value()) return std::forward<U>(alt);
        return error();
    }

    template <typename U>
    constexpr E error_or(U&& alt) && {
        if (has_value()) return std::forward<U>(alt);
        return std::move(error());
    }

    // comparison operations

    template <typename U, typename F, std::enable_if_t<!std::is_void_v<F>, int> = 0>
    friend constexpr bool operator==(const expected& x, const backport::expected<U, F>& y) {
        return x.data_ == y.data_;
    }

    template <typename U, std::enable_if_t<!detail::is_expected_v<U> && !detail::is_unexpected_v<U>, int> = 0>
    friend constexpr bool operator==(const expected& x, const U& y) {
        return x.has_value() && *x == y;
    }

    template <typename F>
    friend constexpr bool operator==(const expected& x, const backport::unexpected<F>& y) {
        return !x.has_value() && x.error() == y.error();
    }

#if __cplusplus < 202002L
    template <typename U, typename F, std::enable_if_t<!std::is_void_v<F>, int> = 0>
    friend constexpr bool operator!=(const expected& x, const backport::expected<U, F>& y) {
        return !(x==y);
    }

    template <typename U, std::enable_if_t<!detail::is_expected_v<U> && !detail::is_unexpected_v<U>, int> = 0>
    friend constexpr bool operator!=(const expected& x, const U& y) {
        return !(x==y);
    }

    template <typename F>
    friend constexpr bool operator!=(const expected& x, const backport::unexpected<F>& y) {
        return !(x==y);
    }
#endif

    // monadic operations

    template <typename F>
    auto and_then(F&& f) & {
        using R = std::remove_cv_t<std::remove_reference_t<std::invoke_result_t<F, T&>>>;
        return *this? std::invoke(std::forward<F>(f), **this): R(unexpect, error());
    }

    template <typename F>
    auto and_then(F&& f) const& {
        using R = std::remove_cv_t<std::remove_reference_t<std::invoke_result_t<F, const T&>>>;
        return *this? std::invoke(std::forward<F>(f), **this): R(unexpect, error());
    }

    template <typename F>
    auto and_then(F&& f) && {
        using R = std::remove_cv_t<std::remove_reference_t<std::invoke_result_t<F, T&&>>>;
        return *this? std::invoke(std::forward<F>(f), std::move(**this)): R(unexpect, std::move(error()));
    }

    template <typename F>
    auto and_then(F&& f) const&& {
        using R = std::remove_cv_t<std::remove_reference_t<std::invoke_result_t<F, const T&&>>>;
        return *this? std::invoke(std::forward<F>(f), std::move(**this)): R(unexpect, std::move(error()));
    }

    template <typename F>
    auto or_else(F&& f) & {
        using R = std::remove_cv_t<std::remove_reference_t<std::invoke_result_t<F, E&>>>;
        return *this? R(std::in_place, **this): std::invoke(std::forward<F>(f), error());
    }

    template <typename F>
    auto or_else(F&& f) const& {
        using R = std::remove_cv_t<std::remove_reference_t<std::invoke_result_t<F, const E&>>>;
        return *this? R(std::in_place, **this): std::invoke(std::forward<F>(f), error());
    }

    template <typename F>
    auto or_else(F&& f) && {
        using R = std::remove_cv_t<std::remove_reference_t<std::invoke_result_t<F, E&&>>>;
        return *this? R(std::in_place, std::move(**this)): std::invoke(std::forward<F>(f), std::move(error()));
    }

    template <typename F>
    auto or_else(F&& f) const&& {
        using R = std::remove_cv_t<std::remove_reference_t<std::invoke_result_t<F, const E&&>>>;
        return *this? R(std::in_place, std::move(**this)): std::invoke(std::forward<F>(f), std::move(error()));
    }

    template <typename F>
    auto transform(F&& f) & {
        using U = std::remove_cv_t<std::invoke_result_t<F, T&>>;
        if constexpr (std::is_void_v<U>)
            return *this? std::invoke(std::forward<F>(f), **this), expected<U, E>(): expected<U, E>(unexpect, error());
        else
            return *this? expected<U, E>(std::in_place, std::invoke(std::forward<F>(f), **this)): expected<U, E>(unexpect, error());
    }

    template <typename F>
    auto transform(F&& f) const& {
        using U = std::remove_cv_t<std::invoke_result_t<F, const T&>>;
        if constexpr (std::is_void_v<U>)
            return *this? std::invoke(std::forward<F>(f), **this), expected<U, E>(): expected<U, E>(unexpect, error());
        else
            return *this? expected<U, E>(std::in_place, std::invoke(std::forward<F>(f), **this)): expected<U, E>(unexpect, error());
    }

    template <typename F>
    auto transform(F&& f) && {
        using U = std::remove_cv_t<std::invoke_result_t<F, T&&>>;
        if constexpr (std::is_void_v<U>)
            return *this? std::invoke(std::forward<F>(f), std::move(**this)), expected<U, E>(): expected<U, E>(unexpect, std::move(error()));
        else
            return *this? expected<U, E>(std::in_place, std::invoke(std::forward<F>(f), std::move(**this))): expected<U, E>(unexpect, std::move(error()));
    }

    template <typename F>
    auto transform(F&& f) const&& {
        using U = std::remove_cv_t<std::invoke_result_t<F, const T&&>>;
        if constexpr (std::is_void_v<U>)
            return *this? std::invoke(std::forward<F>(f), std::move(**this)), expected<U, E>(): expected<U, E>(unexpect, std::move(error()));
        else
            return *this? expected<U, E>(std::in_place, std::invoke(std::forward<F>(f), std::move(**this))): expected<U, E>(unexpect, std::move(error()));
    }

    template <typename F>
    auto transform_error(F&& f) & {
        using R = expected<T, std::remove_cv_t<std::invoke_result_t<F, E&>>>;
        return *this? R(std::in_place, **this): R(unexpect, std::invoke(std::forward<F>(f), error()));
    }

    template <typename F>
    auto transform_error(F&& f) const& {
        using R = expected<T, std::remove_cv_t<std::invoke_result_t<F, const E&>>>;
        return *this? R(std::in_place, **this): R(unexpect, std::invoke(std::forward<F>(f), error()));
    }

    template <typename F>
    auto transform_error(F&& f) && {
        using R = expected<T, std::remove_cv_t<std::invoke_result_t<F, E&&>>>;
        return *this? R(std::in_place, std::move(**this)): R(unexpect, std::invoke(std::forward<F>(f), std::move(error())));
    }

    template <typename F>
    auto transform_error(F&& f) const&& {
        using R = expected<T, std::remove_cv_t<std::invoke_result_t<F, const E&&>>>;
        return *this? R(std::in_place, std::move(**this)): R(unexpect, std::invoke(std::forward<F>(f), std::move(error())));
    }

    // emplace expected value

    template <
        typename... As,
        std::enable_if_t<std::is_nothrow_constructible_v<T, As...>, int> = 0
    >
    T& emplace(As&&... as) noexcept { return data_.template emplace<0>(std::forward<As>(as)...); }

    template <
        typename X,
        typename... As,
        std::enable_if_t<std::is_nothrow_constructible_v<T, std::initializer_list<X>&, As...>, int> = 0
    >
    T& emplace(std::initializer_list<X> il, As&&... as) noexcept { return data_.template emplace<0>(il, std::forward<As>(as)...); }

    // swap

    void swap(expected& other) noexcept(std::is_nothrow_swappable_v<data_type>) {
        using std::swap;
        swap(data_, other.data_);
    }

    friend void swap(expected& a, expected& b) noexcept(noexcept(a.swap(b))) { a.swap(b); }

private:
    using data_type = std::variant<T, E>;
    data_type data_;
};


// expected class void case

template <typename T, typename E>
struct expected<T, E, true> {
    using value_type = T; // TS is possibly cv-qualified void
    using error_type = E;
    using unexpected_type = unexpected<E>;

    template <typename U>
    using rebind = expected<U, error_type>;

    template <typename, typename, bool>
    friend struct expected;

    constexpr expected() noexcept(std::is_nothrow_default_constructible_v<data_type>) = default;
    constexpr expected(const expected&) = default;
    constexpr expected(expected&&) noexcept(std::is_nothrow_move_constructible_v<data_type>) = default;

    // implicit copy construction from a different expected type
    template <
        typename U,
        typename F,
        std::enable_if_t<
            std::is_void_v<U> &&
            std::is_constructible_v<E, const F&> &&
            !detail::is_constructible_from_any_cref_v<unexpected<E>, expected<U, F>>,
            int
        > = 0,
        std::enable_if_t<std::is_convertible_v<const F&, E>, int> = 0
    >
    constexpr expected(const expected<U, F>& other):
        data_(other.data_) {}

    // explicit copy construction from a different expected type
    template <
        typename U,
        typename F,
        std::enable_if_t<
            std::is_void_v<U> &&
            std::is_constructible_v<E, const F&> &&
            !detail::is_constructible_from_any_cref_v<unexpected<E>, expected<U, F>>,
            int
        > = 0,
        std::enable_if_t<!std::is_convertible_v<const F&, E>, int> = 0
    >
   constexpr explicit expected(const expected<U, F>& other):
        data_(other.data_) {}


    // implicit move construction from a different expected type
    template <
        typename U,
        typename F,
        std::enable_if_t<
            std::is_void_v<U> &&
            std::is_constructible_v<E, F&&> &&
            !detail::is_constructible_from_any_cref_v<unexpected<E>, expected<U, F>>,
            int
        > = 0,
        std::enable_if_t<std::is_convertible_v<F&&, E>, int> = 0
    >
    constexpr expected(expected<U, F>&& other):
        data_(std::move(other.data_)) {}

    // explicit move construction from a different expected type
    template <
        typename U,
        typename F,
        std::enable_if_t<
            std::is_void_v<U> &&
            std::is_constructible_v<E, F&&> &&
            !detail::is_constructible_from_any_cref_v<unexpected<E>, expected<U, F>>,
            int
        > = 0,
        std::enable_if_t<!std::is_convertible_v<F&&, E>, int> = 0
    >
   constexpr explicit expected(expected<U, F>&& other):
        data_(std::move(other.data_)) {}

    // implicit copy construction from compatible unexpected type
    template <
        typename F,
        std::enable_if_t<std::is_constructible_v<E, const F&>, int> = 0,
        std::enable_if_t<std::is_convertible_v<const F&, E>, int> = 0
    >
    constexpr expected(const unexpected<F>& unexp):
        data_(std::in_place, unexp.error()) {}

    // explicit copy construction from compatible unexpected type
    template <
        typename F,
        std::enable_if_t<std::is_constructible_v<E, const F&>, int> = 0,
        std::enable_if_t<!std::is_convertible_v<const F&, E>, int> = 0
    >
    constexpr explicit expected(const unexpected<F>& unexp):
        data_(std::in_place, unexp.error()) {}

    // implicit move construction from compatible unexpected type
    template <
        typename F,
        std::enable_if_t<std::is_constructible_v<E, F>, int> = 0,
        std::enable_if_t<std::is_convertible_v<F, E>, int> = 0
    >
    constexpr expected(unexpected<F>&& unexp):
        data_(std::in_place, std::move(unexp.error())) {}

    // explicit move construction from compatible unexpected type
    template <
        typename F,
        std::enable_if_t<std::is_constructible_v<E, F>, int> = 0,
        std::enable_if_t<!std::is_convertible_v<F, E>, int> = 0
    >
    constexpr explicit expected(unexpected<F>&& unexp):
        data_(std::in_place, std::move(unexp.error())) {}

    // constructors using in_place_t, unexpect_t
    constexpr explicit expected(std::in_place_t) {}

    template <typename... As,
              typename = std::enable_if_t<std::is_constructible_v<E, As...>>>
    constexpr explicit expected(unexpect_t, As&&... as):
        data_(std::in_place, std::forward<As>(as)...) {}

    template <typename X, typename... As, typename = std::enable_if_t<std::is_constructible_v<E, std::initializer_list<X>&, As...>>>
    constexpr explicit expected(unexpect_t, std::initializer_list<X> il, As&&... as):
        data_(std::in_place, il, std::forward<As>(as)...) {}

    // assignment

    constexpr expected& operator=(const expected&) = default;

    constexpr expected& operator=(expected&&)
        noexcept(std::is_nothrow_move_constructible_v<E> &&
                 std::is_nothrow_move_assignable_v<E>) = default;

    template <
        typename G,
        std::enable_if_t<std::is_constructible_v<E, const G&>, int> = 0,
        std::enable_if_t<std::is_assignable_v<E&, const G&>, int> =0
    >
    constexpr expected& operator=(const unexpected<G>& unexp) {
        data_ = unexp.error();
        return *this;
    }

    template <
        typename G,
        std::enable_if_t<std::is_constructible_v<E, G>, int> = 0,
        std::enable_if_t<std::is_assignable_v<E&, G>, int> =0
    >
    constexpr expected& operator=(unexpected<G>&& unexp) {
        data_ = std::move(unexp).error();
        return *this;
    }

    // access methods

    constexpr explicit operator bool() const noexcept { return has_value(); }
    constexpr bool has_value() const noexcept { return !data_.has_value(); }

    constexpr E& error() & { return *data_; }
    constexpr const E& error() const& { return *data_; }
    constexpr E&& error() && { return *std::move(data_); }
    constexpr const E&& error() const&& { return *std::move(data_); }

    constexpr void value() const& { if (!has_value()) throw bad_expected_access(std::as_const(error())); }
    constexpr void value() && { if (!has_value()) throw bad_expected_access(std::move(error())); }

    constexpr void operator*() const noexcept {}

    template <typename U>
    constexpr E error_or(U&& alt) const& {
        if (has_value()) return std::forward<U>(alt);
        return error();
    }

    template <typename U>
    constexpr E error_or(U&& alt) && {
        if (has_value()) return std::forward<U>(alt);
        return std::move(error());
    }

    // comparison operations

    template <typename U, typename F, std::enable_if_t<std::is_void_v<U>, int> = 0>
    friend constexpr bool operator==(const expected& x, const backport::expected<U, F>& y) {
        return x.data_ == y.data_;
    }

    template <typename F>
    friend constexpr bool operator==(const expected& x, const backport::unexpected<F>& y) {
        return !x.has_value() && x.error() == y.error();
    }

#if __cplusplus < 202002L
    template <typename U, typename F, std::enable_if_t<std::is_void_v<U>, int> = 0>
    friend constexpr bool operator!=(const expected& x, const backport::expected<U, F>& y) {
        return !(x==y);
    }

    template <typename F>
    friend constexpr bool operator!=(const expected& x, const backport::unexpected<F>& y) {
        return !(x==y);
    }
#endif

    // monadic operations

    template <typename F>
    auto and_then(F&& f) & {
        using R = std::remove_cv_t<std::remove_reference_t<std::invoke_result_t<F>>>;
        return *this? std::invoke(std::forward<F>(f)): R(unexpect, error());
    }

    template <typename F>
    auto and_then(F&& f) const& {
        using R = std::remove_cv_t<std::remove_reference_t<std::invoke_result_t<F>>>;
        return *this? std::invoke(std::forward<F>(f)): R(unexpect, error());
    }

    template <typename F>
    auto and_then(F&& f) && {
        using R = std::remove_cv_t<std::remove_reference_t<std::invoke_result_t<F>>>;
        return *this? std::invoke(std::forward<F>(f)): R(unexpect, std::move(error()));
    }

    template <typename F>
    auto and_then(F&& f) const&& {
        using R = std::remove_cv_t<std::remove_reference_t<std::invoke_result_t<F>>>;
        return *this? std::invoke(std::forward<F>(f)): R(unexpect, std::move(error()));
    }

    template <typename F>
    auto or_else(F&& f) & {
        using R = std::remove_cv_t<std::remove_reference_t<std::invoke_result_t<F, E&>>>;
        return *this? R(): std::invoke(std::forward<F>(f), error());
    }

    template <typename F>
    auto or_else(F&& f) const& {
        using R = std::remove_cv_t<std::remove_reference_t<std::invoke_result_t<F, const E&>>>;
        return *this? R(): std::invoke(std::forward<F>(f), error());
    }

    template <typename F>
    auto or_else(F&& f) && {
        using R = std::remove_cv_t<std::remove_reference_t<std::invoke_result_t<F, E&&>>>;
        return *this? R(): std::invoke(std::forward<F>(f), std::move(error()));
    }

    template <typename F>
    auto or_else(F&& f) const&& {
        using R = std::remove_cv_t<std::remove_reference_t<std::invoke_result_t<F, const E&&>>>;
        return *this? R(): std::invoke(std::forward<F>(f), std::move(error()));
    }

    template <typename F>
    auto transform(F&& f) & {
        using U = std::remove_cv_t<std::invoke_result_t<F>>;
        if constexpr (std::is_void_v<U>)
            return *this? std::invoke(std::forward<F>(f)), expected<U, E>(): expected<U, E>(unexpect, error());
        else
            return *this? expected<U, E>(std::in_place, std::invoke(std::forward<F>(f))): expected<U, E>(unexpect, error());
    }

    template <typename F>
    auto transform(F&& f) const& {
        using U = std::remove_cv_t<std::invoke_result_t<F>>;
        if constexpr (std::is_void_v<U>)
            return *this? std::invoke(std::forward<F>(f)), expected<U, E>(): expected<U, E>(unexpect, error());
        else
            return *this? expected<U, E>(std::in_place, std::invoke(std::forward<F>(f))): expected<U, E>(unexpect, error());
    }

    template <typename F>
    auto transform(F&& f) && {
        using U = std::remove_cv_t<std::invoke_result_t<F>>;
        if constexpr (std::is_void_v<U>)
            return *this? std::invoke(std::forward<F>(f)), expected<U, E>(): expected<U, E>(unexpect, std::move(error()));
        else
            return *this? expected<U, E>(std::in_place, std::invoke(std::forward<F>(f))): expected<U, E>(unexpect, std::move(error()));
    }

    template <typename F>
    auto transform(F&& f) const&& {
        using U = std::remove_cv_t<std::invoke_result_t<F>>;
        if constexpr (std::is_void_v<U>)
            return *this? std::invoke(std::forward<F>(f)), expected<U, E>(): expected<U, E>(unexpect, std::move(error()));
        else
            return *this? expected<U, E>(std::in_place, std::invoke(std::forward<F>(f))): expected<U, E>(unexpect, std::move(error()));
    }

    template <typename F>
    auto transform_error(F&& f) & {
        using R = expected<T, std::remove_cv_t<std::invoke_result_t<F, E&>>>;
        return *this? R(): R(unexpect, std::invoke(std::forward<F>(f), error()));
    }

    template <typename F>
    auto transform_error(F&& f) const& {
        using R = expected<T, std::remove_cv_t<std::invoke_result_t<F, const E&>>>;
        return *this? R(): R(unexpect, std::invoke(std::forward<F>(f), error()));
    }

    template <typename F>
    auto transform_error(F&& f) && {
        using R = expected<T, std::remove_cv_t<std::invoke_result_t<F, E&&>>>;
        return *this? R(): R(unexpect, std::invoke(std::forward<F>(f), std::move(error())));
    }

    template <typename F>
    auto transform_error(F&& f) const&& {
        using R = expected<T, std::remove_cv_t<std::invoke_result_t<F, const E&&>>>;
        return *this? R(): R(unexpect, std::invoke(std::forward<F>(f), std::move(error())));
    }

    // emplace expected value

    constexpr void emplace() noexcept { data_.reset(); }

    // swap

    void swap(expected& other) noexcept(std::is_nothrow_swappable_v<data_type>) {
        using std::swap;
        swap(data_, other.data_);
    }

    friend void swap(expected& a, expected& b) noexcept(noexcept(a.swap(b))) { a.swap(b); }

private:
    using data_type = std::optional<error_type>;
    data_type data_;
};

} // namespace backport
