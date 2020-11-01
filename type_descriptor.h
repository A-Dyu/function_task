#pragma once

struct bad_function_call : std::exception {
    char const *what() const noexcept override {
        return "bad function call";
    }
};

namespace function_structs {
    template<typename R, typename... Args>
    struct storage;

    constexpr static size_t INPLACE_BUFFER_SIZE = sizeof(void *);
    constexpr static size_t INPLACE_BUFFER_ALIGNMENT = alignof(void *);

    using inplace_buffer = std::aligned_storage<
            INPLACE_BUFFER_SIZE,
            INPLACE_BUFFER_ALIGNMENT>::type;

    template<typename T>
    constexpr static bool fits_small_storage =
            sizeof(T) <= INPLACE_BUFFER_SIZE &&
            INPLACE_BUFFER_ALIGNMENT % alignof(T) == 0 &&
            std::is_nothrow_move_constructible<T>::value;

    template<typename R, typename... Args>
    struct type_descriptor {
        using storage_t = storage<R, Args...>;

        void (*copy)(storage_t const *src, storage_t *dest);

        void (*move)(storage_t *src, storage_t *dest);

        R (*invoke)(storage_t *src, Args...);

        void (*destroy)(storage_t *);
    };

    template<typename R, typename... Args>
    type_descriptor<R, Args...> const *empty_type_descriptor() {
        using storage_t = storage<R, Args...>;

        constexpr static type_descriptor<R, Args...> impl = {
                [](storage_t const *src, storage_t *dest) {
                    dest->desc = src->desc;
                },
                [](storage_t *src, storage_t *dest) {
                    dest->desc = src->desc;
                },
                [](storage_t *src, Args... args) -> R {
                    throw bad_function_call();
                },
                [](storage_t *) {}
        };
        return &impl;
    }

    template<typename T, typename = void>
    struct functional_traits;

    template<typename T>
    struct functional_traits<T, std::enable_if_t<fits_small_storage<T>>> {
        template<typename R, typename... Args>
        static type_descriptor<R, Args...> const *get_type_descriptor() noexcept {
            using storage_t = storage<R, Args...>;
            constexpr static type_descriptor<R, Args...> impl = {
                    // Copy
                    [](storage_t const *src, storage_t *dest) {
                        new(&dest->buf) T(src->template get_static<T>());
                        dest->desc = src->desc;
                    },
                    // move
                    [](storage_t *src, storage_t *dest) {
                        new(&dest->buf) T(std::move(src->template get_static<T>()));
                        dest->desc = src->desc;
                    },
                    // invoke
                    [](storage_t *src, Args... args) -> R {
                        return src->template get_static<T>()(std::forward<Args>(args)...);
                    },
                    // destroy
                    [](storage_t *src) {
                        src->template get_static<T>().~T();
                        src->desc = empty_type_descriptor<R, Args...>();
                    }
            };
            return &impl;
        }
    };

    template<typename T>
    struct functional_traits<T, std::enable_if_t<!fits_small_storage<T>>> {
        template<typename R, typename... Args>
        static type_descriptor<R, Args...> const *get_type_descriptor() noexcept {
            using storage_t = storage<R, Args...>;

            constexpr static type_descriptor<R, Args...> impl = {
                    // Copy
                    [](storage_t const *src, storage_t *dest) {
                        new(&dest->buf) T *(new T(*src->template get_dynamic<T>()));
                        dest->desc = src->desc;
                    },
                    // move
                    [](storage_t *src, storage_t *dest) {
                        new(&dest->buf) T *(src->template get_static<T *>());
                        src->set_dynamic(nullptr);
                        dest->desc = src->desc;
                    },
                    // invoke
                    [](storage_t *src, Args... args) -> R {
                        return (*src->template get_dynamic<T>())(std::forward<Args>(args)...);
                    },
                    // destroy
                    [](storage_t *src) {
                        delete src->template get_dynamic<T>();
                        src->set_dynamic(nullptr);
                    }
            };
            return &impl;
        }
    };
}