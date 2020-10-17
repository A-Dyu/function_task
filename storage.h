#pragma once
#include <exception>
#include <string>
#include <type_traits>
#include "type_descriptor.h"


namespace function_structs {
    template<typename R, typename... Args>
    struct type_descriptor; /// Move, Copy, invoke, Destroy

    template<typename R, typename... Args>
    struct storage {
        using storage_t = storage<R, Args...>;

        storage() : buf(), desc(empty_type_descriptor<R, Args...>()) {}

        storage(storage const &other) {
            other.desc->copy(&other, this);
        }

        storage(storage &&other) {
            other.desc->move(&other, this);
        }

        template<typename T>
        storage(T val): storage() {
            if constexpr (function_structs::fits_small_storage<T>) {
                set_static(std::move(val));
            } else {
                set_dynamic(new T(std::move(val)));
            }
            set_descriptor(function_structs::functional_traits<T>::template get_type_descriptor<R, Args...>());
        }

        storage &operator=(storage const &other) {
            if (this != &other) {
                storage safe(other);
                swap(safe);
            }
            return *this;
        }

        storage &operator=(storage &&other) {
            if (this != &other) {
                other.desc->move(&other, this);
            }
            return *this;
        }

        ~storage() {
            desc->destroy(this);
        }

        template<typename T>
        T &get_static() noexcept {
            return *reinterpret_cast<T *>(&buf);
        }

        template<typename T>
        T const &get_static() const noexcept {
            return *reinterpret_cast<T const *>(&buf);
        }

        template<typename T>
        void set_static(T val) {
            new(&buf) T(std::forward<T>(val));
        }

        void set_dynamic(void *value) noexcept {
            reinterpret_cast<void *&>(buf) = value;
        }

        template<typename T>
        T *get_dynamic() const noexcept {
            return *reinterpret_cast<T *const *>(&buf);
        }

        void set_descriptor(type_descriptor<R, Args...> const *d) {
            desc = d;
        }

        R invoke(Args... args) {
            return desc->invoke(this, std::forward<Args>(args)...);
        }

        template<typename T>
        T const *target() const noexcept {
            if (check_target<T>()) {
                return nullptr;
            }
            if constexpr (fits_small_storage < T >) {
                return &get_static<T>();
            } else {
                return get_dynamic<T>();
            }
        }

        template<typename T>
        T *target() noexcept {
            if (check_target<T>()) {
                return nullptr;
            }
            if constexpr (fits_small_storage < T >) {
                return &get_static<T>();
            } else {
                return get_dynamic<T>();
            }
        }

        explicit operator bool() const noexcept {
            return desc != function_structs::empty_type_descriptor<R, Args...>();
        }

        void swap(storage &other) {
            auto buffer = std::move(other);
            other = std::move(*this);
            *this = std::move(buffer);
        }

        template<typename U, typename... Arguments>
        friend type_descriptor<U, Arguments...> const *empty_type_descriptor();

        template<typename U, typename Arguments>
        friend struct functional_traits;

    private:
        template<typename T>
        bool check_target() {
            return functional_traits<T>::template get_type_descriptor<R, Args...>() != desc ||
                   desc == empty_type_descriptor<R, Args...>();
        }

        inplace_buffer buf;
        type_descriptor<R, Args...> const *desc;
    };
}