#include <gtest/gtest.h>

#include <cmath>
#include <utility>
#include <vector>

#include <hf/expected.h>
#include "common.h"

using std::in_place;
using namespace hf;

TEST(unexpected, ctors) {
    using cc = counted<check_in_place>;

    {
        // direct initialization of error
        cc::reset();
        unexpected<cc> u(20);

        EXPECT_EQ(1, u.error().inner.n_in_place_args);
        EXPECT_EQ(0, cc::n_copy_ctor);
        EXPECT_EQ(0, cc::n_move_ctor);
        EXPECT_EQ(0, cc::n_copy_assign);
        EXPECT_EQ(0, cc::n_move_assign);
    }

    {
        // in-place initialization of error
        cc::reset();
        unexpected<cc> u0(in_place);
        EXPECT_EQ(0, u0.error().inner.n_in_place_args);

        unexpected<cc> u1(in_place, 20);
        EXPECT_EQ(1, u1.error().inner.n_in_place_args);

        unexpected<cc> u2(in_place, 20, 30);
        EXPECT_EQ(2, u2.error().inner.n_in_place_args);

        unexpected<cc> u3(in_place, {3, 4, 5});
        EXPECT_EQ(1, u3.error().inner.n_in_place_args);

        unexpected<cc> u4(in_place, {3, 4, 5}, 20, 30);
        EXPECT_EQ(3, u4.error().inner.n_in_place_args);

        EXPECT_EQ(0, cc::n_copy_ctor);
        EXPECT_EQ(0, cc::n_move_ctor);
        EXPECT_EQ(0, cc::n_copy_assign);
        EXPECT_EQ(0, cc::n_move_assign);
    }

    {
        // move and copy construction
        cc::reset();
        unexpected<cc> u0(in_place, 10);
        EXPECT_EQ(1, u0.error().inner.n_in_place_args);

        unexpected<cc> u1(u0);
        EXPECT_EQ(-1, u1.error().inner.n_in_place_args);
        EXPECT_FALSE(u0.error().is_moved);
        EXPECT_EQ(1, cc::n_copy_ctor);

        unexpected<cc> u2(const_cast<const unexpected<cc>&>(u0));
        EXPECT_EQ(-1, u2.error().inner.n_in_place_args);
        EXPECT_FALSE(u0.error().is_moved);
        EXPECT_EQ(2, cc::n_copy_ctor);

        unexpected<cc> u3(std::move(u0));
        EXPECT_EQ(-1, u3.error().inner.n_in_place_args);
        EXPECT_TRUE(u0.error().is_moved);
        EXPECT_EQ(1, cc::n_move_ctor);
    }
}

TEST(unexpected, error) {
    using ci = counted<int>;

    {
        ci::reset();
        unexpected<ci> u0(in_place);
        const unexpected<ci> u1(in_place);
        EXPECT_EQ(0, ci::n_copy_ctor);
        EXPECT_EQ(0, ci::n_move_ctor);

        ci c1(u0.error());
        EXPECT_EQ(1, ci::n_copy_ctor);
        EXPECT_EQ(0, ci::n_move_ctor);

        ci::reset();
        ci c2(u1.error());
        EXPECT_EQ(1, ci::n_copy_ctor);
        EXPECT_EQ(0, ci::n_move_ctor);

        ci::reset();
        ci c3(std::move(u0).error());
        EXPECT_EQ(0, ci::n_copy_ctor);
        EXPECT_EQ(1, ci::n_move_ctor);

        ci::reset();
        ci c4(std::move(u1).error());
        EXPECT_EQ(0, ci::n_copy_ctor);
        EXPECT_EQ(1, ci::n_move_ctor);
    }
}

TEST(unexpected, comparison) {
    struct X {
        int v;
        bool operator==(X b) const { return v==b.v; }
    };

    struct Y {
        double v;
        bool operator==(Y b) const { return v==b.v; }
        bool operator==(X b) const { return v==b.v; }
    };

    X x{10};
    Y y{10.0};

    EXPECT_TRUE((unexpected(x)==unexpected(x)));
    EXPECT_TRUE((unexpected(y)==unexpected(y)));
    EXPECT_TRUE((unexpected(y)==unexpected(x)));

    y.v = NAN;

    EXPECT_FALSE((unexpected(y)==unexpected(y)));
    EXPECT_FALSE((unexpected(y)==unexpected(x)));
}

template <bool nothrow_swappable = true>
struct X {
    int n = 0;
    void swap(X& other) noexcept(nothrow_swappable) { std::swap(n, other.n); }
    friend void swap(X& a, X& b) noexcept(noexcept(a.swap(b))) { a.swap(b); }
};

TEST(unexpected, swap) {
    ASSERT_TRUE(std::is_nothrow_swappable_v<X<true>>);
    ASSERT_FALSE(std::is_nothrow_swappable_v<X<false>>);

    EXPECT_TRUE(std::is_nothrow_swappable_v<unexpected<X<true>>>);
    EXPECT_FALSE(std::is_nothrow_swappable_v<unexpected<X<false>>>);

    X<> a{3}, b{4};
    unexpected<X<>> ua(a), ub(b);

    using std::swap;
    swap(ua, ub);
    EXPECT_EQ(4, ua.error().n);
    EXPECT_EQ(3, ub.error().n);
}

