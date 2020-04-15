Assorted Examples
=================

<script>
    init_alia_demos(['tip-calculator-demo', 'loop-macros-demo',
        'for-each-map-demo', 'fetch-country', 'time-signal',
        'number-smoothing', 'color-smoothing', 'factor-tree']);
</script>

If you're more interested in code than prose, you'll like this page the best.
Many of these come from other sections of the documentation...

Tip Calculator
--------------

Here's a simple tip calculator that shows off some many of the features of alia:
[actions](actions.md), conditional widgets, and how you can use alia's data
graph to 'magically' manifest state when and where you need it, even in the
middle of declarative component code.

```cpp
void
do_tip_calculator(dom::context ctx)
{
    // Get some component-local state for the bill amount.
    auto bill = get_state(ctx, empty<double>());
    dom::do_text(ctx, "How much is the bill?");
    // Display an input that allows the user to manipulate our bill state.
    dom::do_input(ctx, bill);

    // Get some more component-local state for the tip rate.
    auto tip_rate = get_state(ctx, empty<double>());
    dom::do_text(ctx, "What percentage do you want to tip?");
    // Users like percentages, but we want to keep the 'tip_rate' state as a
    // rate internally, so this input presents a scaled view of it for the user.
    dom::do_input(ctx, scale(tip_rate, 100));
    // Add a few buttons that set the tip rate to common values.
    dom::do_button(ctx, "18%", tip_rate <<= 0.18);
    dom::do_button(ctx, "20%", tip_rate <<= 0.20);
    dom::do_button(ctx, "25%", tip_rate <<= 0.25);

    // Calculate the results and display them for the user.
    // Note that these operations have dataflow semantics, and since `bill` and
    // `tip_rate` both start out empty, nothing will actually be calculated
    // (or displayed) until the user supplies values for them.
    auto tip = bill * tip_rate;
    auto total = bill + tip;
    dom::do_text(ctx,
        printf(ctx, "You should tip %.2f, for a total of %.2f.", tip, total));

    // Conditionally display a message suggesting cash for small amounts.
    alia_if (total < 10)
    {
        dom::do_text(ctx,
            "You should consider using cash for small amounts like this.");
    }
    alia_end
}
```

<div class="demo-panel">
<div id="tip-calculator-demo"></div>
</div>

Containers
----------

Here's an (admittedly contrived) example of working with containers in alia.
It uses a `std::map` to map player names to their scores.

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

Minimal Integration
-------------------

Although alia provides a whole suite of tools for modeling your application's
presentation logic declaratively, it tries not to *force* them on you wholesale.
Here's an example of how you might use a smaller subset of alia's capabilities
to integrate alia with your application data.

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

Factor Trees
------------

This example displays factorization trees for numbers that you enter. (It
implements a poor man's tree view by allowing the user to show and hide the
subtree associated with each composite factor.)

What's interesting about this is that there's actually no application data that
mirrors this tree view. The application 'model' consists entirely of a single
integer. The structure of the UI (and the state that allows the user to expand
and collapse nodes in the tree) is fully defined by the recursive structure of
the calls to `do_factor_tree`.

```cpp
void
do_factor_tree(dom::context ctx, readable<int> n)
{
    dom::scoped_div div(ctx, value("subtree"));

    // Get the 'best' factor that n has. (The one closest to sqrt(n).)
    auto f = apply(ctx, factor, n);

    // If that factor is 1, n is prime.
    alia_if(f != 1)
    {
        dom::do_text(ctx, printf(ctx, "%i: composite", n));

        // Allow the user to expand this block to see more factor.
        auto expanded = get_state(ctx, false);
        dom::do_button(ctx,
            conditional(expanded, "Hide Factors", "Show Factors"),
            toggle(expanded));
        alia_if(expanded)
        {
            do_factor_tree(ctx, f);
            do_factor_tree(ctx, n / f);
        }
        alia_end
    }
    alia_else
    {
        dom::do_text(ctx, printf(ctx, "%i: prime", n));
    }
    alia_end
}

// And here's the demo UI that invokes the top-level of the tree:
void
do_factor_tree_demo(dom::context ctx, duplex<int> n)
{
    dom::do_text(ctx, "Enter a number:");
    dom::do_input(ctx, n);
    do_factor_tree(ctx, n);
}
```

<div class="demo-panel">
<div id="factor-tree"></div>
</div>

