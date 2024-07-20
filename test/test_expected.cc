#include <gtest/gtest.h>

#include <utility>
#include <vector>

#include <hf/expected.h>
#include "common.h"

using namespace hf;
using std::in_place;

TEST(expected, ctors) {
    // Check the myriad constructors and for implicit conversions.
    // Will also exercise expected<T, E>::has_value(), value(), error().

    using cc = counted<check_in_place>;
    struct empty {};

    {
        // value construction

        expected<void, empty> e1;
        EXPECT_TRUE(e1.has_value());

        cc::reset();
        expected<cc, empty> e2;
        EXPECT_TRUE(e2.has_value());
        EXPECT_EQ(0, e2.value().inner.n_in_place_args);

        EXPECT_EQ(0, cc::n_copy_ctor);
        EXPECT_EQ(0, cc::n_move_ctor);
        EXPECT_EQ(0, cc::n_copy_assign);
        EXPECT_EQ(0, cc::n_move_assign);

        expected<void, empty> v1;
        EXPECT_TRUE(v1.has_value());
    }

    {
        // in-place initialization of value

        cc::reset();
        expected<cc, empty> e0(in_place);
        EXPECT_TRUE(e0.has_value());
        EXPECT_EQ(0, e0.value().inner.n_in_place_args);

        expected<cc, empty> e1(in_place, 10);
        EXPECT_TRUE(e1.has_value());
        EXPECT_EQ(1, e1.value().inner.n_in_place_args);

        expected<cc, empty> e2(in_place, 10, 20);
        EXPECT_TRUE(e2.has_value());
        EXPECT_EQ(2, e2.value().inner.n_in_place_args);

        expected<cc, empty> e3(in_place, {3, 4, 5});
        EXPECT_EQ(1, e3.value().inner.n_in_place_args);
        EXPECT_TRUE(e3.has_value());

        expected<cc, empty> e4(in_place, {3, 4, 5}, 20, 30);
        EXPECT_TRUE(e4.has_value());
        EXPECT_EQ(3, e4.value().inner.n_in_place_args);

        EXPECT_EQ(0, cc::n_copy_ctor);
        EXPECT_EQ(0, cc::n_move_ctor);
        EXPECT_EQ(0, cc::n_copy_assign);
        EXPECT_EQ(0, cc::n_move_assign);

        expected<void, empty> v1(std::in_place);
        EXPECT_TRUE(v1.has_value());

    }

    {
        // in-place initialization of error, tests both void
        // and non-void value types.

        cc::reset();
        expected<empty, cc> e0(unexpect);
        EXPECT_FALSE(e0.has_value());
        EXPECT_EQ(0, e0.error().inner.n_in_place_args);

        expected<empty, cc> e1(unexpect, 10);
        EXPECT_FALSE(e1.has_value());
        EXPECT_EQ(1, e1.error().inner.n_in_place_args);

        expected<empty, cc> e2(unexpect, 10, 20);
        EXPECT_FALSE(e2.has_value());
        EXPECT_EQ(2, e2.error().inner.n_in_place_args);

        expected<empty, cc> e3(unexpect, {3, 4, 5});
        EXPECT_FALSE(e3.has_value());
        EXPECT_EQ(1, e3.error().inner.n_in_place_args);

        expected<empty, cc> e4(unexpect, {3, 4, 5}, 20, 30);
        EXPECT_FALSE(e4.has_value());
        EXPECT_EQ(3, e4.error().inner.n_in_place_args);

        expected<void, cc> v0(unexpect);
        EXPECT_FALSE(v0.has_value());
        EXPECT_EQ(0, v0.error().inner.n_in_place_args);

        expected<void, cc> v1(unexpect, 10);
        EXPECT_FALSE(v1.has_value());
        EXPECT_EQ(1, v1.error().inner.n_in_place_args);

        expected<void, cc> v2(unexpect, 10, 20);
        EXPECT_FALSE(v2.has_value());
        EXPECT_EQ(2, v2.error().inner.n_in_place_args);

        expected<void, cc> v3(unexpect, {3, 4, 5});
        EXPECT_FALSE(v3.has_value());
        EXPECT_EQ(1, v3.error().inner.n_in_place_args);

        expected<void, cc> v4(unexpect, {3, 4, 5}, 20, 30);
        EXPECT_FALSE(v4.has_value());
        EXPECT_EQ(3, v4.error().inner.n_in_place_args);

        EXPECT_EQ(0, cc::n_copy_ctor);
        EXPECT_EQ(0, cc::n_move_ctor);
        EXPECT_EQ(0, cc::n_copy_assign);
        EXPECT_EQ(0, cc::n_move_assign);
    }

    {
        // copy and move constructors

        struct X {};
        struct Y {};

        using cx = counted<X>;
        using cy = counted<Y>;

        cx::reset();
        cy::reset();

        expected<cx, cy> v;
        const expected<cx, cy> cv;

        expected<cx, cy> u(unexpect);
        const expected<cx, cy> cu(unexpect);

        expected<cx, cy> e1(v);
        EXPECT_TRUE(e1.has_value());
        EXPECT_EQ(1, cx::n_copy_ctor);

        expected<cx, cy> e2(cv);
        EXPECT_TRUE(e2.has_value());
        EXPECT_EQ(2, cx::n_copy_ctor);

        expected<cx, cy> e3(std::move(v));
        EXPECT_TRUE(e3.has_value());
        EXPECT_EQ(1, cx::n_move_ctor);

        expected<cx, cy> u1(u);
        EXPECT_TRUE(!u1.has_value());
        EXPECT_EQ(1, cy::n_copy_ctor);

        expected<cx, cy> u2(cu);
        EXPECT_TRUE(!u2.has_value());
        EXPECT_EQ(2, cy::n_copy_ctor);

        expected<cx, cy> u3(std::move(u));
        EXPECT_TRUE(!u3.has_value());
        EXPECT_EQ(1, cy::n_move_ctor);

        EXPECT_EQ(2, cx::n_copy_ctor);
        EXPECT_EQ(1, cx::n_move_ctor);
        EXPECT_EQ(0, cx::n_copy_assign);
        EXPECT_EQ(0, cx::n_move_assign);

        EXPECT_EQ(2, cy::n_copy_ctor);
        EXPECT_EQ(1, cy::n_move_ctor);
        EXPECT_EQ(0, cy::n_copy_assign);
        EXPECT_EQ(0, cy::n_move_assign);

        cx::reset();

        expected<void, cx> w;
        const expected<void, cx> cw;

        expected<void, cx> z(unexpect);
        const expected<void, cx> cz(unexpect);

        expected<void, cx> f1(w);
        EXPECT_TRUE(e1.has_value());
        EXPECT_EQ(0, cx::n_copy_ctor);

        expected<void, cx> f2(cw);
        EXPECT_TRUE(f2.has_value());
        EXPECT_EQ(0, cx::n_copy_ctor);

        expected<void, cx> f3(std::move(w));
        EXPECT_TRUE(f3.has_value());
        EXPECT_EQ(0, cx::n_move_ctor);

        expected<void, cx> q1(z);
        EXPECT_TRUE(!q1.has_value());
        EXPECT_EQ(1, cx::n_copy_ctor);

        expected<void, cx> q2(cz);
        EXPECT_TRUE(!q2.has_value());
        EXPECT_EQ(2, cx::n_copy_ctor);

        expected<void, cx> q3(std::move(z));
        EXPECT_TRUE(!q3.has_value());
        EXPECT_EQ(1, cx::n_move_ctor);

        EXPECT_EQ(2, cx::n_copy_ctor);
        EXPECT_EQ(1, cx::n_move_ctor);
        EXPECT_EQ(0, cx::n_copy_assign);
        EXPECT_EQ(0, cx::n_move_assign);
    }

    {
        // construct from types that can construct value type
        // or error type

        struct X {};
        struct Y {};

        using cx = counted<X>;
        using cy = counted<Y>;

        struct Z {
            cx x_;
            cy y_;

            Z() = default;

            // implicit construction from cx with copy or move
            Z(const cx& x): x_(x) {}
            Z(cx&& x): x_(std::move(x)) {}

            // explicit construction from cy with copy or move
            explicit Z(const cy& y): y_(y) {}
            explicit Z(cy&& y): y_(std::move(y)) {}
        };

        EXPECT_TRUE((std::is_convertible_v<cx, expected<Z, empty>>));
        EXPECT_FALSE((std::is_convertible_v<cy, expected<Z, empty>>));

        cx::reset();
        cy::reset();

        cx x1;
        expected<Z, empty> e1(x1);
        EXPECT_EQ(1, cx::n_copy_ctor);

        expected<Z, empty> e2(cx{});
        EXPECT_EQ(1, cx::n_move_ctor);

        cy y1;
        expected<Z, empty> e3(y1);
        EXPECT_EQ(1, cy::n_copy_ctor);

        expected<Z, empty> e4(cy{});
        EXPECT_EQ(1, cy::n_move_ctor);

        EXPECT_EQ(1, cx::n_copy_ctor);
        EXPECT_EQ(1, cx::n_move_ctor);
        EXPECT_EQ(0, cx::n_copy_assign);
        EXPECT_EQ(0, cx::n_move_assign);

        EXPECT_EQ(1, cy::n_copy_ctor);
        EXPECT_EQ(1, cy::n_move_ctor);
        EXPECT_EQ(0, cy::n_copy_assign);
        EXPECT_EQ(0, cy::n_move_assign);

        EXPECT_TRUE((std::is_convertible_v<unexpected<cx>, expected<empty, Z>>));
        EXPECT_TRUE((std::is_convertible_v<unexpected<cx>, expected<void, Z>>));
        EXPECT_FALSE((std::is_convertible_v<unexpected<cy>, expected<empty, Z>>));
        EXPECT_FALSE((std::is_convertible_v<unexpected<cy>, expected<void, Z>>));

        cx::reset();
        cy::reset();

        unexpected<cx> ux1{std::in_place};
        expected<empty, Z> u1(ux1);
        expected<void, Z> vu1(ux1);
        EXPECT_EQ(2, cx::n_copy_ctor);

        expected<empty, Z> u2(unexpected<cx>{std::in_place});
        expected<void, Z> vu2(unexpected<cx>{std::in_place});
        EXPECT_EQ(2, cx::n_move_ctor);

        unexpected<cy> uy1{std::in_place};
        expected<empty, Z> u3(uy1);
        expected<void, Z> vu3(uy1);
        EXPECT_EQ(2, cy::n_copy_ctor);

        expected<empty, Z> u4(unexpected<cy>{std::in_place});
        expected<void, Z> vu4(unexpected<cy>{std::in_place});
        EXPECT_EQ(2, cy::n_move_ctor);

        EXPECT_EQ(2, cx::n_copy_ctor);
        EXPECT_EQ(2, cx::n_move_ctor);
        EXPECT_EQ(0, cx::n_copy_assign);
        EXPECT_EQ(0, cx::n_move_assign);

        EXPECT_EQ(2, cy::n_copy_ctor);
        EXPECT_EQ(2, cy::n_move_ctor);
        EXPECT_EQ(0, cy::n_copy_assign);
        EXPECT_EQ(0, cy::n_move_assign);

        cy::reset();

        // construct from corresponding expect types

        EXPECT_TRUE((std::is_convertible_v<expected<cx, empty>, expected<Z, empty>>));
        EXPECT_FALSE((std::is_convertible_v<expected<cy, empty>, expected<Z, empty>>));

        cx::reset();
        cy::reset();

        expected<cx, empty> ex1;
        expected<Z, empty> f1(ex1);
        EXPECT_EQ(1, cx::n_copy_ctor);

        expected<Z, empty> f2(expected<cx, empty>{});
        EXPECT_EQ(1, cx::n_move_ctor);

        expected<cy, empty> ey1;
        expected<Z, empty> f3(ey1);
        EXPECT_EQ(1, cy::n_copy_ctor);

        expected<Z, empty> f4(expected<cy, empty>{});
        EXPECT_EQ(1, cy::n_move_ctor);

        EXPECT_EQ(1, cx::n_copy_ctor);
        EXPECT_EQ(1, cx::n_move_ctor);
        EXPECT_EQ(0, cx::n_copy_assign);
        EXPECT_EQ(0, cx::n_move_assign);

        EXPECT_EQ(1, cy::n_copy_ctor);
        EXPECT_EQ(1, cy::n_move_ctor);
        EXPECT_EQ(0, cy::n_copy_assign);
        EXPECT_EQ(0, cy::n_move_assign);

        EXPECT_TRUE((std::is_convertible_v<expected<empty, cx>, expected<empty, Z>>));
        EXPECT_FALSE((std::is_convertible_v<expected<empty, cy>, expected<empty, Z>>));

        cx::reset();
        cy::reset();

        expected<empty, cx> gx1(unexpect);
        expected<empty, Z> g1(gx1);
        EXPECT_EQ(1, cx::n_copy_ctor);

        expected<empty, Z> g2(expected<empty, cx>{unexpect});
        EXPECT_EQ(1, cx::n_move_ctor);

        expected<empty, cy> gy1(unexpect);
        expected<empty, Z> g3(gy1);
        EXPECT_EQ(1, cy::n_copy_ctor);

        expected<empty, Z> g4(expected<empty, cy>{unexpect});
        EXPECT_EQ(1, cy::n_move_ctor);

        EXPECT_EQ(1, cx::n_copy_ctor);
        EXPECT_EQ(1, cx::n_move_ctor);
        EXPECT_EQ(0, cx::n_copy_assign);
        EXPECT_EQ(0, cx::n_move_assign);

        EXPECT_EQ(1, cy::n_copy_ctor);
        EXPECT_EQ(1, cy::n_move_ctor);
        EXPECT_EQ(0, cy::n_copy_assign);
        EXPECT_EQ(0, cy::n_move_assign);
    }
}

