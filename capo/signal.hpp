/*
    The Capo Library
    Code covered by the MIT License

    Author: mutouyun (http://darkc.at)
*/

#pragma once

#include "capo/vector.hpp"
#include "capo/type_traits.hpp"
#include "capo/types_to_seq.hpp"
#include "capo/max_min.hpp"

#include <utility>  // std::forward
#include <cstddef>  // size_t

namespace capo {
namespace detail_signal_ {

////////////////////////////////////////////////////////////////

// Function traits

template <typename S, typename F>
struct traits
     : std::conditional<std::is_same<void, typename capo::function_traits<F>::type>::value,
                        capo::function_traits<S>, capo::function_traits<F>>::type
{};

// Get suitable constant_seq<N...>

template <typename F, typename... P>
using suitable_size = 
    size_to_seq<min_number<std::tuple_size<typename traits<void(P...), F>::parameters>::value, sizeof...(P)>::value>;

/*
    Define slot struct
*/

template <typename R, typename... P>
struct slot
{
    virtual ~slot(void) {}
    virtual R call(P...) = 0;
};

template <class C, typename F, typename R, typename... P>
struct slot_fn;

template <typename F, typename... P>
struct slot_fn<void, F, void, P...> : slot<void, P...>
{
    F f_;

    slot_fn(F f) : f_(f) {}

    template <int... N, typename Tp>
    void forward(constant_seq<N...>, Tp&& tp)
    {
        f_(std::get<N>(std::forward<Tp>(tp))...);
    }

    void call(P... args) override
    {
        this->forward(suitable_size<F, P...>{}, std::forward_as_tuple(static_cast<P&&>(args)...));
    }
};

template <typename F, typename R, typename... P>
struct slot_fn<void, F, R, P...> : slot<R, P...>
{
    F f_;

    slot_fn(F f) : f_(f) {}

    template <int... N, typename Tp>
    R forward(constant_seq<N...>, Tp&& tp)
    {
        return f_(std::get<N>(std::forward<Tp>(tp))...);
    }

    R call(P... args) override
    {
        return this->forward(suitable_size<F, P...>{}, std::forward_as_tuple(static_cast<P&&>(args)...));
    }
};

template <class C, typename F, typename... P>
struct slot_fn<C, F, void, P...> : slot<void, P...>
{
    C c_;
    F f_;

    slot_fn(C c, F f) : c_(c), f_(f) {}

    template <int... N, typename Tp>
    void forward(constant_seq<N...>, Tp&& tp)
    {
        (c_->*f_)(std::get<N>(std::forward<Tp>(tp))...);
    }

    void call(P... args) override
    {
        this->forward(suitable_size<F, P...>{}, std::forward_as_tuple(static_cast<P&&>(args)...));
    }
};

template <class C, typename F, typename R, typename... P>
struct slot_fn : slot<R, P...>
{
    C c_;
    F f_;

    slot_fn(C c, F f) : c_(c), f_(f) {}

    template <int... N, typename Tp>
    R forward(constant_seq<N...>, Tp&& tp)
    {
        return (c_->*f_)(std::get<N>(std::forward<Tp>(tp))...);
    }

    R call(P... args) override
    {
        return this->forward(suitable_size<F, P...>{}, std::forward_as_tuple(static_cast<P&&>(args)...));
    }
};

/*
    Define signal base class
*/

template <typename F>
class signal_b;

template <typename R, typename... P>
class signal_b<R(P...)>
{
protected:
    capo::vector<slot<R, P...>*> slots_;

public:
    template <typename C, typename F>
    void connect(C&& receiver, F&& slot)
    {
        slots_.push_back(new slot_fn<typename std::remove_reference<C>::type, 
                                     typename std::remove_reference<F>::type, R, P...>
        {
            std::forward<C>(receiver), std::forward<F>(slot)
        });
    }

    template <typename F>
    void connect(F&& slot)
    {
        slots_.push_back(new slot_fn<void, typename std::remove_reference<F>::type, R, P...>
        {
            std::forward<F>(slot)
        });
    }

    template <typename C, typename F>
    void disconnect(C&& receiver, F&& slot)
    {
        using slot_t = slot_fn<typename std::remove_reference<C>::type,
                               typename std::remove_reference<F>::type, R, P...>;
        auto it = slots_.begin();
        for (; it != slots_.end(); ++it)
        {
            auto sp = dynamic_cast<slot_t*>(*it);
            if (sp == nullptr) continue;
            if ( (sp->c_ == std::forward<C>(receiver)) && 
                 (sp->f_ == std::forward<F>(slot)) )
            {
                delete sp;
                break;
            }
        }
        if (it != slots_.end()) slots_.erase(it);
    }

    template <typename F>
    void disconnect(F&& slot)
    {
        using slot_t = slot_fn<void, typename std::remove_reference<F>::type, R, P...>;
        auto it = slots_.begin();
        for (; it != slots_.end(); ++it)
        {
            auto sp = dynamic_cast<slot_t*>(*it);
            if (sp == nullptr) continue;
            if (sp->f_ == std::forward<F>(slot))
            {
                delete sp;
                break;
            }
        }
        if (it != slots_.end()) slots_.erase(it);
    }

    void disconnect(void)
    {
        for (auto sp : slots_) delete sp;
        slots_.clear();
    }
};

} // namespace detail_signal_

////////////////////////////////////////////////////////////////
/// Define signal-slot service class
////////////////////////////////////////////////////////////////

template <typename F>
class signal;

template <typename... P>
class signal<void(P...)> : public detail_signal_::signal_b<void(P...)>
{
public:
    template <typename... A>
    void operator()(A&&... args)
    {
        for (auto sp : this->slots_)
        {
            if (sp == nullptr) continue;
            sp->call(std::forward<A>(args)...);
        }
    }
};

template <typename R, typename... P>
class signal<R(P...)> : public detail_signal_::signal_b<R(P...)>
{
public:
    template <typename... A>
    R operator()(A&&... args)
    {
        if (this->slots_.empty()) return {};
        for (size_t i = 0; i < this->slots_.size() - 1; ++i)
        {
            auto sp = this->slots_[i];
            if (sp == nullptr) continue;
            sp->call(std::forward<A>(args)...);
        }
        auto sp = this->slots_.back();
        if (sp == nullptr) return {};
        return sp->call(std::forward<A>(args)...);
    }
};

////////////////////////////////////////////////////////////////

} // namespace capo
