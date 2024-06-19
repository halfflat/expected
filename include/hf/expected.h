#pragma once

// C++17 version of C++23 std::expected; missing constexpr methods.

#include <initializer_list>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

namespace hf {

namespace detail {

// (std::remove_cvref is provided in C++20)
template <typename X>
struct remove_cvref {
    using type = std::remove_cv_t<std::remove_reference_t<X>>;
};

template <typename X>
using remove_cvref_t = typename remove_cvref<X>::type;

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

struct unexpect_t {};
inline constexpr unexpect_t unexpect{};


// unexpected class

template <typename E>
struct unexpected {
    template <typename F>
    friend struct unexpected;

    unexpected(const unexpected&) = default;
    unexpected(unexpected&&) = default;

    // emplacing constructors

    template <typename F,
        typename = std::enable_if_t<std::is_constructible_v<E, F>>,
        typename = std::enable_if_t<!std::is_same_v<std::in_place_t, detail::remove_cvref_t<F>>>,
        typename = std::enable_if_t<!std::is_same_v<unexpected, detail::remove_cvref_t<F>>>
    >
    explicit unexpected(F&& f):
        error_(std::forward<F>(f)) {}

    template <typename... As>
    explicit unexpected(std::in_place_t, As&&... as):
        error_(std::forward<As>(as)...) {}

    template <typename X, typename... As>
    explicit unexpected(std::in_place_t, std::initializer_list<X> il, As&&... as):
        error_(il, std::forward<As>(as)...) {}

    // access

    E& error() & { return error_; }
    const E& error() const& { return error_; }
    E&& error() && { return std::move(error_); }
    const E&& error() const&& { return std::move(error_); }

    // comparison

    template <typename F>
    friend bool operator==(const unexpected x, const hf::unexpected<F>& y) { return x.error()==y.error(); }

    template <typename F>
    friend bool operator!=(const unexpected x, const hf::unexpected<F>& y) { return x.error()!=y.error(); }

    // swap

    void swap(unexpected& other) noexcept(std::is_nothrow_swappable_v<E>) {
        using std::swap;
        swap(error_, other.error_);
    }

    friend void swap(unexpected& a, unexpected& b) noexcept(noexcept(a.swap(b))) { a.swap(b); }

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

    expected() noexcept(std::is_nothrow_default_constructible_v<data_type>) = default;
    expected(const expected&) = default;
    expected(expected&&) noexcept(std::is_nothrow_move_constructible_v<data_type>) = default;

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
                !detail::is_constructible_from_any_cref_v<T, expected<U, F>> &&
                !detail::is_convertible_from_any_cref_v<expected<U, F>, T> &&
                !detail::is_constructible_from_any_cref_v<unexpected<E>, expected<U, F>>),
            bool
        > = true,
        std::enable_if_t<
          std::is_convertible_v<Ucref, T> && std::is_convertible_v<Fcref, E>,
          bool
        > = true
    >
    expected(const expected<U, F>& other):
        data_(other.data_) {}

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
                !detail::is_constructible_from_any_cref_v<T, expected<U, F>> &&
                !detail::is_convertible_from_any_cref_v<expected<U, F>, T> &&
                !detail::is_constructible_from_any_cref_v<unexpected<E>, expected<U, F>>),
            bool
        > = true,
        std::enable_if_t<
            !std::is_convertible_v<Ucref, T> || !std::is_convertible_v<Fcref, E>,
            bool
        > = true
    >
    explicit expected(const expected<U, F>& other):
        data_(other.data_) {}

    // implicit move construction from a different expected type
    template <
        typename U,
        typename F,
        std::enable_if_t<
            std::is_constructible_v<T, U> &&
            std::is_constructible_v<E, F> &&
            (std::is_same_v<bool, std::remove_cv_t<T>> ||
                !detail::is_constructible_from_any_cref_v<T, expected<U, F>> &&
                !detail::is_convertible_from_any_cref_v<expected<U, F>, T> &&
                !detail::is_constructible_from_any_cref_v<unexpected<E>, expected<U, F>>),
            bool
        > = true,
        std::enable_if_t<
            std::is_convertible_v<U, T> && std::is_convertible_v<F, E>,
            bool
        > = true
    >
    expected(expected<U, F>&& other):
        data_(std::move(other.data_)) {}