TEST(expected, assignment) {
}

#if 0
TEST(expected, assignment) {
    {
        expected<int, int> x(10), y(12), z(unexpect, 20);

        EXPECT_EQ(12, (x=y).value());
        EXPECT_EQ(20, (x=z).error());

        expected<void, int> u, v, w(unexpect, 30);

        EXPECT_TRUE((u=v).has_value());
        EXPECT_EQ(30, (u=w).error());
    }

    {
        struct X {
            X(): v(0) {}
            X(const int& a): v(10*a) {}
            X(int&& a): v(20*a) {}
            int v;
        };

        expected<X, int> y;
        EXPECT_EQ(20, (y=1).value().v);
        int a = 3;
        EXPECT_EQ(30, (y=a).value().v);

        expected<int, X> z;
        EXPECT_EQ(20, (z=unexpected(1)).error().v);
        unexpected<int> b(3);
        EXPECT_EQ(30, (z=b).error().v);

        expected<void, X> v;
        EXPECT_EQ(20, (v=unexpected(1)).error().v);
        EXPECT_EQ(30, (v=b).error().v);
    }
}

TEST(expected, emplace) {
    // Check we're forwarding properly...
    struct X {
        X(): v(0) {}
        X(const int& a, int b): v(10*a + b) {}
        X(int&& a, int b): v(20*a + b) {}
        int v;
    };

    expected<X, bool> ex;
    EXPECT_TRUE(ex);
    EXPECT_EQ(0, ex.value().v);

    int i = 3, j = 4;
    ex.emplace(i, j);
    EXPECT_TRUE(ex);
    EXPECT_EQ(34, ex.value().v);
    ex.emplace(3, j);
    EXPECT_TRUE(ex);
    EXPECT_EQ(64, ex.value().v);

    // Should also work if ex was in error state.
    expected<X, bool> ux(unexpect);
    EXPECT_FALSE(ux);
    ux.emplace(4, 1);
    EXPECT_TRUE(ux);
    EXPECT_EQ(81, ux.value().v);
}

