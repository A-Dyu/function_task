#pragma once
#include "storage.h"

template <typename F>
struct function;

template <typename R, typename... Args>
struct function<R (Args...)>{
    function() = default;

    function(function const&) = default;

    function(function&&) = default;

    template <typename T>
    function(T val): stg(std::move(val)) {}

    function& operator=(function const& rhs) {
        if (this != &rhs) {
            function safe(rhs);
            swap(safe);
        }
        return *this;
    }

    function& operator=(function&& rhs) noexcept {
        if (this != &rhs) {
            function safe(std::move(rhs));
            swap(safe);
        }
        return *this;
    }

    explicit operator bool() const noexcept {
        return static_cast<bool>(stg);
    }

    R operator()(Args... args) const {
        return stg.invoke(std::forward<Args>(args)...);
    }

    template <typename T>
    T* target() noexcept {
        return stg.template target<T>();
    }

    template <typename T>
    T const* target() const noexcept {
        return stg.template target<T>();
    }

    void swap(function& other) {
        stg.swap(other.stg);
    }

private:
    mutable function_structs::storage<R, Args...> stg;
};