    // explicit move construction from a different expected type
    template <
        typename U,
        typename F,
        std::enable_if_t<
            std::is_constructible_v<T, U> &&
            std::is_constructible_v<E, F> &&
            (std::is_same_v<bool, std::remove_cv_t<T>> ||
                !detail::is_constructible_from_any_cref_v<T, expected<U, F>> &&
                !detail::is_convertible_from_any_cref_v<expected<U, F>, T> &&
                !detail::is_constructible_from_any_cref_v<unexpected<E>, expected<U, F>>),
            bool
        > = true,
        std::enable_if_t<
            !std::is_convertible_v<U, T> || !std::is_convertible_v<F, E>,
            bool
        > = true
    >
    explicit expected(const expected<U, F>& other):
        data_(std::move(other.data_)) {}

    // implicit construction from compatible value type
    template <
        typename U,
        std::enable_if_t<
            std::is_constructible_v<T, U> &&
            !std::is_same_v<std::in_place_t, detail::remove_cvref_t<U>> &&
            !std::is_same_v<expected, detail::remove_cvref_t<U>> &&
            !detail::is_unexpected_v<detail::remove_cvref_t<U>> &&
            !(std::is_same_v<bool, detail::remove_cvref_t<T>> && detail::is_expected_v<detail::remove_cvref_t<U>>),
            bool
        > = true,
        std::enable_if_t<std::is_convertible_v<U, T>, bool> = true
    >
    expected(U&& value):
        data_(std::in_place_index<0>, std::forward<U>(value)) {}

    // explicit construction from compatible value type
    template <
        typename U,
        std::enable_if_t<
            std::is_constructible_v<T, U> &&
            !std::is_same_v<std::in_place_t, detail::remove_cvref_t<U>> &&
            !std::is_same_v<expected, detail::remove_cvref_t<U>> &&
            !detail::is_unexpected_v<detail::remove_cvref_t<U>> &&
            !(std::is_same_v<bool, detail::remove_cvref_t<T>> && detail::is_expected_v<detail::remove_cvref_t<U>>),
            bool
        > = true,
        std::enable_if_t<!std::is_convertible_v<U, T>, bool> = true
    >
    explicit expected(U&& value):
        data_(std::in_place_index<0>, std::forward<U>(value)) {}

    // implicit copy construction from compatible unexpected type
    template <
        typename F,
        std::enable_if_t<std::is_constructible_v<E, const F&>> = true,
        std::enable_if_t<std::is_convertible_v<F, E>, bool> = true
    >
    expected(const unexpected<F>& unexp):
        data_(std::in_place_index<1>, unexp) {}

    // explicit copy construction from compatible unexpected type
    template <
        typename F,
        std::enable_if_t<std::is_constructible_v<E, const F&>> = true,
        std::enable_if_t<!std::is_convertible_v<F, E>, bool> = true
    >
    explicit expected(const unexpected<F>& unexp):
        data_(std::in_place_index<1>, unexp) {}

    // implicit move construction from compatible unexpected type
    template <
        typename F,
        std::enable_if_t<std::is_constructible_v<E, const F&>> = true,
        std::enable_if_t<std::is_convertible_v<const F&, E>, bool> = true
    >
    expected(unexpected<F>&& unexp):
        data_(std::in_place_index<1>, std::move(unexp)) {}

    // explicit move construction from compatible unexpected type
    template <
        typename F,
        std::enable_if_t<std::is_constructible_v<E, F>> = true,
        std::enable_if_t<!std::is_convertible_v<F, E>, bool> = true
    >
    explicit expected(const unexpected<F>& unexp):
        data_(std::in_place_index<1>, unexp) {}

