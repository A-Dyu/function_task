#pragma once
#include <exception>
#include <string>
#include <type_traits>
#include "type_descriptor.h"



template<typename R, typename... Args>
struct type_descriptor; /// Move, Copy, invoke, Destroy

template<typename R, typename... Args>
struct storage {
    using storage_t = storage<R, Args...>;

    storage() :buf(), desc(empty_type_descriptor<R, Args...>()) {}

    storage(storage const& other) {
        other.desc->copy(&other, this);
    }

    storage(storage&& other) {
        other.desc->move(&other, this);
    }

    ~storage() {
        desc->destroy(this);
    }

    template<typename T>
    T& get_static() noexcept {
        return *reinterpret_cast<T*>(&buf);
    }

    template<typename T>
    T const& get_static() const noexcept {
        return *reinterpret_cast<T const*>(&buf);
    }

    template<typename T>
    void set_static(T val) {
        new(&buf) T(std::forward<T>(val));
    }

    void set_dynamic(void* value) noexcept {
        reinterpret_cast<void*&>(buf) = value;
    }

    template<typename T>
    T* get_dynamic() const noexcept {
        return *reinterpret_cast<T* const*>(&buf);
    }

    void set_descriptor(type_descriptor<R, Args...> const* d) {
        desc = d;
    }

    R invoke(Args... args) {
        return desc->invoke(this, std::forward<Args>(args)...);
    }

    template<typename T>
    T const* target() const noexcept {
        if (check_target<T>()) {
            return nullptr;
        }
        return (fits_small_storage<T> ? &get_static<T>() : get_dynamic<T>());
    }

    template<typename T>
    T* target() noexcept {
        if (check_target<T>()) {
            return nullptr;
        }
        return (fits_small_storage<T> ? &get_static<T>() : get_dynamic<T>());
    }

    void swap(storage& other) {
        using std::swap;
        swap(buf, other.buf);
        swap(desc, other.desc);
    }

    template<typename U, typename... Arguments>
    friend type_descriptor<U, Arguments...> const* empty_type_descriptor();

    template<typename U, typename Arguments>
    friend struct functional_traits;

    template<typename T>
    friend struct function;

private:
    template<typename T>
    bool check_target() {
        if (functional_traits<T>::template get_type_descriptor<R, Args...>() != desc || desc == empty_type_descriptor<R, Args...>()) {
            return true;
        }
    }

    inplace_buffer buf;
    type_descriptor<R, Args...> const* desc;
};