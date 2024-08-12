#include <gtest/gtest.h>

#include <utility>
#include <vector>

#include <backport/expected.h>
#include "common.h"

using backport::expected;
using backport::unexpect;
using backport::unexpected;
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
        expected<void, cx> wx1(unexpect);
        expected<void, Z> vg1(wx1);
        EXPECT_EQ(2, cx::n_copy_ctor);

        expected<empty, Z> g2(expected<empty, cx>{unexpect});
        expected<void, Z> vg2(expected<void, cx>{unexpect});
        EXPECT_EQ(2, cx::n_move_ctor);

        expected<empty, cy> gy1(unexpect);
        expected<empty, Z> g3(gy1);
        expected<void, cy> wy1(unexpect);
        expected<void, Z> vg3(wy1);
        EXPECT_EQ(2, cy::n_copy_ctor);

        expected<empty, Z> g4(expected<empty, cy>{unexpect});
        expected<void, Z> vg4(expected<void, cy>{unexpect});
        EXPECT_EQ(2, cy::n_move_ctor);

        EXPECT_EQ(2, cx::n_copy_ctor);
        EXPECT_EQ(2, cx::n_move_ctor);
        EXPECT_EQ(0, cx::n_copy_assign);
        EXPECT_EQ(0, cx::n_move_assign);

        EXPECT_EQ(2, cy::n_copy_ctor);
        EXPECT_EQ(2, cy::n_move_ctor);
        EXPECT_EQ(0, cy::n_copy_assign);
        EXPECT_EQ(0, cy::n_move_assign);
    }
}

TEST(expected, assignment) {
    {
        // copy and move assignment from same expected type

        struct X {};
        struct Y {};

        using cx = counted<X>;
        using cy = counted<Y>;

        cx::reset();
        cy::reset();

        expected<cx, cy> be;
        expected<cx, cy> bu(unexpect);

        expected<cx, cy> ae1, ae2, ae3, ae4;
        expected<cx, cy> au1(unexpect), au2(unexpect), au3(unexpect), au4(unexpect);

        cx::reset();
        cy::reset();

        ae1 = be;
        EXPECT_TRUE(ae1.has_value());
        EXPECT_EQ(1, cx::n_copy_assign);

        ae2 = bu;
        EXPECT_FALSE(ae2.has_value());
        EXPECT_EQ(1, cy::n_copy_ctor);

        ae3 = expected<cx, cy>{};
        EXPECT_TRUE(ae3.has_value());
        EXPECT_EQ(1, cx::n_move_assign);

        ae4 = expected<cx, cy>(unexpect);
        EXPECT_FALSE(ae4.has_value());
        EXPECT_EQ(1, cy::n_move_ctor);

        au1 = be;
        EXPECT_TRUE(au1.has_value());
        EXPECT_EQ(1, cx::n_copy_ctor);

        au2 = bu;
        EXPECT_FALSE(au2.has_value());
        EXPECT_EQ(1, cy::n_copy_assign);

        au3 = expected<cx, cy>{};
        EXPECT_TRUE(au3.has_value());
        EXPECT_EQ(1, cx::n_move_ctor);

        au4 = expected<cx, cy>(unexpect);
        EXPECT_FALSE(au4.has_value());
        EXPECT_EQ(1, cy::n_move_assign);

        // check total ctors, assigns

        EXPECT_EQ(1, cx::n_copy_ctor);
        EXPECT_EQ(1, cx::n_move_ctor);
        EXPECT_EQ(1, cx::n_copy_assign);
        EXPECT_EQ(1, cx::n_move_assign);

        EXPECT_EQ(1, cy::n_copy_ctor);
        EXPECT_EQ(1, cy::n_move_ctor);
        EXPECT_EQ(1, cy::n_copy_assign);
        EXPECT_EQ(1, cy::n_move_assign);

        // expected<void,...> cases

        expected<void, cy> vbe;
        expected<void, cy> vbu(unexpect);

        expected<void, cy> vae1, vae2, vae3, vae4;
        expected<void, cy> vau1(unexpect), vau2(unexpect), vau3(unexpect), vau4(unexpect);

        cy::reset();

        vae1 = vbe;
        EXPECT_TRUE(vae1.has_value());

        vae2 = vbu;
        EXPECT_FALSE(vae2.has_value());
        EXPECT_EQ(1, cy::n_copy_ctor);

        vae3 = expected<void, cy>{};
        EXPECT_TRUE(vae3.has_value());

        vae4 = expected<void, cy>(unexpect);
        EXPECT_FALSE(vae4.has_value());
        EXPECT_EQ(1, cy::n_move_ctor);

        vau1 = vbe;
        EXPECT_TRUE(vau1.has_value());

        vau2 = vbu;
        EXPECT_FALSE(vau2.has_value());
        EXPECT_EQ(1, cy::n_copy_assign);

        vau3 = expected<void, cy>{};
        EXPECT_TRUE(vau3.has_value());

        vau4 = expected<void, cy>(unexpect);
        EXPECT_FALSE(vau4.has_value());
        EXPECT_EQ(1, cy::n_move_assign);

        EXPECT_EQ(1, cy::n_copy_ctor);
        EXPECT_EQ(1, cy::n_move_ctor);
        EXPECT_EQ(1, cy::n_copy_assign);
        EXPECT_EQ(1, cy::n_move_assign);
    }

    {
        // assignment from value type

        struct X {};
        struct Y {};

        using cx = counted<X>;
        using cy = counted<Y>;

        expected<cx, cy> ae1, ae2, au1(unexpect), au2(unexpect);

        cx::reset();
        cy::reset();

        cx x;

        ae1 = x;
        EXPECT_TRUE(ae1.has_value());
        EXPECT_EQ(1, cx::n_copy_assign);

        ae2 = cx{};
        EXPECT_TRUE(ae2.has_value());
        EXPECT_EQ(1, cx::n_move_assign);

        au1 = x;
        EXPECT_TRUE(au1.has_value());
        EXPECT_EQ(1, cx::n_copy_ctor);

        au2 = cx{};
        EXPECT_TRUE(au2.has_value());
        EXPECT_EQ(1, cx::n_move_ctor);

        EXPECT_EQ(1, cx::n_copy_ctor);
        EXPECT_EQ(1, cx::n_move_ctor);
        EXPECT_EQ(1, cx::n_copy_assign);
        EXPECT_EQ(1, cx::n_move_assign);
    }

    {
        // assignment from compatible value type

        struct X {};
        struct Z {
            Z() = default;
            Z(const X&): cc(true) {}
            Z(X&&): mc(true) {}

            Z& operator=(const X&) { return ca=true, *this; }
            Z& operator=(X&&) { return ma=true, *this; }

            bool cc = false;
            bool mc = false;
            bool ca = false;
            bool ma = false;
        };

        expected<Z, X> z1, z2, zu1(unexpect), zu2(unexpect);
        X x;
        z1 = x;
        ASSERT_TRUE(z1.has_value());
        EXPECT_TRUE(z1.value().ca);

        z2 = X{};
        ASSERT_TRUE(z2.has_value());
        EXPECT_TRUE(z2.value().ma);

        zu1 = x;
        ASSERT_TRUE(zu1.has_value());
        EXPECT_TRUE(zu1.value().cc);

        zu2 = X{};
        ASSERT_TRUE(zu2.has_value());
        EXPECT_TRUE(zu2.value().mc);
    }

    {
        // asignment from unexpected type


        // expected<void, ...> cases
    }
    {
        // asignment from compatible unexpected type

        // expected<void, ...> cases
    }
}