    // constructors using in_place_t, unexpect_t
    template <typename... As,
              typename = std::enable_if_t<std::is_constructible_v<T, As...>>>
    explicit expected(std::in_place_t, As&&... as):
        data_(std::in_place_index<0>, std::forward<As>(as)...) {}

    template <typename X,
              typename... As,
              typename = std::enable_if_t<std::is_constructible_v<T, std::initializer_list<X>&, As...>>>
    explicit expected(std::in_place_t, std::initializer_list<X> il, As&&... as):
        data_(std::in_place_index<0>, il, std::forward<As>(as)...) {}

    template <typename... As,
              typename = std::enable_if_t<std::is_constructible_v<E, As...>>>
    explicit expected(unexpect_t, As&&... as):
        data_(std::in_place_index<1>, std::in_place_t{}, std::forward<As>(as)...) {}

    template <typename X, typename... As, typename = std::enable_if_t<std::is_constructible_v<E, std::initializer_list<X>&, As...>>>
    explicit expected(unexpect_t, std::initializer_list<X> il, As&&... as):
        data_(std::in_place_index<1>, std::in_place_t{}, il, std::forward<As>(as)...) {}

    // access methods

    explicit operator bool() const noexcept { return data_.index()==0; }
    bool has_value() const noexcept { return data_.index()==0; }

    E& error() & { return std::get<1>(data_).error(); }
    const E& error() const& { return std::get<1>(data_).error(); }
    E&& error() && { return std::get<1>(std::move(data_)).error(); }
    const E&& error() const&& { return std::get<1>(std::move(data_)).error(); }

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
        return *this? std::get<0>(std::move(data_)): throw bad_expected_access(std::as_const(error()));
    }

    const T&& value() const&& {
        return *this? std::get<0>(std::move(data_)): throw bad_expected_access(std::as_const(error()));
    }

    template <typename U>
    T value_or(U&& alt) const& {
        return *this? **this: static_cast<T>(std::forward<U>(alt));
    }

    template <typename U>
    T value_or(U&& alt) && {
        return *this? std::move(**this): static_cast<T>(std::forward<U>(alt));
    }

    // monadic operations

    template <typename F>
    auto and_then(F&& f) & {
        using R = detail::remove_cvref_t<std::invoke_result_t<F, T&>>;
        return *this? std::invoke(std::forward<F>(f), **this): R(unexpect, error());
    }

    template <typename F>
    auto and_then(F&& f) const& {
        using R = detail::remove_cvref_t<std::invoke_result_t<F, const T&>>;
        return *this? std::invoke(std::forward<F>(f), **this): R(unexpect, error());
    }

    template <typename F>
    auto and_then(F&& f) && {
        using R = detail::remove_cvref_t<std::invoke_result_t<F, T&&>>;
        return *this? std::invoke(std::forward<F>(f), std::move(**this)): R(unexpect, std::move(error()));
    }

    template <typename F>
    auto and_then(F&& f) const&& {
        using R = detail::remove_cvref_t<std::invoke_result_t<F, const T&&>>;
        return *this? std::invoke(std::forward<F>(f), std::move(**this)): R(unexpect, std::move(error()));
    }

    template <typename F>
    auto or_else(F&& f) & {
        using R = detail::remove_cvref_t<std::invoke_result_t<F, E&>>;
        return *this? R(std::in_place, **this): std::invoke(std::forward<F>(f), error());
    }

    template <typename F>
    auto or_else(F&& f) const& {
        using R = detail::remove_cvref_t<std::invoke_result_t<F, const E&>>;
        return *this? R(std::in_place, **this): std::invoke(std::forward<F>(f), error());
    }

    template <typename F>
    auto or_else(F&& f) && {
        using R = detail::remove_cvref_t<std::invoke_result_t<F, E&&>>;
        return *this? R(std::in_place, std::move(**this)): std::invoke(std::forward<F>(f), std::move(error()));
    }

    template <typename F>
    auto or_else(F&& f) const&& {
        using R = detail::remove_cvref_t<std::invoke_result_t<F, const E&&>>;
        return *this? R(std::in_place, std::move(**this)): std::invoke(std::forward<F>(f), std::move(error()));
    }

    template <typename F>
    auto transform(F&& f) & {
        using R = expected<std::remove_cv_t<std::invoke_result_t<F, T&>>, E>;
        return *this? R(std::in_place, std::invoke(std::forward<F>(f), **this)): R(unexpect, error());
    }

    template <typename F>
    auto transform(F&& f) const& {
        using R = expected<std::remove_cv_t<std::invoke_result_t<F, const T&>>, E>;
        return *this? R(std::in_place, std::invoke(std::forward<F>(f), **this)): R(unexpect, error());
    }

    template <typename F>
    auto transform(F&& f) && {
        using R = expected<std::remove_cv_t<std::invoke_result_t<F, T&&>>, E>;
        return *this? R(std::in_place, std::invoke(std::forward<F>(f), std::move(**this))): R(unexpect, std::move(error()));
    }

    template <typename F>
    auto transform(F&& f) const&& {
        using R = expected<std::remove_cv_t<std::invoke_result_t<F, const T&&>>, E>;
        return *this? R(std::in_place, std::invoke(std::forward<F>(f), std::move(**this))): R(unexpect, std::move(error()));
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
        std::enable_if_t<std::is_nothrow_constructible_v<T, As...>, bool> = true
    >
    T& emplace(As&&... as) noexcept { return data_.template emplace<0>(std::forward<As>(as)...); }

    template <
        typename X,
        typename... As,
        std::enable_if_t<std::is_nothrow_constructible_v<T, std::initializer_list<X>&, As...>, bool> = true
    >
    T& emplace(std::initializer_list<X> il, As&&... as) noexcept { return data_.template emplace<0>(il, std::forward<As>(as)...); }

    // swap

    void swap(expected& other) noexcept(std::is_nothrow_swappable_v<data_type>) {
        using std::swap;
        swap(data_, other.data_);
    }

    friend void swap(expected& a, expected& b) noexcept(noexcept(a.swap(b))) { a.swap(b); }

private:
    using data_type = std::variant<T, unexpected_type>;
    data_type data_;
};


