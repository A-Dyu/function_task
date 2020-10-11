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
    function(T val) {
        if (fits_small_storage<T>) {
            stg.set_static(std::forward<T>(val));
        } else {
            stg.set_dynamic(new T(std::forward<T>(val)));
        }
        stg.set_descriptor(functional_traits<T>::template get_type_descriptor<R, Args...>());
    }

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
        return stg.desc != empty_type_descriptor<R, Args...>();
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
    mutable storage<R, Args...> stg;
};
