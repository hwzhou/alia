Flow Tracking
=============

<script>
    init_alia_demos(['numerical-analysis', 'switch-example',
        'loop-macros-demo', 'for-each-map-demo', 'for-each-vector-demo',
        'named-blocks-demo']);
</script>

General Rules
-------------

In order to allow alia to do its job maintaining the data graph, you have to
follow some simple rules:

1. Wherever you have loops or conditionals in your component-level application
   code, use the flow tracking constructs documented below to allow alia to
   track it.

2. Don't `return` from the middle of a function. - alia doesn't actually know
   when you enter and leave functions, so if you suddenly leave a function at a
   different spot in the data graph, this confuses it.

   ?> In the near future, mechanisms will be added to allow this, but since most
      controller functions return `void` anyway, it's usually trivial to rewrite
      your code to avoid `return`.

3. Don't use `goto`.

Note that these rules only apply to the *component-level* portions of your
application code. alia is designed so that the code that [performs
computations](function-application.md) and the code that [produces side
effects](actions.md#custom-actions) can be written as normal C++, without
worrying about flow tracking/restrictions.

alia_if/else
------------

alia provides the equivalent of `if`, `else if` and `else` as macros. Here again
is a simple example that makes use of all three:

?> The alia control flow macros come in both uppercase and lowercase forms.
   Traditionally, they have been lowercase to more closely resemble the actual
   C++ keywords that they mimic. However, the uppercase form makes it more
   obvious to readers (and clang-format) that the macros are indeed macros.
   Ultimately, it's up to you which style you prefer. If you want to be strict
   in your project, you can disable the lowercase form in [the
   configuration](configuration.md).

```cpp
auto n = get_state(ctx, empty<double>());

dom::do_text(ctx, "Enter a number:");

dom::do_input(ctx, n);

alia_if(n > 0)
{
    dom::do_text(ctx, "The number is positive!");
}
alia_else_if(n < 0)
{
    dom::do_text(ctx, "The number is negative!");
}
alia_else
{
    dom::do_text(ctx, "The number is zero!");
}
alia_end
```

<div class="demo-panel">
<div id="numerical-analysis"></div>
</div>

The conditions that you provide to `alia_if` (and `alia_else_if`) can be signals
or raw C++ values. If a signal is provided and the signal has no value, alia
considers the condition *neither true nor false.* As such, the code dependent on
that statement isn't executed, but any subsequent `else` blocks are *also not
considered.* A condition without a value essentially terminates the entire
sequence of `alia_if/else` statements. As you can see in the above example,
before `n` is given a value, none of the `do_text` calls are executed.

As with all alia control flow macros, `alia_if` blocks must be terminated with
`alia_end`.

### Context Naming

All alia control flow macros need access to the alia *context*, so they assume
it's available in the `ctx` variable. It will make your life much easier if you
adopt the convention of always naming your context like this. If, however, you
find yourself in a situation where that's inconvenient or impossible, the macros
are all available in an alternate form that ends with an underscore and takes
the context as a first argument. For example:

```cpp
alia_if_(my_oddly_named_context, n < 0)
{
    dom::do_text(my_oddly_named_context, "Hi!");
}
alia_end
```

(These also come in uppercase form as well.)

alia_switch
-----------

alia provides a similar set of macros that mirror the behavior of `switch`,
`case`, and `default`. Note that all the normal semantics of `switch` statements
work as expected, including fallthrough:

```cpp
dom::do_text(ctx, "Enter a number:");
dom::do_input(ctx, n);
alia_switch(n)
{
 alia_case(0):
    dom::do_text(ctx, "foo");
    break;
 alia_case(1):
    dom::do_text(ctx, "bar");
 alia_case(2):
 alia_case(3):
    dom::do_text(ctx, "baz");
    break;
 alia_default:
    dom::do_text(ctx, "zub");
}
alia_end
```

<div class="demo-panel">
<div id="switch-example"></div>
</div>

The value passed to `alia_switch` is a signal, and just like `alia_if`
conditionals, if that signal doesn't have a value, none of the cases will match
(including the default, if any).

for_each
--------

`for_each` is the preferred way to loop over containers in alia. It takes care
of flow tracking for you, and it operates on signals: you pass in the container
as a signal, and it passes items back to you as signals.

<dl>

<dt>for_each(ctx, container, f)</dt><dd>

Invoke `f` for each item in `container`. (`container` must be a signal.)

If `container` carries a map-like container (i.e., a container that associates
keys with values), `f` is invoked as `f(ctx, key, value)`, where `key` and
`value` are both signals.

Otherwise, `f` is invoked as `f(ctx, item)`, where `item` is a signal.

In either case, `ctx` is the original context passed to `for_each` (and has the
same contents).

</dd>

</dl>

Here's an example of using `for_each` with a `std::map`:

```cpp
void
do_scoreboard(dom::context ctx, duplex<std::map<std::string, int>> scores)
{
    for_each(ctx, scores,
        [](auto ctx, auto player, auto score) {
            dom::scoped_div div(ctx, value("item"));
            dom::do_heading(ctx, "h4", player);
            dom::do_text(ctx, printf(ctx, "%d points", score));
            dom::do_button(ctx, "GOAL!", ++score);
        });

    auto new_player = get_state(ctx, string());
    dom::do_input(ctx, new_player);
    dom::do_button(ctx, "Add Player",
        (scores[new_player] <<= mask(0, new_player != ""),
         new_player <<= ""));
}
```

<div class="demo-panel">
<div id="for-each-map-demo"></div>
</div>

And here's an equivalent example using a `std::vector` to represent the same
data:

```cpp
struct player
{
    std::string name;
    int score;
};

void
do_scoreboard(dom::context ctx, duplex<std::vector<player>> players)
{
    for_each(ctx, players,
        [](auto ctx, auto player) {
            dom::scoped_div div(ctx, value("item"));
            do_heading(ctx, "h4", alia_field(player, name));
            do_text(ctx, printf(ctx, "%d points", alia_field(player, score)));
            do_button(ctx, "GOAL!", ++alia_field(player, score));
        });

    auto new_player = get_state(ctx, string());
    do_input(ctx, new_player);
    do_button(ctx, "Add Player",
        (push_back(players) <<
            apply(ctx,
                [](auto name) { return player{name, 0}; },
                mask(new_player, new_player != "")),
         new_player <<= ""));
}
```

<div class="demo-panel">
<div id="for-each-vector-demo"></div>
</div>

### Item/Data Associations

`for_each` tries to do a reasonable job of associating items consistently with
the same nodes in the data graph, even if those items move around within the
container. For map-like containers, it identifies items by their key. For lists,
it uses the address of the item, which is stable. Otherwise, it simply uses the
index, which of course isn't stable.

If you use `for_each` on a `std::vector` where items can move around, *the
underlying data associated with each item will change when the item moves.*
Depending on what you're associating with your items, the effects of this can
vary from slight inefficiencies to complete discontinuities in your interface.

You can override the default association by defining a `get_alia_id` function
for your item's type. (It should be accessible from within the `alia` namespace,
either via ADL or because it's defined there.) It should take the item as a
parameter and return an alia ID. (See [Working with IDs](working-with-ids.md).)
It can also return `null_id` to fall back to the default ID behavior.

transform
---------

Often, we loop over signal containers so that we can *map* the individual items
to other values. Just as `std::transform` allows you to do this for raw C++
containers, `alia::transform` allows you to do this for signal containers.

`alia::transform` is documented [here](function-application.md#transform), along
with the other signal function application utilities.

alia_for/while
--------------

!> Although alia provides macros for tracking `for` and `while` loops, these do
   *not* integrate naturally into alia applications the way that the `if` and
   `switch` versions do. There's an inherent tension between alia's declarative
   style and the imperative looping constructs like `for` and `while`. While
   alia normally discourages introducing immediate, imperative-style side
   effects into your component-level code, `for` and `while` *depend* on those
   side effects to determine when to terminate the loop. And since alia's
   signals are meant to capture values that change over the life of the
   application (and *not* within a single traversal of your application
   content), `for` and `while` are essentially incompatible with signals.<br>
   <br> If possible, you should use `for_each` (above), since that plays nicely
   with signals and avoids introducing immediate side effects into your
   component-level code. The `alia_for` and `alia_while` macros are provided
   largely as a convenience for applications that are trying to transition to
   alia with a minimum of effort.

!> Also note that these macros *assume that your iteration order remains
   constant.* If the data that you're looping over changes order (including
   adding or removing items in the middle of the sequence), *this will cause a
   shift in the underlying alia objects that are associated with your items.*
   Depending on what you're associating with your items, the effects of this can
   vary from slight inefficiencies to complete discontinuities in your
   interface. If you want to avoid this, you should use `named_block`s instead
   of these macros.

If you've read all the above and still want to use `alia_for` or `alia_while`,
here's an example of how you might use `alia_for` to hook up an existing
application's data structures to alia:

```cpp
// Here's an application data type, which has nothing to do with alia.
struct my_record
{
    std::string label;
    int x = 0, y = 0;
};

// Here's a function that operates on that type and also has nothing to do with
// alia.
bool
in_bounds(my_record const& r)
{
    return r.x >= 0 && r.x < 100 && r.y >= -20 && r.y < 20;
}

// Here's an alia function that exposes our application data via asm-dom.
void
do_records_ui(dom::context ctx, std::vector<my_record>& records)
{
    // Loop through our records and present a UI for each of them...
    // Since we're trying to integrate our application's data structure with the
    // least possible effort, we'll just loop through them as we would in a
    // normal C++ for loop but using alia_for instead, since it provides
    // tracking.
    alia_for(auto& record : records)
    {
        dom::scoped_div div(ctx, value("item"));

        // And now, at the point that we actually connect our individual records
        // to our widgets, we'll just use 'direct' to create signals that
        // directly connect our record's fields to the widgets.
        dom::do_heading(ctx, "h4", direct(record.label));
        dom::do_input(ctx, direct(record.x));
        dom::do_input(ctx, direct(record.y));

        // Display a warning if the record fails our in_bounds check.
        // Note that although alia provides signal-based dataflow mechanics for
        // doing this test, we can also just call the function like we would in
        // normal C++ code...
        alia_if(!in_bounds(record))
        {
            // Apparently the Docsify CSS class for a warning is 'tip'.
            dom::scoped_div div(ctx, value("tip"));
            do_text(ctx, "This is out of bounds!");
        }
        alia_end
    }
    alia_end

    // Also present a little UI for adding new records...
    {
        // Get some local state for the new label.
        auto new_label = get_state(ctx, string());
        dom::do_input(ctx, new_label);

        // Create an action that adds the new record.
        auto add_record = lambda_action(
            [&](std::string label) { records.push_back({label}); });

        // Present a button that adds the new record.
        dom::do_button(ctx, "Add",
            // Hook up the new label to our action (if the label isn't empty).
            (add_record << mask(new_label, new_label != ""),
             // Also add in an action that resets the label.
             new_label <<= ""));
    }
}
```

<div class="demo-panel">
<div id="loop-macros-demo"></div>
</div>

named_block
-----------

If you need more control over how you iterate over your data structures, you can
use named blocks. By 'naming' a block of code, you ensure that the piece of the
data graph that's associated with that code is always the same, regardless of
what order your blocks appear in in the execution of your code. A typical usage
follows this form:

```cpp
naming_context nc(ctx);
for(/* Iterate over your items however you want. */)
{
    named_block nb(nc, make_id(id_for_this_item));

    // Do the UI for this item, as you normally would...
}
```

The naming context anchors the named blocks to the data graph. It also provides
a unique context for naming, so if you need to iterate over those same items in
another part of your UI, you can use the same IDs without worrying about
collisions.

`named_block` is a scoped object. It takes effect immediately upon construction,
and when its scope ends and it's destructed, your code will naturally resume
using the normal alia data graph. It also provides `begin` and `end` member
functions in case you need to control its scope more explicitly.

See [Working with IDs](working-with-ids.md) for info about generating the IDs
for your blocks.

Here's a full example in action:

```cpp
struct my_record
{
    std::string id, label;
    int x = 0, y = 0;
};

void
do_records_ui(dom::context ctx, std::vector<my_record>& records)
{
    naming_context nc(ctx);
    for(auto& record : records)
    {
        named_block nb(nc, make_id(record.id));

        // Do the controls for this record, like we normally would...

        dom::scoped_div div(ctx, value("item"));

        do_heading(ctx, "h4", direct(record.label));
        do_input(ctx, direct(record.x));
        do_input(ctx, direct(record.y));

        // Just to demonstrate that each record is associated with the same data
        // block, we'll get some local state here. Feel free to type something
        // in here and shuffle the records to see what happens...
        do_text(ctx, "Local UI state associated with this record:");
        do_input(ctx, get_state(ctx, ""));
    }

    do_button(ctx, "Shuffle!",
        lambda_action(
            [&]() { std::shuffle(records.begin(), records.end(), rng); }));

}
```

<div class="demo-panel">
<div id="named-blocks-demo"></div>
</div>