// expected class void case

template <typename T, typename E>
struct expected<T, E, true> {
    using value_type = T; // TS is possibly cv-qualified void
    using error_type = E;
    using unexpected_type = unexpected<E>;

    expected() noexcept(std::is_nothrow_default_constructible_v<data_type>) = default;
    expected(const expected&) = default;
    expected(expected&&) noexcept(std::is_nothrow_move_constructible_v<data_type>) = default;

    // implicit copy construction from a different expected type
    template <
        typename U,
        typename F,
        std::enable_if_t<
            std::is_void_v<U> &&
            std::is_constructible_v<E, const F&> &&
            !detail::is_constructible_from_any_cref_v<unexpected<E>, expected<U, F>>,
            bool
        > = true,
        std::enable_if_t<std::is_convertible_v<const F&, E>, bool> = true
    >
    expected(const expected<U, F>& other):
        data_(other.data_) {}

    // explicit copy construction from a different expected type
    template <
        typename U,
        typename F,
        std::enable_if_t<
            std::is_void_v<U> &&
            std::is_constructible_v<E, const F&> &&
            !detail::is_constructible_from_any_cref_v<unexpected<E>, expected<U, F>>,
            bool
        > = true,
        std::enable_if_t<!std::is_convertible_v<const F&, E>, bool> = true
    >
    expected(const expected<U, F>& other):
        data_(other.data_) {}

// wip ...
#if 0

