/*
* Includes all asp headers and brings classes from sub-namespaces into `asp`.
*/

#pragma once
#include "asp/sync.hpp"
#include "asp/thread.hpp"
#include "asp/Log.hpp"

namespace asp {
    using namespace ::asp::sync;
    using namespace ::asp::thread;
}
