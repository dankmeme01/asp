#pragma once

#include "../config.hpp"

#include <string>
#include <optional>
#include <functional>

namespace asp::util {
    void throwResultError(const std::string& message);

    struct EmptyValue {};

template <typename T = EmptyValue, typename E = std::string>
class Result {
    static constexpr unsigned char HAS_SUCCESS = 0b00000001;
    static constexpr unsigned char MOVED_FROM = 0b00000010;

    static constexpr bool IsUnit = std::is_same_v<T, EmptyValue>;
    using UnwrapRet = std::conditional<IsUnit, void, T&&>;

public:
    static constexpr bool SameTypes = std::is_same_v<T, E>;
    using Success = T;
    using Error = E;

    Result() requires (!IsUnit) = delete;
    Result() requires (IsUnit) : Result(EmptyValue {}) {}

    ~Result() {
        // run destructors even if moved !!
        if (this->isOk()) {
            _value.~T();
        } else {
            _error.~E();
        }
    }

    static Result ok(T&& val) requires (!SameTypes) {
        return Result(std::move(val));
    }

    static Result ok(const T& val) requires (!SameTypes) {
        return Result(val);
    }

    static Result err(E&& val) requires (!SameTypes) {
        return Result(std::move(val));
    }

    static Result err(const E& val) requires (!SameTypes) {
        return Result(val);
    }

    static Result ok() requires IsUnit {
        return Result();
    }

    // same types

    static Result ok(T&& val) requires SameTypes {
        return Result(std::move(val), true);
    }

    static Result ok(const T& val) requires SameTypes {
        return Result(val, true);
    }

    static Result err(T&& val) requires SameTypes {
        return Result(std::move(val), false);
    }

    static Result err(const T& val) requires SameTypes {
        return Result(val, false);
    }

    bool isOk() {
        return flags & HAS_SUCCESS;
    }

    bool isErr() {
        return !isOk();
    }

    // Throws an exception with the given message if the `Result` contains an error, otherwise returns the value.
    T&& expect(const std::string_view message) {
        this->markMoved();

        if (isErr()) {
            if constexpr (std::is_same_v<E, std::string>) {
                throwResultError(std::string(message) + ": " + _error);
            } else {
                throwResultError(std::string(message));
            }
        } else {
            return std::move(_value);
        }
    }

    // Returns the success value. Throws if the `Result` contains an error.
    T&& unwrap() {
        this->markMoved();

        ASP_ALWAYS_ASSERT(isOk(), "attempting to get the value from a failed Result");

        return std::move(_value);
    }

    // Returns the success value or the provided alternative, if an error is contained.
    T&& unwrapOr(T&& alternative) {
        this->markMoved();

        if (isErr()) return std::move(alternative);

        return std::move(_value);
    }

    // Returns the success value or the default initialization of it.
    T unwrapOrDefault() requires std::is_default_constructible_v<T> {
        this->markMoved();

        if (isOk()) {
            T ret = std::move(_value);
            return ret;
        }

        return T{};
    }
    // Returns the success value if the `Result` contains a value, otherwise returns the result of `func`.
    T&& unwrapOrElse(std::function<T(E&&)>&& func) {
        this->markMoved();

        if (isOk()) return std::move(_value);

        return std::move(func(std::move(_error)));
    }

    // Returns the success value. Invokes undefined behavior if the `Result` contains an error or has been moved from.
    T&& unwrapUnchecked() {
        return std::move(_value);
    }

    // Returns the error. Invokes undefined behavior if the `Result` contains a value or has been moved from.
    E&& unwrapErrUnchecked() {
        return std::move(_error);
    }

    // Returns the error. Throws if the `Result` contains a value.
    E&& unwrapErr() {
        this->markMoved();

        ASP_ALWAYS_ASSERT(isErr(), "attempting to get an error from a successful Result");

        return std::move(_error);
    }

    // Returns a reference to the value.
    T& valueRef() {
        this->markMoved();

        ASP_ALWAYS_ASSERT(isOk(), "attempting to get the value from a failed Result");

        return value;
    }

    // Returns a reference to the error.
    E& errorRef() {
        this->checkMoved();

        ASP_ALWAYS_ASSERT(isErr(), "attempting to get an error from a successful Result");

        return error;
    }

    // Returns `std::optional(value)` if the result contains value, otherwise `std::nullopt`
    std::optional<T> value() {
        this->markMoved();

        if (isErr()) return std::nullopt;

        auto ret = std::optional<T>(std::move(_value));

        return ret;
    }

    // Returns `std::optional(error)` if the result contains error, otherwise `std::nullopt`
    std::optional<E> error() {
        this->markMoved();

        if (isOk()) return std::nullopt;

        auto ret = std::optional<E>(std::move(_error));

        return ret;
    }

    // Maps `Result<T, E>` into `Result<R, E>`
    template <typename R>
    Result<R, E> map(std::function<R(T&&)>&& func) {
        this->markMoved();

        if (isErr()) return Result<R, E>::err(std::move(_error));

        return Result<R, E>::ok(func(std::move(_value)));
    }

    // Maps `Result<T, E>` into `Result<T, E2>`
    template <typename E2>
    Result<T, E2> mapErr(std::function<E2(E&&)>&& func) {
        this->markMoved();

        if (isOk()) return Result<T, E2>::ok(std::move(_value));

        return Result<T, E2>::err(func(std::move(_error)));
    }

    // Maps `Result<Result<T2, E>, E>` into `Result<T2, E>`
    template <typename T2>
    Result<T2, E> flatten() requires std::is_same_v<T, Result<T2, E>> {
        this->markMoved();

        if (isErr()) return Result<T2, E>::err(std::move(_error));
        if (_value.isErr()) return Result<T2, E>::err(std::move(_value.unwrapErr()));

        return Result<T2, E>::ok(std::move(_value.unwrap()));
    }

private:
    union {
        T _value;
        E _error;
    };

    unsigned char flags;

    template <typename other>
    Result(other&& v) requires (!SameTypes && std::is_convertible_v<other, T>) : _value(std::move(v)), flags(HAS_SUCCESS) {}
    template <typename other>
    Result(other&& e) requires (!SameTypes && std::is_convertible_v<other, E>) : _error(std::move(e)), flags(0) {}

    template <typename other>
    Result(const other& v) requires (!SameTypes && std::is_convertible_v<other, T>) : _value(v), flags(HAS_SUCCESS) {}
    template <typename other>
    Result(const other& e) requires (!SameTypes && std::is_convertible_v<other, E>) : _error(e), flags(0) {}

    // If `T` and `E` name the same type, we have a separate ctor.
    Result(T&& v, bool success) requires SameTypes : _value(std::move(v)), flags(success ? HAS_SUCCESS : 0) {}
    Result(const T& v, bool success) requires SameTypes : _value(v), flags(success ? HAS_SUCCESS : 0) {}

    inline void checkMoved() {
        ASP_ALWAYS_ASSERT(!(flags & MOVED_FROM), "attempting to unwrap a Result twice");
    }

    inline void markMoved() {
        this->checkMoved();
        flags |= MOVED_FROM;
    }
};

}