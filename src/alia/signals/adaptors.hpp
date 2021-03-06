#ifndef ALIA_SIGNALS_ADAPTORS_HPP
#define ALIA_SIGNALS_ADAPTORS_HPP

#include <alia/signals/basic.hpp>
#include <alia/signals/utilities.hpp>

namespace alia {

// signalize(x) turns x into a signal if it isn't already one.
// Or, in other words...
// signalize(s), where s is a signal, returns s.
// signalize(v), where v is a raw value, returns a value signal carrying v.
template<class Signal>
std::enable_if_t<is_readable_signal_type<Signal>::value, Signal>
signalize(Signal s)
{
    return s;
}
template<class Value, std::enable_if_t<!is_signal_type<Value>::value, int> = 0>
auto
signalize(Value v)
{
    return value(std::move(v));
}

// fake_readability(s), where :s is a signal, yields a wrapper for :s that
// pretends to have read capabilities. It will never actually have a value, but
// it will type-check as a readable signal.
template<class Wrapped>
struct readability_faker : signal<
                               readability_faker<Wrapped>,
                               typename Wrapped::value_type,
                               typename signal_direction_union<
                                   read_only_signal,
                                   typename Wrapped::direction_tag>::type>
{
    readability_faker(Wrapped wrapped) : wrapped_(wrapped)
    {
    }
    id_interface const&
    value_id() const
    {
        return null_id;
    }
    bool
    has_value() const
    {
        return false;
    }
    // Since this is only faking readability, read() should never be called.
    // LCOV_EXCL_START
    typename Wrapped::value_type const&
    read() const
    {
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnull-dereference"
#endif
        return *(typename Wrapped::value_type const*) nullptr;
#ifdef __clang__
#pragma clang diagnostic pop
#endif
    }
    // LCOV_EXCL_STOP
    bool
    ready_to_write() const
    {
        return wrapped_.ready_to_write();
    }
    void
    write(typename Wrapped::value_type const& value) const
    {
        return wrapped_.write(value);
    }

 private:
    Wrapped wrapped_;
};
template<class Wrapped>
readability_faker<Wrapped>
fake_readability(Wrapped const& wrapped)
{
    return readability_faker<Wrapped>(wrapped);
}

// fake_writability(s), where :s is a signal, yields a wrapper for :s that
// pretends to have write capabilities. It will never actually be ready to
// write, but it will type-check as a writable signal.
template<class Wrapped>
struct writability_faker : signal<
                               writability_faker<Wrapped>,
                               typename Wrapped::value_type,
                               typename signal_direction_union<
                                   write_only_signal,
                                   typename Wrapped::direction_tag>::type>
{
    writability_faker(Wrapped wrapped) : wrapped_(wrapped)
    {
    }
    id_interface const&
    value_id() const
    {
        return wrapped_.value_id();
    }
    bool
    has_value() const
    {
        return wrapped_.has_value();
    }
    typename Wrapped::value_type const&
    read() const
    {
        return wrapped_.read();
    }
    bool
    ready_to_write() const
    {
        return false;
    }
    // Since this is only faking writability, write() should never be called.
    // LCOV_EXCL_START
    void
    write(typename Wrapped::value_type const&) const
    {
    }
    // LCOV_EXCL_STOP

 private:
    Wrapped wrapped_;
};
template<class Wrapped>
writability_faker<Wrapped>
fake_writability(Wrapped const& wrapped)
{
    return writability_faker<Wrapped>(wrapped);
}

// signal_cast<Value>(x), where :x is a signal, yields a proxy for :x with
// the value type :Value. The proxy will apply static_casts to convert its
// own values to and from :x's value type.
template<class Wrapped, class To>
struct casting_signal : regular_signal<
                            casting_signal<Wrapped, To>,
                            To,
                            typename Wrapped::direction_tag>
{
    casting_signal(Wrapped wrapped) : wrapped_(wrapped)
    {
    }
    bool
    has_value() const
    {
        return wrapped_.has_value();
    }
    To const&
    read() const
    {
        return lazy_reader_.read(
            [&] { return static_cast<To>(wrapped_.read()); });
    }
    bool
    ready_to_write() const
    {
        return wrapped_.ready_to_write();
    }
    void
    write(To const& value) const
    {
        return wrapped_.write(static_cast<typename Wrapped::value_type>(value));
    }

 private:
    Wrapped wrapped_;
    lazy_reader<To> lazy_reader_;
};
// Don't use a casting_signal if the value type is the same.
template<class Wrapped, class To>
struct signal_caster
{
    typedef casting_signal<Wrapped, To> type;
    static type
    apply(Wrapped wrapped)
    {
        return type(wrapped);
    }
};
template<class Wrapped>
struct signal_caster<Wrapped, typename Wrapped::value_type>
{
    typedef Wrapped type;
    static type
    apply(Wrapped wrapped)
    {
        return wrapped;
    }
};
template<class To, class Wrapped>
typename signal_caster<Wrapped, To>::type
signal_cast(Wrapped wrapped)
{
    return signal_caster<Wrapped, To>::apply(wrapped);
}

// has_value(x) yields a signal to a boolean which indicates whether or not :x
// has a value. (The returned signal itself always has a value.)
template<class Wrapped>
struct value_presence_signal
    : regular_signal<value_presence_signal<Wrapped>, bool, read_only_signal>
{
    value_presence_signal(Wrapped const& wrapped) : wrapped_(wrapped)
    {
    }
    bool
    has_value() const
    {
        return true;
    }
    bool const&
    read() const
    {
        value_ = wrapped_.has_value();
        return value_;
    }

 private:
    Wrapped wrapped_;
    mutable bool value_;
};
template<class Wrapped>
auto
has_value(Wrapped const& wrapped)
{
    return value_presence_signal<Wrapped>(wrapped);
}

// ready_to_write(x) yields a signal to a boolean that indicates whether or not
// :x is ready to write. (The returned signal always has a value.)
template<class Wrapped>
struct write_readiness_signal
    : regular_signal<write_readiness_signal<Wrapped>, bool, read_only_signal>
{
    write_readiness_signal(Wrapped const& wrapped) : wrapped_(wrapped)
    {
    }
    bool
    has_value() const
    {
        return true;
    }
    bool const&
    read() const
    {
        value_ = wrapped_.ready_to_write();
        return value_;
    }

 private:
    Wrapped wrapped_;
    mutable bool value_;
};
template<class Wrapped>
auto
ready_to_write(Wrapped const& wrapped)
{
    return write_readiness_signal<Wrapped>(wrapped);
}

// add_fallback(primary, fallback), where :primary and :fallback are both
// signals, yields another signal whose value is that of :primary if it has one
// and that of :fallback otherwise.
// All writes go directly to :primary.
template<class Primary, class Fallback>
struct fallback_signal : signal<
                             fallback_signal<Primary, Fallback>,
                             typename Primary::value_type,
                             typename Primary::direction_tag>
{
    fallback_signal()
    {
    }
    fallback_signal(Primary primary, Fallback fallback)
        : primary_(primary), fallback_(fallback)
    {
    }
    bool
    has_value() const
    {
        return primary_.has_value() || fallback_.has_value();
    }
    typename Primary::value_type const&
    read() const
    {
        return primary_.has_value() ? primary_.read() : fallback_.read();
    }
    id_interface const&
    value_id() const
    {
        id_ = combine_ids(
            make_id(primary_.has_value()),
            primary_.has_value() ? ref(primary_.value_id())
                                 : ref(fallback_.value_id()));
        return id_;
    }
    bool
    ready_to_write() const
    {
        return primary_.ready_to_write();
    }
    void
    write(typename Primary::value_type const& value) const
    {
        primary_.write(value);
    }

 private:
    mutable id_pair<simple_id<bool>, id_ref> id_;
    Primary primary_;
    Fallback fallback_;
};
template<class Primary, class Fallback>
fallback_signal<Primary, Fallback>
make_fallback_signal(Primary primary, Fallback fallback)
{
    return fallback_signal<Primary, Fallback>(primary, fallback);
}
template<class Primary, class Fallback>
auto
add_fallback(Primary primary, Fallback fallback)
{
    return make_fallback_signal(signalize(primary), signalize(fallback));
}

// simplify_id(s), where :s is a signal, yields a wrapper for :s with the exact
// same read/write behavior but whose value ID is a simple_id (i.e., it is
// simply the value of the signal).
//
// The main utility of this is in cases where you have a signal carrying a
// small value with a complicated value ID (because it was picked from the
// signal of a larger data structure, for example). The more complicated ID
// might change superfluously.
//
template<class Wrapped>
struct simplified_id_wrapper : regular_signal<
                                   simplified_id_wrapper<Wrapped>,
                                   typename Wrapped::value_type,
                                   typename Wrapped::direction_tag>
{
    simplified_id_wrapper(Wrapped wrapped) : wrapped_(wrapped)
    {
    }
    bool
    has_value() const
    {
        return wrapped_.has_value();
    }
    typename Wrapped::value_type const&
    read() const
    {
        return wrapped_.read();
    }
    bool
    ready_to_write() const
    {
        return wrapped_.ready_to_write();
    }
    void
    write(typename Wrapped::value_type const& value) const
    {
        return wrapped_.write(value);
    }

 private:
    Wrapped wrapped_;
};
template<class Wrapped>
simplified_id_wrapper<Wrapped>
simplify_id(Wrapped wrapped)
{
    return simplified_id_wrapper<Wrapped>(wrapped);
}

// mask(signal, availibility_flag) does the equivalent of bit masking on
// individual signals. If :availibility_flag evaluates to true, the mask
// evaluates to signal equivalent to :signal. Otherwise, it evaluates to an
// empty signal of the same type.
template<class Primary, class Mask>
struct masking_signal : signal<
                            masking_signal<Primary, Mask>,
                            typename Primary::value_type,
                            typename Primary::direction_tag>
{
    masking_signal()
    {
    }
    masking_signal(Primary primary, Mask mask) : primary_(primary), mask_(mask)
    {
    }
    bool
    has_value() const
    {
        return mask_.has_value() && mask_.read() && primary_.has_value();
    }
    typename Primary::value_type const&
    read() const
    {
        return primary_.read();
    }
    id_interface const&
    value_id() const
    {
        if (mask_.has_value() && mask_.read())
            return primary_.value_id();
        else
            return null_id;
    }
    bool
    ready_to_write() const
    {
        return mask_.has_value() && mask_.read() && primary_.ready_to_write();
    }
    void
    write(typename Primary::value_type const& value) const
    {
        primary_.write(value);
    }

 private:
    Primary primary_;
    Mask mask_;
};
template<class Signal, class AvailabilityFlag>
auto
make_masking_signal(Signal signal, AvailabilityFlag availability_flag)
{
    return masking_signal<Signal, AvailabilityFlag>(signal, availability_flag);
}
template<class Signal, class AvailabilityFlag>
auto
mask(Signal signal, AvailabilityFlag availability_flag)
{
    return make_masking_signal(signalize(signal), signalize(availability_flag));
}

// mask_writes(signal, writability_flag) masks writes to :signal according to
// the value of :writability_flag.
//
// :writability_flag can be either a signal or a raw value. If it evaluates to
// true (in a boolean context), the mask evaluates to a signal equivalent to
// :signal. Otherwise, it evaluates to one with equivalent reading behavior but
// with writing disabled.
//
// Note that in either case, the masked version has the same directionality as
// :signal.
//
template<class Primary, class Mask>
struct write_masking_signal : signal<
                                  write_masking_signal<Primary, Mask>,
                                  typename Primary::value_type,
                                  typename Primary::direction_tag>
{
    write_masking_signal()
    {
    }
    write_masking_signal(Primary primary, Mask mask)
        : primary_(primary), mask_(mask)
    {
    }
    bool
    has_value() const
    {
        return primary_.has_value();
    }
    typename Primary::value_type const&
    read() const
    {
        return primary_.read();
    }
    id_interface const&
    value_id() const
    {
        return primary_.value_id();
    }
    bool
    ready_to_write() const
    {
        return mask_.has_value() && mask_.read() && primary_.ready_to_write();
    }
    void
    write(typename Primary::value_type const& value) const
    {
        primary_.write(value);
    }

 private:
    Primary primary_;
    Mask mask_;
};
template<class Signal, class WritabilityFlag>
auto
make_write_masking_signal(Signal signal, WritabilityFlag writability_flag)
{
    return write_masking_signal<Signal, WritabilityFlag>(
        signal, writability_flag);
}
template<class Signal, class WritabilityFlag>
auto
mask_writes(Signal signal, WritabilityFlag writability_flag)
{
    return make_write_masking_signal(signal, signalize(writability_flag));
}

// disable_writes(s), where :s is a signal, yields a wrapper for :s where writes
// are disabled. Like mask_signal, this doesn't change the directionality of :s.
template<class Signal>
auto
disable_writes(Signal s)
{
    return mask_writes(s, false);
}

// unwrap(signal), where :signal is a signal carrying a std::optional value,
// yields a signal that directly carries the value wrapped inside the optional.
template<class Wrapped>
struct unwrapper_signal : signal<
                              unwrapper_signal<Wrapped>,
                              typename Wrapped::value_type::value_type,
                              typename Wrapped::direction_tag>
{
    unwrapper_signal()
    {
    }
    unwrapper_signal(Wrapped wrapped) : wrapped_(wrapped)
    {
    }
    bool
    has_value() const
    {
        return wrapped_.has_value() && wrapped_.read().has_value();
    }
    typename Wrapped::value_type::value_type const&
    read() const
    {
        return wrapped_.read().value();
    }
    id_interface const&
    value_id() const
    {
        if (this->has_value())
            return wrapped_.value_id();
        else
            return null_id;
    }
    bool
    ready_to_write() const
    {
        return wrapped_.ready_to_write();
    }
    void
    write(typename Wrapped::value_type::value_type const& value) const
    {
        wrapped_.write(value);
    }

 private:
    Wrapped wrapped_;
};
template<class Signal>
auto
unwrap(Signal signal)
{
    return unwrapper_signal<Signal>(signal);
}

} // namespace alia

#endif
