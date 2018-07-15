/*******************************************************************************
    Copyright (c) 2013 NVIDIA Corporation

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to
    deal in the Software without restriction, including without limitation the
    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

        The above copyright notice and this permission notice shall be
        included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.

*******************************************************************************/

/*
 * This file contains common header code that can be used for all variants of
 * the UVM kernel module.
 */

#ifndef _UVM_COMMON_H
#define _UVM_COMMON_H

#ifdef DEBUG
    #define UVM_IS_DEBUG() 1
#else
    #define UVM_IS_DEBUG() 0
#endif

// NVIDIA_UVM_DEVELOP implies DEBUG, but not vice-versa
// TODO Bug 1773100: Figure out the right distinction between develop and debug
// builds.
#ifdef NVIDIA_UVM_DEVELOP
    #define UVM_IS_DEVELOP() 1
#else
    #define UVM_IS_DEVELOP() 0
#endif

#include "uvmtypes.h"
#include "uvm_linux.h"
#include "uvm_utils.h"
#include "uvm_minimal_init.h"

#define UVM_PRINT_FUNC_PREFIX(func, prefix, fmt, ...) \
    func(prefix "%s:%u %s[pid:%d]" fmt,               \
         kbasename(__FILE__),                         \
         __LINE__,                                    \
         __FUNCTION__,                                \
         current->pid,                                \
         ##__VA_ARGS__)

#define UVM_PRINT_FUNC(func, fmt, ...)  \
    UVM_PRINT_FUNC_PREFIX(func, "", fmt, ##__VA_ARGS__)

// Check whether UVM_{ERR,DBG,INFO)_PRINT* should be enabled
bool uvm_debug_prints_enabled(void);

// A printing helper like UVM_PRINT_FUNC_PREFIX that only prints if
// uvm_debug_prints_enabled() returns true.
#define UVM_PRINT_FUNC_PREFIX_CHECK(func, prefix, fmt, ...)             \
    do {                                                                \
        if (uvm_debug_prints_enabled()) {                               \
            UVM_PRINT_FUNC_PREFIX(func, prefix, fmt, ##__VA_ARGS__);    \
        }                                                               \
    } while (0)

#define UVM_ERR_PRINT(fmt, ...) \
    UVM_PRINT_FUNC_PREFIX_CHECK(printk, KERN_ERR NVIDIA_UVM_PRETTY_PRINTING_PREFIX, " " fmt, ##__VA_ARGS__)

#define UVM_ERR_PRINT_RL(fmt, ...) \
    UVM_PRINT_FUNC_PREFIX_CHECK(printk_ratelimited, KERN_ERR NVIDIA_UVM_PRETTY_PRINTING_PREFIX, " " fmt, ##__VA_ARGS__)

#define UVM_DBG_PRINT(fmt, ...) \
    UVM_PRINT_FUNC_PREFIX_CHECK(printk, KERN_DEBUG NVIDIA_UVM_PRETTY_PRINTING_PREFIX, " " fmt, ##__VA_ARGS__)

#define UVM_DBG_PRINT_RL(fmt, ...)                              \
    UVM_PRINT_FUNC_PREFIX_CHECK(printk_ratelimited, KERN_DEBUG NVIDIA_UVM_PRETTY_PRINTING_PREFIX, " " fmt, ##__VA_ARGS__)

#define UVM_INFO_PRINT(fmt, ...) \
    UVM_PRINT_FUNC_PREFIX_CHECK(printk, KERN_INFO NVIDIA_UVM_PRETTY_PRINTING_PREFIX, " " fmt, ##__VA_ARGS__)

//
// Please see the documentation of format_uuid_to_buffer, for details on what
// this routine prints for you.
//
#define UVM_DBG_PRINT_UUID(msg, uuidPtr)                                \
    do {                                                                \
        char uuidBuffer[UVM_GPU_UUID_TEXT_BUFFER_LENGTH];               \
        format_uuid_to_buffer(uuidBuffer, sizeof(uuidBuffer), uuidPtr); \
        UVM_DBG_PRINT("%s: %s\n", msg, uuidBuffer);                     \
    } while(0)

#define UVM_ERR_PRINT_NV_STATUS(msg, rmStatus, ...)                        \
    UVM_ERR_PRINT("ERROR: %s : " msg "\n", nvstatusToString(rmStatus), ##__VA_ARGS__)

#define UVM_ERR_PRINT_UUID(msg, uuidPtr, ...)                              \
    do {                                                                   \
        char uuidBuffer[UVM_GPU_UUID_TEXT_BUFFER_LENGTH];                  \
        format_uuid_to_buffer(uuidBuffer, sizeof(uuidBuffer), uuidPtr);    \
        UVM_ERR_PRINT("ERROR: %s : " msg "\n", uuidBuffer, ##__VA_ARGS__); \
    } while(0)

#define UVM_PANIC()             UVM_PRINT_FUNC(panic, "\n")
#define UVM_PANIC_MSG(fmt, ...) UVM_PRINT_FUNC(panic, ": " fmt, ##__VA_ARGS__)

#define UVM_PANIC_ON_MSG(cond, fmt, ...)        \
    do {                                        \
        if (unlikely(cond))                     \
            UVM_PANIC_MSG(fmt, ##__VA_ARGS__);  \
    } while(0)

#define UVM_PANIC_ON(cond)  UVM_PANIC_ON_MSG(cond, "failed cond %s\n", #cond)

// expr may include function calls. Use sizeof to prevent it from being
// evaluated while also preventing unused variable warnings. sizeof() can't be
// used on a bitfield however, so use ! to force the expression to evaluate as
// an int.
#define UVM_IGNORE_EXPR(expr) ((void)sizeof(!(expr)))

#define UVM_IGNORE_EXPR2(expr1, expr2)  \
    do {                                \
        UVM_IGNORE_EXPR(expr1);         \
        UVM_IGNORE_EXPR(expr2);         \
    } while (0)

// NO-OP function to break on_uvm_assert - that is just to set a breakpoint
void on_uvm_assert(void);
#if UVM_IS_DEBUG()
     #define _UVM_ASSERT_MSG(expr, cond, fmt, ...)                                                  \
        do {                                                                                        \
            if (unlikely(!(expr))) {                                                                \
                UVM_ERR_PRINT("Assert failed, condition %s not true" fmt, cond, ##__VA_ARGS__);     \
                dump_stack();                                                                       \
                on_uvm_assert();                                                                    \
            }                                                                                       \
        } while (0)
#else
    // Prevent function calls in expr and the print argument list from being
    // evaluated.
    #define _UVM_ASSERT_MSG(expr, cond, fmt, ...)   \
        do {                                        \
            UVM_IGNORE_EXPR(expr);                  \
            UVM_NO_PRINT(fmt, ##__VA_ARGS__);       \
        } while (0)
#endif

#define UVM_ASSERT_MSG(expr, fmt, ...) _UVM_ASSERT_MSG(expr, #expr, ": " fmt, ##__VA_ARGS__)
#define UVM_ASSERT(expr) _UVM_ASSERT_MSG(expr, #expr, "\n")

// Provide a short form of UUID's, typically for use in debug printing:
#define ABBREV_UUID(uuid) (unsigned)(uuid)

static inline NvBool uvm_uuid_is_cpu(const NvProcessorUuid *uuid)
{
    return memcmp(uuid, &NV_PROCESSOR_UUID_CPU_DEFAULT, sizeof(*uuid)) == 0;
}

#define UVM_ALIGN_DOWN(x, a) ({         \
        typeof(x) _a = a;               \
        UVM_ASSERT(is_power_of_2(_a));  \
        (x) & ~(_a - 1);                \
    })

#define UVM_ALIGN_UP(x, a) ({           \
        typeof(x) _a = a;               \
        UVM_ASSERT(is_power_of_2(_a));  \
        ((x) + _a - 1) & ~(_a - 1);     \
    })

#define UVM_PAGE_ALIGN_UP(value) UVM_ALIGN_UP(value, PAGE_SIZE)
#define UVM_PAGE_ALIGN_DOWN(value) UVM_ALIGN_DOWN(value, PAGE_SIZE)

// These macros provide a convenient way to string-ify enum values.
#define UVM_ENUM_STRING_CASE(value) case value: return #value
#define UVM_ENUM_STRING_DEFAULT() default: return "UNKNOWN"

// Divide by a dynamic value known at runtime to be a power of 2. ilog2 is
// optimized as a single instruction in many processors, whereas integer
// division is always slow.
static inline NvU32 uvm_div_pow2_32(NvU32 numerator, NvU32 denominator_pow2)
{
    UVM_ASSERT(is_power_of_2(denominator_pow2));
    UVM_ASSERT(denominator_pow2);
    return numerator >> ilog2(denominator_pow2);
}

static inline NvU64 uvm_div_pow2_64(NvU64 numerator, NvU64 denominator_pow2)
{
    UVM_ASSERT(is_power_of_2(denominator_pow2));
    UVM_ASSERT(denominator_pow2);
    return numerator >> ilog2(denominator_pow2);
}

#define SUM_FROM_0_TO_N(n) (((n) * ((n) + 1)) / 2)

// Start and end are inclusive
static inline NvBool uvm_ranges_overlap(NvU64 a_start, NvU64 a_end, NvU64 b_start, NvU64 b_end)
{
    // De Morgan's of: !(a_end < b_start || b_end < a_start)
    return a_end >= b_start && b_end >= a_start;
}

static int debug_mode(void)
{
#ifdef DEBUG
    return 1;
#else
    return 0;
#endif
}

static inline void kmem_cache_destroy_safe(struct kmem_cache **ppCache)
{
    if (ppCache)
    {
        if (*ppCache)
            kmem_cache_destroy(*ppCache);
        *ppCache = NULL;
    }
}

static const uid_t UVM_ROOT_UID = 0;


typedef struct
{
    NvU64 start_time_ns;
    NvU64 print_time_ns;
} uvm_spin_loop_t;

static inline void uvm_spin_loop_init(uvm_spin_loop_t *spin)
{
    NvU64 curr = NV_GETTIME();
    spin->start_time_ns = curr;
    spin->print_time_ns = curr;
}

// Periodically yields the CPU when not called from interrupt context. Returns
// NV_ERR_TIMEOUT_RETRY if the caller should print a warning that we've been
// waiting too long, and NV_OK otherwise.
NV_STATUS uvm_spin_loop(uvm_spin_loop_t *spin);

#define UVM_SPIN_LOOP(__spin) ({                                                        \
    NV_STATUS __status = uvm_spin_loop(__spin);                                         \
    if (__status == NV_ERR_TIMEOUT_RETRY) {                                             \
        UVM_DBG_PRINT("Warning: stuck waiting for %llus\n",                             \
                (NV_GETTIME() - (__spin)->start_time_ns) / (1000*1000*1000));           \
                                                                                        \
        if (uvm_debug_prints_enabled())                                                 \
            dump_stack();                                                               \
    }                                                                                   \
    __status;                                                                           \
})

// Execute the loop code while cond is true. Invokes uvm_spin_loop_iter at the
// end of each iteration.
#define UVM_SPIN_WHILE(cond, spin)                                                \
    if (cond)                                                                     \
        for (uvm_spin_loop_init(spin); (cond); UVM_SPIN_LOOP(spin))

//
// Documentation for the internal routines listed below may be found in the
// implementation file(s).
//
NV_STATUS errno_to_nv_status(int errnoCode);
int nv_status_to_errno(NV_STATUS status);
unsigned uvm_get_stale_process_id(void);
unsigned uvm_get_stale_thread_id(void);
NvBool uvm_user_id_security_check(uid_t euidTarget);
NV_STATUS uvm_api_stub(void *pParams, struct file *filp);
NV_STATUS uvm_api_unsupported(void *pParams, struct file *filp);

extern int uvm_enable_builtin_tests;

static inline void uvm_init_character_device(struct cdev *cdev, const struct file_operations *fops)
{
    cdev_init(cdev, fops);
    cdev->owner = THIS_MODULE;
}

typedef struct
{
    int rm_control_fd;
    NvHandle user_client;
    NvHandle user_object;
} uvm_rm_user_object_t;

// ARM/x86 architectures require addresses to be in "canonical form". Sign-extend 49-bit virtual
// address for all architectures as per Bug 1568165
static inline NvU64 uvm_address_get_canonical_form(NvU64 address)
{
#if NVCPU_IS_X86_64 || NVCPU_IS_AARCH64
    address = (NvU64)(((NvS64) address << (64 - 49)) >> (64 - 49));
#endif
    return address;
}

// Macro used to compare two values for types that support less than operator.
// It returns -1 if a < b, 1 if a > b and 0 if a == 0
#define UVM_CMP_DEFAULT(a,b)              \
({                                        \
    typeof(a) _a = a;                     \
    typeof(b) _b = b;                     \
    int __ret;                            \
    BUILD_BUG_ON(sizeof(a) != sizeof(b)); \
    if (_a < _b)                          \
        __ret = -1;                       \
    else if (_b < _a)                     \
        __ret = 1;                        \
    else                                  \
        __ret = 0;                        \
                                          \
    __ret;                                \
})


    // TODO: Bug 2034846: Use common speculation_barrier implementation
    static void speculation_barrier(void)
    {
    #if NVCPU_IS_X86 || NVCPU_IS_X86_64
        __asm__ __volatile__ ("lfence" : : : "memory");
    #endif
    }



#endif /* _UVM_COMMON_H */
