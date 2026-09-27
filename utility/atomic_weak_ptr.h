// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#if defined(__cpp_lib_atomic_shared_ptr) && (__cpp_lib_atomic_shared_ptr >= 202011L)
#include "utility/atomic_weak_ptr/std.h"
#elif defined(__APPLE__)
#include "utility/atomic_weak_ptr/darwin.h"
#else
#include "utility/atomic_weak_ptr/mutex.h"
#endif
