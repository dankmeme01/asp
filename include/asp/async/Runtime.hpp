#pragma once

#include "../config.hpp"
#include "Future.hpp"

#include <memory>
#include <type_traits>

namespace asp::async {

class RuntimeImpl;

struct RuntimeSettings {
    // 0 - auto, 1 - single threaded
    size_t threadCount;
};

class Runtime {
public:
    static Runtime& get();

    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;
    Runtime(Runtime&&) = delete;
    Runtime& operator=(Runtime&&) = delete;

    // Set the settings of the async runtime. Throws an error if the runtime has already been launched.
    void configure(const RuntimeSettings& settings);

    // Asynchronously launches the runtime, without blocking the calling thread.
    void launch();

    // Spawns a future in a different thread, returns a handle that allows you to see the progress of the execution.
    template <typename F, typename FOut = typename std::invoke_result_t<F>>
    FutureHandle<FOut> spawn(F&& func) {
        auto fut = std::make_shared<Future<FOut>>(std::move(func));
        this->runAsync([fut] {
            fut->start();
        });
        return FutureHandle<FOut>(std::move(fut));
    }

    // Spawns a future in a different thread, returns a handle that allows you to see the progress of the execution.
    template <typename F, typename FOut = typename std::invoke_result_t<F>>
    FutureHandle<FOut> spawn(const F& func) {
        auto fut = std::make_shared<Future<FOut>>(func);
        this->runAsync([fut] {
            fut->start();
        });
        return FutureHandle<FOut>(std::move(fut));
    }

private:
    std::unique_ptr<RuntimeImpl> impl;

    Runtime();

    void runAsync(std::function<void()>&& f);
};


// Equivalent to `Runtime::get().spawn(func)`
template <typename F, typename FOut = typename std::invoke_result_t<F>>
FutureHandle<FOut> spawn(F&& func) {
    return Runtime::get().spawn<F, FOut>(std::move(func));
}

// Equivalent to `Runtime::get().spawn(func)`
template <typename F, typename FOut = typename std::invoke_result_t<F>>
FutureHandle<FOut> spawn(const F& func) {
    return Runtime::get().spawn<F, FOut>(func);
}

template<typename... Futures>
std::tuple<> _await_helper(const Futures&...) {
    return std::make_tuple();
}

template<typename Future, typename... Futures>
auto _await_helper(const Future& future, const Futures&... futures) {
    // join the current future and recursively join the rest
    auto current_result = std::make_tuple(future.await());
    auto remaining_results = _await_helper(futures...);

    return std::tuple_cat(current_result, remaining_results);
}

// Joins all the given futures and returns their return values.
template<typename... Futures>
auto await(const Futures&... futures) {
    return _await_helper(futures...);
}

// Like `await`, but can be used for void futures.
template <typename... Futures>
void await_void(const Futures&... futures) {
    (futures.await(), ...);
}

// Like `await_void`, but will not throw if any of the futures failed.
template <typename... Futures>
void join(const Futures&... futures) {
    (futures.join(), ...);
}

}