Asynchronous I/O
----------------

This shows an example of using `alia::async()` to integrate an asynchronous API.
In this case, we're using Emscripten's fetch API to remotely look up country
names on [REST Countries](https://restcountries.eu).

```cpp
// This is what the app sees as the result of the fetch.
struct fetch_result
{
    bool successful;
    std::string country_name;
};

// Here's our handler for the fetch results. (This is called asynchronously.)
void
handle_fetch_result(emscripten_fetch_t* fetch, fetch_result result)
{
    // Recover our callback from the Emscripten fetch object.
    auto* result_callback
        = reinterpret_cast<std::function<void(fetch_result)>*>(fetch->userData);

    // Report the result.
    (*result_callback)(result);

    // Clean up.
    delete result_callback;
    emscripten_fetch_close(fetch);
}
// And these are the actual callbacks that we give to Emscripten: one for
// success and one for failure...
void
handle_fetch_success(emscripten_fetch_t* fetch)
{
    // Parse the JSON response and pass it along.
    auto response = json::parse(fetch->data, fetch->data + fetch->numBytes);
    handle_fetch_result(fetch, fetch_result{true, response["name"]});
}
void
handle_fetch_failure(emscripten_fetch_t* fetch)
{
    // Pass along an unsuccessful/empty result.
    handle_fetch_result(fetch, fetch_result{false, std::string()});
}

// Here's our actual component-level function for retrieving country names.
auto
fetch_country_name(dom::context ctx, readable<std::string> country_code)
{
    // This will be invoked to launch the fetch operation whenever necessary
    // (i.e., whenever we get a new country code).
    auto launcher = [](auto ctx, auto report_result, auto country_code) {
        emscripten_fetch_attr_t attr;
        emscripten_fetch_attr_init(&attr);
        strcpy(attr.requestMethod, "GET");
        attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
        attr.onsuccess = handle_fetch_success;
        attr.onerror = handle_fetch_failure;
        attr.userData = new std::function<void(fetch_result)>(report_result);
        auto url = "https://restcountries.eu/rest/v2/alpha/" + country_code;
        emscripten_fetch(&attr, url.c_str());
    };
    // async() handles all the underlying logic of determining when to invoke
    // the launcher and how to get results back to this point in the UI.
    return async<fetch_result>(ctx, launcher, country_code);
}

// And here's the UI for interacting with it.
void
do_fetch_ui(dom::context ctx, duplex<std::string> country_code)
{
    dom::do_text(ctx, "Enter a country code:");
    dom::do_input(ctx, country_code);
    auto result = fetch_country_name(ctx, country_code);
    dom::do_text(
        ctx,
        add_fallback(
            deflicker(
                ctx,
                conditional(
                    alia_field(result, successful),
                    alia_field(result, country_name),
                    "Not found")),
            "Fetching..."));
}
```

<div class="demo-panel">
<div id="fetch-country"></div>
</div>

Timing Signals
--------------

Although all signals in alia are conceptually time-varying values, most of them
only care about the present (e.g., `a + b` is just whatever `a` is right now
plus whatever `b` is). However, some signals are more closely linked to time and
explicitly vary with it:

```cpp
dom::do_text(ctx,
    printf(ctx,
        "It's been %d seconds since you started looking at this page.",
        get_animation_tick_count(ctx) / 1000));
```

<div class="demo-panel">
<div id="time-signal"></div>
</div>

You can use this explicit notion of time to do fun things like smooth out other
signals:

```cpp
dom::do_text(ctx, "Enter N:");
dom::do_input(ctx, n);
dom::do_button(ctx, "1", n <<= 1);
dom::do_button(ctx, "100", n <<= 100);
dom::do_button(ctx, "10000", n <<= 10000);
dom::do_text(ctx, "Here's a smoothed view of N:");
dom::do_heading(ctx, "h4", smooth(ctx, n));
```

<div class="demo-panel">
<div id="number-smoothing"></div>
</div>

You can smooth anything that provides the basic arithmetic operators. Here's a
smoothed view of a color:

```cpp
dom::do_colored_box(ctx, smooth(ctx, color));
dom::do_button(ctx, "Go Light", color <<= rgb8(210, 210, 220));
dom::do_button(ctx, "Go Dark", color <<= rgb8(50, 50, 55));
```

<div class="demo-panel">
<div id="color-smoothing"></div>
</div>
