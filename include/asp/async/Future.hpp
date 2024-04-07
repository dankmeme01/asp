#pragma once

#include "Macros.hpp"
#include "../config.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <condition_variable>

namespace asp::async {

namespace detail {
    void futureFail(const std::exception& e);
}

// Exception that is thrown when joining a future that ends up throwing.
class FutureFailed : public std::runtime_error {
public:
    FutureFailed(const std::exception& e) : inner(e), runtime_error(e.what()) {}

private:
    std::exception inner;
};

template <typename Out = void>
class Future {
public:
    using Task = std::function<Out()>;

    Future(Task&& func) : task(std::move(func)) {}
    Future(const Task& func) : task(func) {}

    Future(const Future&) = delete;
    Future& operator=(const Future&) = delete;

    ~Future() {
        if (finished) {
            _res().~Out();
        } else if (failed) {
            _err().~exception();
        }
    }

    // Starts the future. Blocks the calling thread.
    void start() {
        {
            std::unique_lock lock(mtx);

            ASP_ASSERT(!running, "cannot start the same Future twice");
            ASP_ASSERT(!finished && !failed, "cannot restart a Future that has already finished running");
            ASP_ALWAYS_ASSERT(task, "cannot start an empty Future");

            running = true;
        }

        // this line of code will haunt you in your dreams
        // *new (&resultBuf) Out = std::move(task());

        try {
            new (&_ru.resultBuf) Out(std::move(task()));
        } catch (const std::exception& e) {
            std::unique_lock lock(mtx);
            running = false;

            new (&_ru.errorBuf) std::exception(e);

            failed = true;

            if (errorHandler) {
                errorHandler(e);
            } else {
                detail::futureFail(e);
            }

            cvar.notify_all();

            return;
        }

        std::unique_lock lock(mtx);
        running = false;
        finished = true;

        if (callback) callback(_res());

        cvar.notify_all();
    }

    bool isRunning() const {
        std::unique_lock lock(mtx);
        return running;
    }

    bool hasFinished() const {
        std::unique_lock lock(mtx);
        return finished || failed;
    }

    bool hasResult() const {
        std::unique_lock lock(mtx);
        return finished;
    }

    bool hasError() const {
        std::unique_lock lock(mtx);
        return failed;
    }

    Out& getResult() const {
        std::unique_lock lock(mtx);

        ASP_ASSERT(!failed, "cannot get a result of a Future that has failed");
        ASP_ALWAYS_ASSERT(finished, "cannot get a result of a Future that hasn't yet been finished");

        return _res();
    }

    const std::exception& getError() const {
        std::unique_lock lock(mtx);

        ASP_ALWAYS_ASSERT(failed, "cannot get an error of a Future that has not failed");

        return _err();
    }

    Out& await() const {
        std::unique_lock lock(mtx);

        if (finished) {
            return _res();
        } else if (failed) {
            throw FutureFailed(_err());
        }

        cvar.wait(lock, [this] { return finished || failed; });

        if (failed) {
            throw FutureFailed(_err());
        }

        return _res();
    }

    void join() const {
        std::unique_lock lock(mtx);

        if (finished || failed) {
            return;
        }

        cvar.wait(lock, [this] { return finished || failed; });
    }

    void then(std::function<void(const Out&)>&& f) const {
        std::unique_lock lock(mtx);

        if (finished) {
            f(_res());
        } else if (failed) {
            return;
        }

        callback = std::move(f);
    }

    void expect(std::function<void(const std::exception&)>&& f) const {
        std::unique_lock lock(mtx);

        if (finished) {
            return;
        } else if (failed) {
            f(_err());
        }

        errorHandler = std::move(f);
    }

private:
    Task task;
    mutable std::mutex mtx;
    mutable std::condition_variable cvar;
    mutable std::function<void(const Out&)> callback;
    mutable std::function<void(const std::exception&)> errorHandler;
    bool running = false, finished = false, failed = false;

    mutable union {
        alignas(Out) std::byte resultBuf[sizeof(Out)];
        alignas(std::exception) std::byte errorBuf[sizeof(std::exception)];
    } _ru;

    Out& _res() const {
        ASP_ASSERT(finished, "Future::_res called on a future that hasn't finished successfully");

        Out* ptr = std::launder(reinterpret_cast<Out*>(&_ru.resultBuf));
        return *ptr;
    }

    const std::exception& _err() const {
        ASP_ASSERT(failed, "Future::_err called on a future that hasn't failed");

        std::exception* ptr = std::launder(reinterpret_cast<std::exception*>(&_ru.errorBuf));

        return *ptr;
    }
};

template <typename FOut = void>
class FutureHandle {
public:
    FutureHandle(std::shared_ptr<Future<FOut>> fut) : fut(std::move(fut)) {}

    // Returns whether the future is currently running.
    bool isRunning() const {
        return fut->isRunning();
    }

    // Returns whether the future is no longer running, aka it either completed successfully or failed.
    bool hasFinished() const {
        return fut->hasFinished();
    }

    // Returns whether the future has completed successfully and a result is available.
    bool hasResult() const {
        return fut->hasResult();
    }

