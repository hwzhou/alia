#include <alia/system/internals.hpp>

#include <chrono>

namespace alia {

millisecond_count
default_external_interface::get_tick_count() const
{
    static auto start = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<
               std::chrono::duration<millisecond_count, std::milli>>(
               now - start)
        .count();
}

void
default_external_interface::schedule_timer_event(
    routable_component_id component, millisecond_count time)
{
    schedule_event(owner.scheduler, component, time);
}

void
initialize_system(
    system& sys,
    std::function<void(context)> const& controller,
    external_interface* external)
{
    sys.controller = controller;
    sys.external.reset(
        external ? external : new default_external_interface(sys));
}

void
process_internal_timing_events(system& sys, millisecond_count now)
{
    issue_ready_events(
        sys.scheduler,
        now,
        [&](routable_component_id component, millisecond_count trigger_time) {
            timer_event event;
            event.trigger_time = trigger_time;
            dispatch_targeted_event(sys, event, component);
        });
}

} // namespace alia
