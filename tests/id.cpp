#include <alia/id.hpp>
#include <utility>
#include <boost/lexical_cast.hpp>

#include <catch.hpp>

TEST_CASE("IDs", "[id]")
{
    using namespace alia;

    // Test value IDs and the basic ID interface operators.
    value_id<int> a = make_id(1);
    REQUIRE(boost::lexical_cast<std::string>(a) == "1");
    REQUIRE(boost::lexical_cast<std::string>(ref(a)) == "1");
    REQUIRE(a == a);
    value_id<int> c = make_id(1);
    REQUIRE(ref(a) == ref(a));
    REQUIRE(a == c);
    REQUIRE(ref(a) == ref(c));
    value_id<int> b = make_id(2);
    REQUIRE(a != b);
    REQUIRE(ref(a) != ref(b));
    REQUIRE(a < b);
    REQUIRE(ref(a) < ref(b));
    REQUIRE(!(b < a));
    REQUIRE(!(ref(b) < ref(a)));
    int x;
    value_id<int*> d = make_id(&x);
    REQUIRE(a != d);
    REQUIRE(ref(a) != ref(d));
    REQUIRE((a < d && !(d < a) || d < a && !(a < d)));

    // Test owned_id.
    owned_id o;
    o.store(a);
    REQUIRE(o.get() == a);
    REQUIRE(o.get() != b);
    owned_id p;
    REQUIRE(o != p);
    p.store(a);
    REQUIRE(o == p);
    p.store(c);
    REQUIRE(o == p);
    p.store(b);
    REQUIRE(o != p);
    REQUIRE(o < p);
    REQUIRE(boost::lexical_cast<std::string>(o) == "1");

    // Test id_pair.
    o.store(combine_ids(a, b));
    REQUIRE(boost::lexical_cast<std::string>(o) == "(1,2)");
    REQUIRE(o.get() == combine_ids(a, b));
    REQUIRE(combine_ids(a, c) < combine_ids(a, b));
    REQUIRE(combine_ids(a, b) != combine_ids(b, a));
    REQUIRE(combine_ids(a, b) < combine_ids(b, a));
    o.store(combine_ids(a, ref(b)));
    REQUIRE(o.get() == combine_ids(a, ref(b)));
}