    // Returns whether the future has failed and an error is available.
    bool hasError() const {
        return fut->hasError();
    }

    // Gets the result of the future, if it has finished. If it has not, throws an exception.
    FOut& getResult() const {
        return fut->getResult();
    }

    // Gets the error that occurred while running the future. If `hasError() == false`, throws an exception.
    const std::exception& getError() const {
        return fut->getError();
    }

    // Blocks until the future is complete and then returns a reference to the return value.
    FOut& await() const {
        return fut->await();
    }

    // Like `await()`, but does not return the result of the future, and does not throw if the future failed.
    void join() const {
        return fut->join();
    }

    // Schedule a callback to be called when the future completes execution.
    // The callback will always be ran strictly *before* notifying any awaiters.
    // If the future has already successfully finished execution, the callback is invoked immediately.
    FutureHandle& then(std::function<void(const FOut&)>&& f) {
        fut->then(std::move(f));
        return *this;
    }

    // Sets the function that will be called if the future throws an exception.
    // When not set, by default the exception message will simply be logged.
    // If the future has already finished execution and ended up throwing an exception, the callback is invoked immediately.
    FutureHandle& expect(std::function<void(const std::exception&)>&& f) {
        fut->expect(std::move(f));
        return *this;
    }

private:
    std::shared_ptr<Future<FOut>> fut;
};

/* Specialization for void futures */

template <>
class Future<void> {
public:
    using Task = std::function<void()>;

    Future(Task&& func) : task(std::move(func)) {}
    Future(const Task& func) : task(func) {}

    Future(const Future&) = delete;
    Future& operator=(const Future&) = delete;

    void start() {
        {
            std::unique_lock lock(mtx);

            ASP_ASSERT(!running, "cannot start the same Future twice");
            ASP_ASSERT(!finished, "cannot restart a Future that was already finished");
            ASP_ALWAYS_ASSERT(task, "cannot start an empty Future");

            running = true;
        }
        try {
            task();
        } catch (const std::exception& e) {
            std::unique_lock lock(mtx);
            running = false;
            error = e;
            failed = true;

            if (errorHandler) {
                errorHandler(e);
            } else {
                detail::futureFail(e);
            }

            cvar.notify_all();

            return;
        }

        std::unique_lock lock(mtx);
        running = false;
        finished = true;

        if (callback) callback();

        cvar.notify_all();
    }

    bool isRunning() const {
        std::unique_lock lock(mtx);
        return running;
    }

    bool hasFinished() const {
        std::unique_lock lock(mtx);
        return finished || failed;
    }

    bool hasResult() const {
        std::unique_lock lock(mtx);
        return finished;
    }

    bool hasError() const {
        std::unique_lock lock(mtx);
        return failed;
    }

    const std::exception& getError() const {
        std::unique_lock lock(mtx);

        ASP_ALWAYS_ASSERT(failed, "cannot get an error of a Future that has not failed");

        return _err();
    }

    void await() const {
        std::unique_lock lock(mtx);

        if (finished) {
            return;
        } else if (failed) {
            throw FutureFailed(_err());
        }

        cvar.wait(lock, [this] { return finished || failed; });

        if (failed) {
            throw FutureFailed(_err());
        }
    }

    void join() const {
        std::unique_lock lock(mtx);

        if (finished || failed) {
            return;
        }

        cvar.wait(lock, [this] { return finished || failed; });
    }

    void then(std::function<void()>&& f) const {
        std::unique_lock lock(mtx);

        if (finished) {
            f();
        } else if (failed) {
            return;
        }

        callback = std::move(f);
    }

    void expect(std::function<void(const std::exception&)>&& f) const {
        std::unique_lock lock(mtx);

        if (finished) {
            return;
        } else if (failed) {
            f(_err());
        }

        errorHandler = std::move(f);
    }

private:
    Task task;
    mutable std::mutex mtx;
    mutable std::condition_variable cvar;
    mutable std::function<void()> callback;
    mutable std::function<void(const std::exception&)> errorHandler;
    bool running = false, finished = false, failed = false;
    std::exception error;

    const std::exception& _err() const {
        ASP_ASSERT(failed, "Future::_err called on a future that hasn't failed");

        return error;
    }
};

template <>
class FutureHandle<void> {
public:
    FutureHandle(std::shared_ptr<Future<void>> fut) : fut(std::move(fut)) {}

    bool isRunning() const {
        return fut->isRunning();
    }

    bool hasFinished() const {
        return fut->hasFinished();
    }

    bool hasResult() const {
        return fut->hasResult();
    }

    bool hasError() const {
        return fut->hasError();
    }

    const std::exception& getError() const {
        return fut->getError();
    }

    void await() const {
        return fut->await();
    }

    void join() const {
        return fut->join();
    }

    FutureHandle& then(std::function<void()>&& f) {
        fut->then(std::move(f));
        return *this;
    }

    FutureHandle& expect(std::function<void(const std::exception&)>&& f) {
        fut->expect(std::move(f));
        return *this;
    }

private:
    std::shared_ptr<Future<void>> fut;
};

}