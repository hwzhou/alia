The Context
===========

<script>
    init_alia_demos(['custom-context', 'doubly-extended-context']);
</script>

Almost every function in alia takes a context as its first parameter. The
context is how application code accesses the utilities provided by alia: the
data graph, event dispatch info, etc.

The context is designed to be extended with externally defined objects to give
applications additional capabilities. All of the examples in this documentation
use a `dom::context`, which is defined by the asm-dom wrapper. `dom::context`
extends `alia::context` to add the ability to interact with asm-dom and control
a portion of the DOM.

A context is essentially an unordered collection of objects. Context objects
have compile-time tags (to identify them) and run-time data (so they can do
things). Contexts use [structural
typing](https://en.wikipedia.org/wiki/Structural_type_system). When calling
another function, the context you provide must simply have a superset of the
objects that the function requires. (Unlike what might happen in an
inheritance-based system, it doesn't matter what order you add the objects to
your context.)

Applications are also free to extend the context. The context can be a good
place to store information and resources that are repeatedly accessed throughout
your application code: the active user, session info, workspace info, I/O
objects, etc. The information doesn't even have to be global to your
application. You might create different contexts for different pages or displays
within your application.

Here's an example of how you might use a custom context to establish a username
as a globally accessible property in your app.

```cpp

// Define our username context tag.
// At the moment, all context objects must be stored by reference, so although
// we wouldn't normally use a reference for a readable<std::string>, we have to
// do so here.
ALIA_DEFINE_TAGGED_TYPE(username_tag, readable<std::string>&)

// Define our app's context type by extending the asm-dom context type.
typedef extend_context_type_t<dom::context, username_tag> app_context;

// The functions that define the internal portions of our app UI use
// app_context and have access to the username via that context...
void
do_internal_app_ui(app_context ctx)
{
    do_text(ctx, printf(ctx, "Welcome, %s!", ctx.get<username_tag>()));
}

// Our top-level UI function takes the context that the asm-dom wrapper provides
// and extends it to what our app needs...
void
do_main_app_ui(dom::context ctx)
{
    // Get the username.
    // (Maybe in a real app this wouldn't be hardcoded...)
    readable<std::string> username = value("tmadden");

    // Extend the app context to include the username.
    app_context app_ctx = ctx.add<username_tag>(username);

    // Pass that context along to the internal portions of the app UI...
    do_internal_app_ui(app_ctx);
}
```

<div class="demo-panel">
<div id="custom-context"></div>
</div>

And of course, your application can extend the context with as many objects as
it wants...

```cpp
ALIA_DEFINE_TAGGED_TYPE(username_tag, readable<std::string>&)
ALIA_DEFINE_TAGGED_TYPE(api_key_tag, readable<std::string>&)

typedef extend_context_type_t<dom::context, username_tag, api_key_tag>
    app_context;

void
do_internal_app_ui(app_context ctx)
{
    do_text(ctx, printf(ctx, "Welcome, %s!", ctx.get<username_tag>()));
    do_text(ctx,
        printf(ctx,
            "Your secret key is %s! Keep it safe!",
            ctx.get<api_key_tag>()));
}

void
do_main_app_ui(dom::context ctx)
{
    readable<std::string> username = value("tmadden");
    readable<std::string> api_key = value("abc123");

    app_context app_ctx =
        ctx.add<username_tag>(username).add<api_key_tag>(api_key);

    do_internal_app_ui(app_ctx);
}
```

<div class="demo-panel">
<div id="doubly-extended-context"></div>
</div>