TEST(expected, emplace) {
    using cc = counted<check_in_place>;
    struct empty {};

    {
        cc::reset();
        expected<cc, empty> ex(in_place);

        EXPECT_TRUE(ex.has_value());
        EXPECT_EQ(0, ex.value().inner.n_in_place_args);

        ex.emplace(10);
        EXPECT_TRUE(ex.has_value());
        EXPECT_EQ(1, ex.value().inner.n_in_place_args);

        ex.emplace(10, 20);
        EXPECT_TRUE(ex.has_value());
        EXPECT_EQ(2, ex.value().inner.n_in_place_args);

        ex.emplace({3, 4, 5});
        EXPECT_EQ(1, ex.value().inner.n_in_place_args);
        EXPECT_TRUE(ex.has_value());

        ex.emplace({3, 4, 5}, 20, 30);
        EXPECT_TRUE(ex.has_value());
        EXPECT_EQ(3, ex.value().inner.n_in_place_args);

        expected<cc, empty> mt(unexpect);
        EXPECT_FALSE(mt.has_value());

        mt.emplace({3, 4, 5}, 20, 30);
        EXPECT_TRUE(mt.has_value());
        EXPECT_EQ(3, mt.value().inner.n_in_place_args);

        EXPECT_EQ(0, cc::n_copy_ctor);
        EXPECT_EQ(0, cc::n_move_ctor);
        EXPECT_EQ(0, cc::n_copy_assign);
        EXPECT_EQ(0, cc::n_move_assign);

        expected<void, empty> v;
        EXPECT_TRUE(v.has_value());

        v.emplace();
        EXPECT_TRUE(v.has_value());

        expected<void, empty> w(unexpect);
        EXPECT_FALSE(w.has_value());

        w.emplace();
        EXPECT_TRUE(w.has_value());
    }
}


#if 0
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