    // implicit move construction from a different expected type
    template <
        typename U,
        typename F,
        std::enable_if_t<
            std::is_constructible_v<T, U> &&
            std::is_constructible_v<E, F> &&
            !std::is_same_v<bool, std::remove_cv_t<T>> &&
            !detail::is_constructible_from_any_cref_v<T, expected<U, F>> &&
            !detail::is_convertible_from_any_cref_v<expected<U, F>, T> &&
            !detail::is_constructible_from_any_cref_v<unexpected<E>, expected<U, F>>,
            bool
        > = true,
        std::enable_if_t<
            std::is_convertible_v<U, T> && std::is_convertible_v<F, E>,
            bool
        > = true
    >
    expected(expected<U, F>&& other):
        data_(std::move(other.data_)) {}

    // explicit move construction from a different expected type
    template <
        typename U,
        typename F,
        std::enable_if_t<
            std::is_constructible_v<T, U> &&
            std::is_constructible_v<E, F> &&
            !std::is_same_v<bool, std::remove_cv_t<T>> &&
            !detail::is_constructible_from_any_cref_v<T, expected<U, F>> &&
            !detail::is_convertible_from_any_cref_v<expected<U, F>, T> &&
            !detail::is_constructible_from_any_cref_v<unexpected<E>, expected<U, F>>,
            bool
        > = true,
        std::enable_if_t<
            !std::is_convertible_v<U, T> || !std::is_convertible_v<F, E>,
            bool
        > = true
    >
    explicit expected(const expected<U, F>& other):
        data_(std::move(other.data_)) {}

    // implicit construction from compatible value type
    template <
        typename U,
        std::enable_if_t<
            std::is_constructible_v<T, U> &&
            !std::is_same_v<std::in_place_t, detail::remove_cvref_t<U>> &&
            !std::is_same_v<expected, detail::remove_cvref_t<U>> &&
            !detail::is_unexpected_v<detail::remove_cvref_t<U>> &&
            !(std::is_same_v<bool, detail::remove_cvref_t<T>> && detail::is_expected_v<detail::remove_cvref_t<U>>),
            bool
        > = true,
        std::enable_if_t<std::is_convertible_v<U, T>, bool> = true
    >
    expected(U&& value):
        data_(std::in_place_index<0>, std::forward<U>(value)) {}

    // explicit construction from compatible value type
    template <
        typename U,
        std::enable_if_t<
            std::is_constructible_v<T, U> &&
            !std::is_same_v<std::in_place_t, detail::remove_cvref_t<U>> &&
            !std::is_same_v<expected, detail::remove_cvref_t<U>> &&
            !detail::is_unexpected_v<detail::remove_cvref_t<U>> &&
            !(std::is_same_v<bool, detail::remove_cvref_t<T>> && detail::is_expected_v<detail::remove_cvref_t<U>>),
            bool
        > = true,
        std::enable_if_t<!std::is_convertible_v<U, T>, bool> = true
    >
    explicit expected(U&& value):
        data_(std::in_place_index<0>, std::forward<U>(value)) {}

    // implicit copy construction from compatible unexpected type
    template <
        typename F,
        std::enable_if_t<std::is_constructible_v<E, const F&>> = true,
        std::enable_if_t<std::is_convertible_v<F, E>, bool> = true
    >
    expected(const unexpected<F>& unexp):
        data_(std::in_place_index<1>, unexp) {}

    // explicit copy construction from compatible unexpected type
    template <
        typename F,
        std::enable_if_t<std::is_constructible_v<E, const F&>> = true,
        std::enable_if_t<!std::is_convertible_v<F, E>, bool> = true
    >
    explicit expected(const unexpected<F>& unexp):
        data_(std::in_place_index<1>, unexp) {}

    // implicit move construction from compatible unexpected type
    template <
        typename F,
        std::enable_if_t<std::is_constructible_v<E, const F&>> = true,
        std::enable_if_t<std::is_convertible_v<const F&, E>, bool> = true
    >
    expected(unexpected<F>&& unexp):
        data_(std::in_place_index<1>, std::move(unexp)) {}

