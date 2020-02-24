#include <alia/signals/basic.hpp>

using namespace alia;

void
f_readable(readable<int>)
{
}

void
f_bidirectional(bidirectional<int>)
{
}

void
f()
{
    auto read_only = value(0);
    f_readable(read_only);
#ifdef ALIA_TEST_COMPILATION_FAILURE
    f_bidirectional(read_only);
#endif
}