TEST(expected, equality) {
    {
        // non-void value expected comparisons:

        expected<int, int> ex1(1), ux1(unexpect, 1), ex2(2), ux2(unexpect, 2);
        expected<int, int> x(ex1);

        EXPECT_TRUE(x==ex1);
        EXPECT_TRUE(ex1==x);
        EXPECT_FALSE(x!=ex1);
        EXPECT_FALSE(ex1!=x);

        EXPECT_FALSE(x==ex2);
        EXPECT_FALSE(ex2==x);
        EXPECT_TRUE(x!=ex2);
        EXPECT_TRUE(ex2!=x);

        EXPECT_FALSE(x==ux1);
        EXPECT_FALSE(ux1==x);
        EXPECT_TRUE(x!=ux1);
        EXPECT_TRUE(ux1!=x);

        EXPECT_FALSE(ux1==ux2);
        EXPECT_FALSE(ux2==ux1);
        EXPECT_TRUE(ux1!=ux2);
        EXPECT_TRUE(ux2!=ux1);
    }
    {
        // non-void comparison against values and unexpected.

        expected<int, int> x(10);

        EXPECT_TRUE(x==10);
        EXPECT_TRUE(10==x);
        EXPECT_FALSE(x!=10);
        EXPECT_FALSE(10!=x);

        EXPECT_FALSE(x==unexpected(10));
        EXPECT_FALSE(unexpected(10)==x);
        EXPECT_TRUE(x!=unexpected(10));
        EXPECT_TRUE(unexpected(10)!=x);

        x = unexpected(10);

        EXPECT_FALSE(x==10);
        EXPECT_FALSE(10==x);
        EXPECT_TRUE(x!=10);
        EXPECT_TRUE(10!=x);

        EXPECT_TRUE(x==unexpected(10));
        EXPECT_TRUE(unexpected(10)==x);
        EXPECT_FALSE(x!=unexpected(10));
        EXPECT_FALSE(unexpected(10)!=x);
    }
    {
        // void value expected comparisons:

        expected<void, int> ev, uv1(unexpect, 1), uv2(unexpect, 2);
        expected<void, int> x(ev);

        EXPECT_TRUE(x==ev);
        EXPECT_TRUE(ev==x);
        EXPECT_FALSE(x!=ev);
        EXPECT_FALSE(ev!=x);

        EXPECT_FALSE(x==uv1);
        EXPECT_FALSE(uv1==x);
        EXPECT_TRUE(x!=uv1);
        EXPECT_TRUE(uv1!=x);

        EXPECT_FALSE(uv1==uv2);
        EXPECT_FALSE(uv2==uv1);
        EXPECT_TRUE(uv1!=uv2);
        EXPECT_TRUE(uv2!=uv1);
    }
    {
        // void value but difference unexpected types:

        expected<void, int> uvi(unexpect);
        expected<void, double> uvd(unexpect, 3.);

        EXPECT_FALSE(uvi==uvd);
        EXPECT_FALSE(uvd==uvi);

        uvi = expected<void, int>();
        ASSERT_TRUE(uvi);
        EXPECT_FALSE(uvi==uvd);
        EXPECT_FALSE(uvd==uvi);

        uvd = expected<void, double>();
        ASSERT_TRUE(uvd);
        EXPECT_TRUE(uvi==uvd);
        EXPECT_TRUE(uvd==uvi);
    }
    {
        // void comparison against unexpected.

        expected<void, int> x;

        EXPECT_TRUE(x);
        EXPECT_FALSE(x==unexpected(10));
        EXPECT_FALSE(unexpected(10)==x);
        EXPECT_TRUE(x!=unexpected(10));
        EXPECT_TRUE(unexpected(10)!=x);

        x = unexpected<int>(10);

        EXPECT_FALSE(x);
        EXPECT_TRUE(x==unexpected(10));
        EXPECT_TRUE(unexpected(10)==x);
        EXPECT_FALSE(x!=unexpected(10));
        EXPECT_FALSE(unexpected(10)!=x);
    }
}

