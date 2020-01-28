#define ALIA_LOWERCASE_MACROS

#include <alia/flow/events.hpp>

#include <catch.hpp>

#include <alia/components/system.hpp>
#include <alia/signals/basic.hpp>

using namespace alia;
using std::string;

namespace {

struct my_event : targeted_event
{
    string result;
};

ALIA_DEFINE_COMPONENT_TYPE(my_tag, std::vector<routable_node_id>*)

typedef add_component_type_t<context, my_tag> my_context;

static void
do_my_thing(my_context ctx, readable<string> label)
{
    node_id this_id = get_node_id(ctx);

    if (is_refresh_event(ctx))
    {
        get_component<my_tag>(ctx)->push_back(
            make_routable_node_id(ctx, this_id));
    }

    handle_targeted_event<my_event>(
        ctx, this_id, [&](auto ctx, my_event& event) {
            event.result = read_signal(label);
        });
}

} // namespace

TEST_CASE("node IDs", "[flow][events]")
{
    alia::system sys;

    REQUIRE(!is_valid(null_node_id));

    std::vector<routable_node_id> ids;

    sys.controller = [&](context vanilla_ctx) {
        my_context ctx = add_component<my_tag>(vanilla_ctx, &ids);
        do_my_thing(ctx, val("one"));
        do_my_thing(ctx, val("two"));
    };
    refresh_system(sys);

    REQUIRE(ids.size() == 2);
    REQUIRE(is_valid(ids[0]));
    REQUIRE(is_valid(ids[1]));

    {
        my_event event;
        dispatch_targeted_event(sys, event, ids[0]);
        REQUIRE(event.result == "one");
    }
    {
        my_event event;
        dispatch_targeted_event(sys, event, ids[1]);
        REQUIRE(event.result == "two");
    }
}
