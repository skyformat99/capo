/*
    The Capo Library
    Code covered by the MIT License

    Author: mutouyun (http://darkc.at)
*/

#pragma once

#include "capo/detect_plat.hpp"
#include "capo/type_name.hpp"

#include <iostream>     // std::cout, std::cerr, std::clog
#include <string>       // std::string
#include <stdexcept>    // std::invalid_argument
#include <type_traits>  // std::is_convertible, std::decay
#include <utility>      // std::forward, std::move
#include <cstdint>      // intmax_t, uintmax_t
#include <cstddef>      // size_t, ptrdiff_t
#include <cwchar>       // wint_t
#include <cstdio>       // vsnprintf
#include <cstdarg>      // va_list, va_start, va_end

#if defined(CAPO_OS_WIN_)
#   include <windows.h> // ::OutputDebugStringA
#endif

namespace capo {

////////////////////////////////////////////////////////////////
/// Define standard stream policies
////////////////////////////////////////////////////////////////

namespace use
{
    inline static void std_cout(const std::string& str)
    {
        std::cout << str;
    }

    inline static void std_cerr(const std::string& str)
    {
#   if defined(CAPO_OS_WIN_)
        ::OutputDebugStringA(str.c_str());
#   endif
        std::cerr << str;
    };

    inline static void std_clog(const std::string& str)
    {
        std::clog << str;
    };
}

////////////////////////////////////////////////////////////////
/// Perpare for type-safe printf
////////////////////////////////////////////////////////////////

namespace detail_printf
{
    inline void enforce(std::string&& what)
    {
        throw std::invalid_argument
        {
            "Invalid format: " + what + "."
        };
    }

    template <typename T, typename U>
    inline void enforce(void)
    {
        if (!std::is_convertible<T, U>::value)
        {
            enforce(type_name<T>() + " => " + type_name<U>());
        }
    }

    template <typename T>
    void enforce_argument(const char* fmt)
    {
        using t_t = typename std::decay<T>::type;
        enum class length_t
        {
            none, h, hh, l, ll, j, z, t, L
        }
        state = length_t::none;
        for (; *fmt; ++fmt)
        {
            switch(*fmt)
            {
            // check specifiers's length
            case 'h':
                 if (state == length_t::h) state = length_t::hh;
                 else state = length_t::h; break;
            case 'l':
                 if (state == length_t::l) state = length_t::ll;
                 else state = length_t::l; break;
            case 'j': state = length_t::j; break;
            case 'z': state = length_t::z; break;
            case 't': state = length_t::t; break;
            case 'L': state = length_t::L; break;

            // check specifiers
            case 'd': case 'i':
                switch(state)
                {
                default:
                case length_t::none: enforce<t_t, int>();       break;
                case length_t::h:    enforce<t_t, short>();     break;
                case length_t::hh:   enforce<t_t, char>();      break;
                case length_t::l:    enforce<t_t, long>();      break;
                case length_t::ll:   enforce<t_t, long long>(); break;
                case length_t::j:    enforce<t_t, intmax_t>();  break;
                case length_t::z:    enforce<t_t, size_t>();    break;
                case length_t::t:    enforce<t_t, ptrdiff_t>(); break;
                }
                return;
            case 'u': case 'o': case 'x': case 'X':
                switch(state)
                {
                default:
                case length_t::none: enforce<t_t, unsigned int>();       break;
                case length_t::h:    enforce<t_t, unsigned short>();     break;
                case length_t::hh:   enforce<t_t, unsigned char>();      break;
                case length_t::l:    enforce<t_t, unsigned long>();      break;
                case length_t::ll:   enforce<t_t, unsigned long long>(); break;
                case length_t::j:    enforce<t_t, uintmax_t>();          break;
                case length_t::z:    enforce<t_t, size_t>();             break;
                case length_t::t:    enforce<t_t, ptrdiff_t>();          break;
                }
                return;
            case 'f': case 'F': case 'e': case 'E': case 'g': case 'G': case 'a': case 'A':
                switch(state)
                {
                default:
                case length_t::none: enforce<t_t, double>();      break;
                case length_t::L:    enforce<t_t, long double>(); break;
                }
                return;
            case 'c':
                switch(state)
                {
                default:
                case length_t::none: enforce<t_t, char>();    break;
                case length_t::l:    enforce<t_t, wchar_t>(); break;
                }
                return;
            case 's':
                switch(state)
                {
                default:
                case length_t::none: enforce<t_t, const char*>();    break;
                case length_t::l:    enforce<t_t, const wchar_t*>(); break;
                }
                return;
            case 'p':
                enforce<t_t, void*>();
                return;
            case 'n':
                switch(state)
                {
                default:
                case length_t::none: enforce<t_t, int*>();       break;
                case length_t::h:    enforce<t_t, short*>();     break;
                case length_t::hh:   enforce<t_t, char*>();      break;
                case length_t::l:    enforce<t_t, long*>();      break;
                case length_t::ll:   enforce<t_t, long long*>(); break;
                case length_t::j:    enforce<t_t, intmax_t*>();  break;
                case length_t::z:    enforce<t_t, size_t*>();    break;
                case length_t::t:    enforce<t_t, ptrdiff_t*>(); break;
                }
                return;
            }
        }
        enforce("Has no specifier");
    }

    void check(const char* fmt)
    {
        for (; *fmt; ++fmt)
        {
            if (*fmt != '%' || *++fmt == '%') continue;
            enforce("Bad format");
        }
    }

    template <typename T1, typename... T>
    void check(const char* fmt, T1&& /*a1*/, T&&... args)
    {
        for (; *fmt; ++fmt)
        {
            if (*fmt != '%' || *++fmt == '%') continue;
            enforce_argument<T1>(fmt);
            return check(++fmt, args...);
        }
        enforce("Too few format specifiers");
    }

    template <typename F>
    int output(F&& call_out, const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        std::string buf;
        int n = ::vsnprintf(NULL, 0, fmt, args);
        if (n <= 0) goto exit_printf;
        buf.resize(n);
        n = ::vsnprintf(const_cast<char*>(buf.data()), n + 1, fmt, args);
        if (n <= 0) goto exit_printf;
        call_out(buf);
    exit_printf:
        va_end(args);
        return n;
    }
}

////////////////////////////////////////////////////////////////
/// Print formatted data to standard stream
////////////////////////////////////////////////////////////////

template <typename F, typename... T>
int printf(F&& call_out, const char* fmt, T&&... args)
{
    if (!fmt) return 0;
    detail_printf::check(fmt, std::forward<T>(args)...);
    return detail_printf::output(std::forward<F>(call_out), fmt, std::forward<T>(args)...);
}

} // namespace capo