    // explicit move construction from compatible unexpected type
    template <
        typename F,
        std::enable_if_t<std::is_constructible_v<E, F>> = true,
        std::enable_if_t<!std::is_convertible_v<F, E>, bool> = true
    >
    explicit expected(const unexpected<F>& unexp):
        data_(std::in_place_index<1>, unexp) {}

    // constructors using in_place_t, unexpect_t
    template <typename... As,
              typename = std::enable_if_t<std::is_constructible_v<T, As...>>>
    explicit expected(std::in_place_t, As&&... as):
        data_(std::in_place_index<0>, std::forward<As>(as)...) {}

    template <typename X,
              typename... As,
              typename = std::enable_if_t<std::is_constructible_v<T, std::initializer_list<X>&, As...>>>
    explicit expected(std::in_place_t, std::initializer_list<X> il, As&&... as):
        data_(std::in_place_index<0>, il, std::forward<As>(as)...) {}

    template <typename... As,
              typename = std::enable_if_t<std::is_constructible_v<E, As...>>>
    explicit expected(unexpect_t, As&&... as):
        data_(std::in_place_index<1>, std::in_place_t{}, std::forward<As>(args)...) {}

    template <typename X, typename... As, typename = std::enable_if_t<std::is_constructible_v<E, std::initializer_list<X>&, As...>>>
    explicit expected(unexpect_t, std::initializer_list<X> il, As&&... as):
        data_(std::in_place_index<1>, std::in_place_t{}, il, std::forward<As>(as)...) {}

    // access methods

    explicit operator bool() const noexcept { return data_.index()==0; }
    bool has_value() const noexcept { return data_.index()==0; }

