/*
* Includes all asp headers and brings classes from sub-namespaces into `asp`.
*/

#pragma once
#include "asp/sync/Atomic.hpp"
#include "asp/sync/Mutex.hpp"
#include "asp/sync/Channel.hpp"
#include "asp/thread/Thread.hpp"
#include "asp/thread/ThreadPool.hpp"
#include "asp/Log.hpp"

namespace asp {
    using namespace ::asp::sync;
    using namespace ::asp::thread;
}
