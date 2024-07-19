#pragma once

#include <type_traits>
#include <utility>

// Utility classes etc. for unit tests

template <typename X>
struct counted {
    X inner;
    mutable bool is_moved = false;

    template <typename... As>
    counted(As&&... as): inner(std::forward<As>(as)...) {}

    counted(counted& other): inner(other.inner) {
        ++n_copy_ctor;
    }

    counted(const counted& other): inner(other.inner) {
        ++n_copy_ctor;
    }

    counted(counted&& other): inner(std::move(other.inner)) {
        ++n_move_ctor;
        other.is_moved = true;
    }

    counted(const counted&& other): inner(std::move(other.inner)) {
        ++n_move_ctor;
        other.is_moved = true;
    }

    counted& operator=(counted& other) {
        inner = other.inner;
        ++n_copy_assign;
    }

    counted& operator=(const counted& other) {
        inner = other.inner;
        ++n_copy_assign;
    }

    counted& operator=(counted&& other) {
        inner = std::move(other.inner);
        ++n_move_assign;
        other.is_moved = true;
    }

    counted& operator=(const counted&& other) {
        inner = std::move(other.inner);
        ++n_move_assign;
        other.is_moved = true;
    }

    void swap(counted& other) {
        ++n_swaps;
        std::swap(inner, other.inner);
    }

    friend void swap(counted& a, counted& b) noexcept(std::is_nothrow_swappable_v<X>) { a.swap(b); }

    static void reset() {
        n_copy_ctor = n_move_ctor = 0;
        n_copy_assign = n_move_assign = 0;
        n_swaps = 0;
    }

    static inline unsigned n_copy_ctor;
    static inline unsigned n_move_ctor;
    static inline unsigned n_copy_assign;
    static inline unsigned n_move_assign;
    static inline unsigned n_swaps;
};

struct check_in_place {
    const int n_in_place_args = -1; // -1 => copy or move constructed
    const bool copy_constructed = false;
    const bool move_constructed = false;

    check_in_place(): n_in_place_args(0) {}
    check_in_place(int): n_in_place_args(1) {}
    check_in_place(int, int): n_in_place_args(2) {}

    check_in_place(std::initializer_list<int>): n_in_place_args(1) {}
    check_in_place(std::initializer_list<int>, int): n_in_place_args(2) {}
    check_in_place(std::initializer_list<int>, int, int): n_in_place_args(3) {}

    check_in_place(const check_in_place&): copy_constructed{true} {}
    check_in_place(check_in_place&&): move_constructed{true} {}
};