    E& error() & { return std::get<1>(data_).error(); }
    const E& error() const& { return std::get<1>(data_).error(); }
    E&& error() && { return std::get<1>(std::move(data_)).error(); }
    const E&& error() const&& { return std::get<1>(std::move(data_)).error(); }

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
        return *this? std::get<0>(std::move(data_)): throw bad_expected_access(std::as_const(error()));
    }

    const T&& value() const&& {
        return *this? std::get<0>(std::move(data_)): throw bad_expected_access(std::as_const(error()));
    }

    template <typename U>
    T value_or(U&& alt) const& {
        return *this? **this: static_cast<T>(std::forward<U>(alt));
    }

    template <typename U>
    T value_or(U&& alt) && {
        return *this? std::move(**this): static_cast<T>(std::forward<U>(alt));
    }

    // monadic operations

    template <typename F>
    auto and_then(F&& f) & {
        using R = detail::remove_cvref_t<std::invoke_result_t<F, T&>>;
        return *this? std::invoke(std::forward<F>(f), **this): R(unexpect, error());
    }

    template <typename F>
    auto and_then(F&& f) const& {
        using R = detail::remove_cvref_t<std::invoke_result_t<F, const T&>>;
        return *this? std::invoke(std::forward<F>(f), **this): R(unexpect, error());
    }

    template <typename F>
    auto and_then(F&& f) const& {
        using R = detail::remove_cvref_t<std::invoke_result_t<F, T&&>>;
        return *this? std::invoke(std::forward<F>(f), std::move(**this)): R(unexpect, std::move(error()));
    }

    template <typename F>
    auto and_then(F&& f) const&& {
        using R = detail::remove_cvref_t<std::invoke_result_t<F, const T&&>>;
        return *this? std::invoke(std::forward<F>(f), std::move(**this)): R(unexpect, std::move(error()));
    }

    template <typename F>
    auto or_else(F&& f) & {
        using R = detail::remove_cvref_t<std::invoke_result_t<F, E&>>;
        return *this? R(std::in_place, **this): std::invoke(std::forward<F>(f), error());
    }

    template <typename F>
    auto or_else(F&& f) const& {
        using R = detail::remove_cvref_t<std::invoke_result_t<F, const E&>>;
        return *this? R(std::in_place, **this): std::invoke(std::forward<F>(f), error());
    }

    template <typename F>
    auto or_else(F&& f) && {
        using R = detail::remove_cvref_t<std::invoke_result_t<F, E&&>>;
        return *this? R(std::in_place, std::move(**this)): std::invoke(std::forward<F>(f), std::move(error()));
    }

    template <typename F>
    auto or_else(F&& f) const&& {
        using R = detail::remove_cvref_t<std::invoke_result_t<F, const E&&>>;
        return *this? R(std::in_place, std::move(**this)): std::invoke(std::forward<F>(f), std::move(error()));
    }

    template <typename F>
    auto transform(F&& f) & {
        using R = expected<std::remove_cv_t<std::invoke_result_t<F, T&>>, E>;
        return *this? R(std::in_place, std::invoke(std::forward<F>(f), **this)): R(unexpect, error());
    }

    template <typename F>
    auto transform(F&& f) const& {
        using R = expected<std::remove_cv_t<std::invoke_result_t<F, const T&>>, E>;
        return *this? R(std::in_place, std::invoke(std::forward<F>(f), **this)): R(unexpect, error());
    }

    template <typename F>
    auto transform(F&& f) && {
        using R = expected<std::remove_cv_t<std::invoke_result_t<F, T&&>>, E>;
        return *this? R(std::in_place, std::invoke(std::forward<F>(f), std::move(**this))): R(unexpect, std::move(error()));
    }

    template <typename F>
    auto transform(F&& f) const&& {
        using R = expected<std::remove_cv_t<std::invoke_result_t<F, const T&&>>, E>;
        return *this? R(std::in_place, std::invoke(std::forward<F>(f), std::move(**this))): R(unexpect, std::move(error()));
    }

    template <typename F>
    auto transform_error(F&& f) & {
        using R = expected<T, std::remove_cv_t<std::invoke_result_t<F, E&>>;
        return *this? R(std::in_place, **this): R(unexpect, std::invoke(std::forward<F>(f), error()));
    }

    template <typename F>
    auto transform_error(F&& f) const& {
        using R = expected<T, std::remove_cv_t<std::invoke_result_t<F, const E&>>;
        return *this? R(std::in_place, **this): R(unexpect, std::invoke(std::forward<F>(f), error()));
    }

    template <typename F>
    auto transform_error(F&& f) && {
        using R = expected<T, std::remove_cv_t<std::invoke_result_t<F, E&&>>;
        return *this? R(std::in_place, std::move(**this)): R(unexpect, std::invoke(std::forward<F>(f), std::move(error())));
    }

    template <typename F>
    auto transform_error(F&& f) const&& {
        using R = expected<T, std::remove_cv_t<std::invoke_result_t<F, const E&&>>;
        return *this? R(std::in_place, std::move(**this)): R(unexpect, std::invoke(std::forward<F>(f), std::move(error())));
    }

    // emplace expected value

    template <
        typename... As,
        std::enable_if_t<std::is_nothrow_constructible_v<T, As...>, bool> = true
    >
    T& emplace(As&&... as) noexcept { return data_.emplace<0>(std::forward<As>(as)...); }

    template <
        typename X,
        typename... As,
        std::enable_if_t<std::is_nothrow_constructible_v<T, std::initializer_list<X>&, As...>, bool> = true
    >
    T& emplace(std::initializer_list<X> il, As&&... as) noexcept { return data_.emplace<0>(il, std::forward<As>(as)...); }

    // swap

    void swap(expected& other) noexcept(std::is_nothrow_swappable_v<data_type>) {
        using std::swap;
        swap(data_, other.data_);
    }

    friend void swap(expected& a, expected& b) noexcept(noexcept(a.swap(b))) { a.swap(b); }
#endif

private:
    using data_type = std::optional<unexpected_type>;
    data_type data_;
};

} // namespace hf