TEST(expected, value_or) {
    expected<double, char> a(2.0), b(unexpect, 'x');
    EXPECT_EQ(2.0, a.value_or(1));
    EXPECT_EQ(1.0, b.value_or(1));
}

namespace {
struct Xswap {
    explicit Xswap(int val, int& r): val(val), n_swap_ptr(&r) {}
    int val;
    int* n_swap_ptr;
    void swap(Xswap& other) {
        ++*n_swap_ptr;
        std::swap(val, other.val);
    }
    friend void swap(Xswap& x1, Xswap& x2) noexcept { x1.swap(x2); }
};

struct swap_can_throw {
    friend void swap(swap_can_throw& s1, swap_can_throw& s2) noexcept(false) {}
};
}

TEST(expected, swap) {
    int swaps = 0;
    expected<Xswap, int> x1(in_place, -1, swaps), x2(in_place, -2, swaps);

    using std::swap;
    swap(x1, x2);
    EXPECT_EQ(-2, x1->val);
    EXPECT_EQ(-1, x2->val);
    EXPECT_EQ(1, swaps);

    swaps = 0;
    expected<Xswap, int> x3(unexpect, 4);
    swap(x1, x3);
    EXPECT_EQ(4, x1.error());
    EXPECT_EQ(-2, x3->val);
    EXPECT_EQ(0, swaps); // Xswap is moved, not swapped.

    swaps = 0;
    unexpected<Xswap> u1(in_place, -1, swaps), u2(in_place, -2, swaps);
    swap(u1, u2);
    EXPECT_EQ(-2, u1.value().val);
    EXPECT_EQ(-1, u2.value().val);
    EXPECT_EQ(1, swaps);

    EXPECT_TRUE(std::is_nothrow_swappable<unexpected<Xswap>>::value);
    EXPECT_FALSE(std::is_nothrow_swappable<unexpected<swap_can_throw>>::value);
}
#endif
