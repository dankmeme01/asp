#pragma once

// macro allowing for syntax like $into_async(socket.send(data)).await();
#define $into_async(expr) ::asp::async::spawn([&] { return expr; })

// same as `$into_async` but captures variables by copy instead of reference
#define $into_async_copy(expr) ::asp::async::spawn([=] { return expr; })

// macro allowing for syntax like this:
//
// ```
// auto asyncFunc(int x) $async ({
//     // your code
//     return 3;
// })
// ```
//
// the invokation of `asyncFunc` will not run the code inside immediately, and instead return a `FutureHandle`.
#define $async(block) { return ::asp::async::spawn([] { block }); }
