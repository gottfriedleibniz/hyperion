/* Hercules atomic operations.                                       */
/*                                 John Hartmann 5 Oct 2015 11:32:17 */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*********************************************************************/
/* This  file  was  put into the public domain 2015-10-05 by John P. */
/* Hartmann.   You  can use it for anything you like as long as this */
/* notice remains.                                                   */
/*********************************************************************/

/*********************************************************************/
/* This  code  was  written  originally  to  support the Interlocked */
/* Access  Facility  2  (NI  OI  XI  NIY  OIY XIY instructions).  It */
/* defines  atomic operations to atomically fetch and update a       */
/* storage  location,  or  (if  the  operation  cannot  be performed */
/* atomically)  to  perform the original logical operation.  For GCC */
/* and  CLANG  the  underlying intrinsic function is overloaded, but */
/* this is not the case for MSVC.                                    */
/*                                                                   */
/* Support   for   IAF2  can  be  disabled  by  the  configure  flag */
/* --disable-interlocked-update-facility-2                           */
/*                                                                   */
/* Note that we need an atomic fetch and update, not just the atomic */
/* update.   C11  defines  atomic  fetch  and  update,  that  is the */
/* function  result  is the value before the operation, but that can */
/* easily  be mapped into atomic update and fetch (apply the logical */
/* operation).                                                       */
/*                                                                   */
/* For  the  non-MSVC  case,  we  check  whether  the  C11  standard */
/* functions are available and use them if so.  If not, we check for */
/* the GCC intrinsic functions (which CLANG also implements) and use */
/* them  if  so.   If  not,  we  do not implement IAF2 (STFLE bit 52 */
/* remains off) and fall back on the original code.                  */
/*                                                                   */
/* The operations we export take the form:                           */
/*    atomic_{OP}_{TYPE}(PTR, IMM)                                   */
/*    OP       The actual logical operation to perform               */
/*    TYPE     The underling pointer type                            */
/*    ptr      Pointer to the [byte] to update                       */
/*    imm      The immediate field from the instruction (i2)         */
/*                                                                   */
/* Refer to general1.c instruction NI for an example.                */
/*                                                                   */
/* We  also  define the macro CAN_IAF2 when bit 52 of the facilities */
/* list should be 1 (any mode).                                      */
/*                                                                   */
/* Notes on observed compiler code generation (October 2015):        */
/* ==========================================================        */
/*                                                                   */
/* For  the  atomic  update  case  (where the function result is not */
/* used)  Intel  compilers generate a locked and operation without a */
/* loop.   When  the  result  is  needed,  all  compilers generate a */
/* compare and swap loop, for example:                               */
/*                                                                   */
/* .L2:                                                              */
/*      movl    %eax, %edx      # tmp66, tmp65                       */
/*      andl    $42, %edx       #, tmp65                             */
/*      lock cmpxchgb   %dl, 47(%esp)   #, tmp65,                    */
/*      jne     .L2     #,                                           */
/*                                                                   */
/* In my view this is not a lock free operation, but apparently the  */
/* compiler writers have decided otherwise.                          */
/*                                                                   */
/* Rejected alternatives:                                             */
/* =====================                                             */
/*                                                                   */
/* An  attempt  was  made to map the three non C11 cases into macros */
/* that  could  expand  the  C11  functions to the desired code, but */
/* while  it  is  trivial  to  map the C11 case to the GCC intrinsic */
/* <op>_fetch, a temporary variable would be needed to implement the */
/* non-IAF2  case,  which would have negated the overloading we take */
/* advantage of.                                                     */
/*********************************************************************/

#ifndef _JPH_HATOMIC_H
#define _JPH_HATOMIC_H

#if defined( _MSVC_ )
  #if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L \
                                && !defined( __STDC_NO_ATOMICS__ )
    #define C11_ATOMICS_AVAILABLE
    #define C11_ATOMIC_BOOL_LOCK_FREE      ATOMIC_BOOL_LOCK_FREE
    #define C11_ATOMIC_CHAR_LOCK_FREE      ATOMIC_CHAR_LOCK_FREE
    #define C11_ATOMIC_CHAR16_T_LOCK_FREE  ATOMIC_CHAR16_T_LOCK_FREE
    #define C11_ATOMIC_CHAR32_T_LOCK_FREE  ATOMIC_CHAR32_T_LOCK_FREE
    #define C11_ATOMIC_WCHAR_T_LOCK_FREE   ATOMIC_WCHAR_T_LOCK_FREE
    #define C11_ATOMIC_SHORT_LOCK_FREE     ATOMIC_SHORT_LOCK_FREE
    #define C11_ATOMIC_INT_LOCK_FREE       ATOMIC_INT_LOCK_FREE
    #define C11_ATOMIC_LONG_LOCK_FREE      ATOMIC_LONG_LOCK_FREE
    #define C11_ATOMIC_LLONG_LOCK_FREE     ATOMIC_LLONG_LOCK_FREE
    #define C11_ATOMIC_POINTER_LOCK_FREE   ATOMIC_POINTER_LOCK_FREE
  #else
    #undef  C11_ATOMICS_AVAILABLE
  #endif
#endif

/*-------------------------------------------------------------------*/
/* Hercules Atomic Configuration                                     */
/*-------------------------------------------------------------------*/

#if defined( C11_ATOMICS_AVAILABLE )
  #include <stdatomic.h>
#endif

#define NEVER_ATOMIC        0
#define SOMETIMES_ATOMIC    1
#define ALWAYS_ATOMIC       2

#define IAF2_ATOMICS_UNAVAILABLE    0
#define IAF2_C11_STANDARD_ATOMICS   1
#define IAF2_MICROSOFT_INTRINSICS   2
#define IAF2_ATOMIC_INTRINSICS      3
#define IAF2_SYNC_BUILTINS          4

#if defined( C11_ATOMICS_AVAILABLE ) && \
    (C11_ATOMIC_CHAR_LOCK_FREE == ALWAYS_ATOMIC || \
    (C11_ATOMIC_CHAR_LOCK_FREE == SOMETIMES_ATOMIC && defined( _MSVC_ )))
  #define HERC_ATOMICS    IAF2_C11_STANDARD_ATOMICS
#elif defined( _MSVC_ )
  #define HERC_ATOMICS    IAF2_MICROSOFT_INTRINSICS
#elif defined( HAVE_ATOMIC_INTRINSICS )
  #define HERC_ATOMICS    IAF2_ATOMIC_INTRINSICS
#elif defined( HAVE_SYNC_BUILTINS )
  #define HERC_ATOMICS    IAF2_SYNC_BUILTINS
#else
  #define HERC_ATOMICS    IAF2_ATOMICS_UNAVAILABLE

  WARNING( "Missing atomic atomic support!" )
#endif

/* Atomic operations supported */
#define HERC_ATOMICS_AVAILABLE   (HERC_ATOMICS != IAF2_ATOMICS_UNAVAILABLE)
#define HERC_ATOMICS_UNAVAILABLE (HERC_ATOMICS == IAF2_ATOMICS_UNAVAILABLE)

/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Interlocked Access Facility 2 Configuration                       */
/*-------------------------------------------------------------------*/

#if !defined( DISABLE_IAF2 )
  #define CAN_IAF2          HERC_ATOMICS
#else /* defined( DISABLE_IAF2 ) */
  #define CAN_IAF2          IAF2_ATOMICS_UNAVAILABLE
#endif

/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Atomic Operations                                                 */
/*-------------------------------------------------------------------*/

/*
 * Current approach uses statically typed inlined functions instead of macros
 * that do compile-time size routing similar to H_ATOMIC_OP (or if C11 were
 * available _Generic dispatching). Much easier to conceptualize as requirements
 * are being worked out.
 *
 * NOTE: For LOAD/STORE in case of MSVC it may be required to switch to
 * idempotent interlocked intrinsics, e.g.,
 *      LOAD = _InterlockedOr((volatile MSVC_TYPE*)ptr, 0);
 *     STORE = _InterlockedExchange((volatile MSVC_TYPE*)ptr, val);
 */

#if defined( _MSC_VER )
  #define HERC_ATOMIC_SPECIFIER __forceinline
#elif defined( __GNUC__ ) || defined( __clang__ )
  #define HERC_ATOMIC_SPECIFIER inline __attribute__((always_inline))
#elif defined( __STDC_VERSION__ ) && __STDC_VERSION__ >= 199901L
  #define HERC_ATOMIC_SPECIFIER inline
#else
  #define HERC_ATOMIC_SPECIFIER __inline
#endif

/* Helpers to ensure field and pointer alignment for atomic operations */
#define HERC_ATOMIC_IS_ALIGNED(ptr, boundary) (((uintptr_t)(ptr) % (boundary)) == 0)
#define HERC_ATOMIC_ASSUME_ALIGNED(ptr, type) do { } while (0)

#if HERC_ATOMICS == IAF2_C11_STANDARD_ATOMICS

#define DEFINE_ATOMIC_LOADSTORE(TYPE, SUFFIX, C11_LOAD_ORDER, C11_STORE_ORDER, GCC_LOAD_ORDER, GCC_STORE_ORDER, IS_RELAXED, IS_ACQUIRE, IS_RELEASE) \
HERC_ATOMIC_SPECIFIER                                                   \
TYPE atomic_load_##SUFFIX(const volatile TYPE *ptr) {                   \
    const volatile _Atomic(TYPE) *atomic_ptr =                          \
        (const volatile _Atomic(TYPE) *)ptr;                            \
    HERC_ATOMIC_ASSUME_ALIGNED(ptr, TYPE);                              \
    return atomic_load_explicit(atomic_ptr, C11_LOAD_ORDER);            \
}                                                                       \
                                                                        \
HERC_ATOMIC_SPECIFIER                                                   \
void atomic_store_##SUFFIX(volatile TYPE *ptr, TYPE val) {              \
    volatile _Atomic(TYPE) *atomic_ptr = (volatile _Atomic(TYPE) *)ptr; \
    HERC_ATOMIC_ASSUME_ALIGNED(ptr, TYPE);                              \
    atomic_store_explicit(atomic_ptr, val, C11_STORE_ORDER);            \
}

#define DEFINE_ATOMIC_OPERATION(OP_NAME, OP, TYPE, MSVC_TYPE, MSVC_FUNC, C11_FUNC, C11_ORDER, GCC_FUNC, GCC_ORDER, SYNC_FUNC) \
HERC_ATOMIC_SPECIFIER                                                 \
TYPE atomic_##OP_NAME##_##TYPE(volatile TYPE *ptr, TYPE value) {      \
    volatile _Atomic TYPE *atomic_ptr = (volatile _Atomic TYPE *)ptr; \
    return C11_FUNC(atomic_ptr, value, C11_ORDER) OP value;           \
}

#define DEFINE_ATOMIC_EXCHANGE(TYPE, MSVC_TYPE, MSVC_FUNC, C11_ORDER, GCC_ORDER) \
HERC_ATOMIC_SPECIFIER                                                            \
TYPE atomic_exchange_##TYPE(volatile TYPE *ptr, TYPE val) {                      \
    volatile _Atomic TYPE *atomic_ptr = (volatile _Atomic TYPE *)ptr;            \
    return atomic_exchange_explicit(atomic_ptr, val, C11_ORDER);                 \
}

#define DEFINE_ATOMIC_COMPARE_EXCHANGE(TYPE, MSVC_TYPE, MSVC_FUNC, C11_SUCCESS_ORDER, C11_FAIL_ORDER, GCC_SUCCESS_ORDER, GCC_FAIL_ORDER) \
HERC_ATOMIC_SPECIFIER                                                                   \
bool atomic_compare_exchange_##TYPE(volatile TYPE *ptr, TYPE *expected, TYPE desired) { \
    volatile _Atomic TYPE *atomic_ptr = (volatile _Atomic TYPE *)ptr;                   \
    return atomic_compare_exchange_weak_explicit(                                       \
        atomic_ptr, expected, desired, C11_SUCCESS_ORDER, C11_FAIL_ORDER                \
    );                                                                                  \
}

#define DEFINE_ATOMIC_MASKSET(TYPE, MSVC_TYPE, MSVC_FUNC, C11_SUCCESS_ORDER, C11_FAIL_ORDER, GCC_SUCCESS_ORDER, GCC_FAIL_ORDER) \
HERC_ATOMIC_SPECIFIER                                                             \
TYPE atomic_mask_or_##TYPE(volatile TYPE *ptr, TYPE andbits, TYPE orbits) {       \
    volatile _Atomic TYPE *atomic_ptr = (volatile _Atomic TYPE *)ptr;             \
    TYPE current = atomic_load_explicit(atomic_ptr, C11_FAIL_ORDER);              \
    TYPE target;                                                                  \
    do {                                                                          \
        target = (current & andbits) | orbits;                                    \
    } while (!atomic_compare_exchange_weak_explicit(atomic_ptr, &current, target, \
                C11_SUCCESS_ORDER, C11_FAIL_ORDER));                              \
    return current;                                                               \
}

#elif HERC_ATOMICS == IAF2_MICROSOFT_INTRINSICS

#define DEFINE_ATOMIC_LOADSTORE(TYPE, SUFFIX, C11_LOAD_ORDER, C11_STORE_ORDER, GCC_LOAD_ORDER, GCC_STORE_ORDER, IS_RELAXED, IS_ACQUIRE, IS_RELEASE) \
HERC_ATOMIC_SPECIFIER                                      \
TYPE atomic_load_##SUFFIX(const volatile TYPE *ptr) {      \
    HERC_ATOMIC_ASSUME_ALIGNED(ptr, TYPE);                 \
    if (!(IS_RELAXED) && !(IS_ACQUIRE)) {                  \
        MemoryBarrier();                                   \
    }                                                      \
    TYPE val = (TYPE)(*ptr);                               \
    if (!(IS_RELAXED) && (IS_ACQUIRE)) {                   \
        MemoryBarrier();                                   \
    }                                                      \
    return val;                                            \
}                                                          \
                                                           \
HERC_ATOMIC_SPECIFIER                                      \
void atomic_store_##SUFFIX(volatile TYPE *ptr, TYPE val) { \
    HERC_ATOMIC_ASSUME_ALIGNED(ptr, TYPE);                 \
    if (!(IS_RELAXED) && (IS_RELEASE)) {                   \
        MemoryBarrier();                                   \
    }                                                      \
    *ptr = val;                                            \
    if (!(IS_RELAXED) && !(IS_RELEASE)) {                  \
        MemoryBarrier();                                   \
    }                                                      \
}

#define DEFINE_ATOMIC_OPERATION(OP_NAME, OP, TYPE, MSVC_TYPE, MSVC_FUNC, C11_FUNC, C11_ORDER, GCC_FUNC, GCC_ORDER, SYNC_FUNC) \
HERC_ATOMIC_SPECIFIER                                                       \
TYPE atomic_##OP_NAME##_##TYPE(volatile TYPE *ptr, TYPE value) {            \
    return MSVC_FUNC((volatile MSVC_TYPE *)ptr, (MSVC_TYPE)value) OP value; \
}

#define DEFINE_ATOMIC_EXCHANGE(TYPE, MSVC_TYPE, MSVC_FUNC, C11_ORDER, GCC_ORDER) \
HERC_ATOMIC_SPECIFIER                                                            \
TYPE atomic_exchange_##TYPE(volatile TYPE *ptr, TYPE val) {                      \
    volatile MSVC_TYPE *msvc_ptr = (volatile MSVC_TYPE *)ptr;                    \
    return (TYPE)MSVC_FUNC(msvc_ptr, (MSVC_TYPE)val);                            \
}

#define DEFINE_ATOMIC_COMPARE_EXCHANGE(TYPE, MSVC_TYPE, MSVC_FUNC, C11_SUCCESS_ORDER, C11_FAIL_ORDER, GCC_SUCCESS_ORDER, GCC_FAIL_ORDER) \
HERC_ATOMIC_SPECIFIER                                                                     \
bool atomic_compare_exchange_##TYPE(volatile TYPE *ptr, TYPE *expected, TYPE desired) {   \
    volatile MSVC_TYPE *msvc_ptr = (volatile MSVC_TYPE *)ptr;                             \
    MSVC_TYPE orig_expected = *(MSVC_TYPE *)expected;                                     \
    MSVC_TYPE actual = (MSVC_TYPE)MSVC_FUNC(msvc_ptr, (MSVC_TYPE)desired, orig_expected); \
    if (actual == orig_expected) {                                                        \
        return true;                                                                      \
    } else {                                                                              \
        *(MSVC_TYPE *)expected = (TYPE)actual;                                            \
        return false;                                                                     \
    }                                                                                     \
}

#define DEFINE_ATOMIC_MASKSET(TYPE, MSVC_TYPE, MSVC_FUNC, C11_SUCCESS_ORDER, C11_FAIL_ORDER, GCC_SUCCESS_ORDER, GCC_FAIL_ORDER) \
HERC_ATOMIC_SPECIFIER                                                       \
TYPE atomic_mask_or_##TYPE(volatile TYPE *ptr, TYPE andbits, TYPE orbits) { \
    volatile MSVC_TYPE *msvc_ptr = (volatile MSVC_TYPE *)ptr;               \
    MSVC_TYPE current = *msvc_ptr;                                          \
    MSVC_TYPE target, old_value;                                            \
    do {                                                                    \
        target = (current & (MSVC_TYPE)andbits) | (MSVC_TYPE)orbits;        \
        old_value = MSVC_FUNC(msvc_ptr, target, current);                   \
        if (old_value == current) break;                                    \
        current = old_value;                                                \
    } while (1);                                                            \
    return (TYPE)current;                                                   \
}

#elif HERC_ATOMICS == IAF2_ATOMIC_INTRINSICS

#define DEFINE_ATOMIC_LOADSTORE(TYPE, SUFFIX, C11_LOAD_ORDER, C11_STORE_ORDER, GCC_LOAD_ORDER, GCC_STORE_ORDER, IS_RELAXED, IS_ACQUIRE, IS_RELEASE) \
HERC_ATOMIC_SPECIFIER                                      \
TYPE atomic_load_##SUFFIX(const volatile TYPE *ptr) {      \
    HERC_ATOMIC_ASSUME_ALIGNED(ptr, TYPE);                 \
    return __atomic_load_n(ptr, GCC_LOAD_ORDER);           \
}                                                          \
                                                           \
HERC_ATOMIC_SPECIFIER                                      \
void atomic_store_##SUFFIX(volatile TYPE *ptr, TYPE val) { \
    HERC_ATOMIC_ASSUME_ALIGNED(ptr, TYPE);                 \
    __atomic_store_n(ptr, val, GCC_STORE_ORDER);           \
}

#define DEFINE_ATOMIC_OPERATION(OP_NAME, OP, TYPE, MSVC_TYPE, MSVC_FUNC, C11_FUNC, C11_ORDER, GCC_FUNC, GCC_ORDER, SYNC_FUNC) \
HERC_ATOMIC_SPECIFIER                                            \
TYPE atomic_##OP_NAME##_##TYPE(volatile TYPE *ptr, TYPE value) { \
    return GCC_FUNC(ptr, value, GCC_ORDER) OP value;             \
}

#define DEFINE_ATOMIC_EXCHANGE(TYPE, MSVC_TYPE, MSVC_FUNC, C11_ORDER, GCC_ORDER) \
HERC_ATOMIC_SPECIFIER                                                            \
TYPE atomic_exchange_##TYPE(volatile TYPE *ptr, TYPE val) {                      \
    return __atomic_exchange_n(ptr, val, GCC_ORDER);                             \
}

#define DEFINE_ATOMIC_COMPARE_EXCHANGE(TYPE, MSVC_TYPE, MSVC_FUNC, C11_SUCCESS_ORDER, C11_FAIL_ORDER, GCC_SUCCESS_ORDER, GCC_FAIL_ORDER) \
HERC_ATOMIC_SPECIFIER                                                                                     \
bool atomic_compare_exchange_##TYPE(volatile TYPE *ptr, TYPE *expected, TYPE desired) {                   \
    return __atomic_compare_exchange_n(ptr, expected, desired, false, GCC_SUCCESS_ORDER, GCC_FAIL_ORDER); \
}

#define DEFINE_ATOMIC_MASKSET(TYPE, MSVC_TYPE, MSVC_FUNC, C11_SUCCESS_ORDER, C11_FAIL_ORDER, GCC_SUCCESS_ORDER, GCC_FAIL_ORDER) \
HERC_ATOMIC_SPECIFIER                                                       \
TYPE atomic_mask_or_##TYPE(volatile TYPE *ptr, TYPE andbits, TYPE orbits) { \
    TYPE current = *ptr;                                                    \
    TYPE target;                                                            \
    do {                                                                    \
        target = (current & andbits) | orbits;                              \
    } while (!__atomic_compare_exchange_n(ptr, &current, target, 1,         \
                GCC_SUCCESS_ORDER, GCC_FAIL_ORDER));                        \
    return current;                                                         \
}

#elif HERC_ATOMICS == IAF2_SYNC_BUILTINS

#define DEFINE_ATOMIC_LOADSTORE(TYPE, SUFFIX, C11_LOAD_ORDER, C11_STORE_ORDER, GCC_LOAD_ORDER, GCC_STORE_ORDER, IS_RELAXED, IS_ACQUIRE, IS_RELEASE) \
HERC_ATOMIC_SPECIFIER                                      \
TYPE atomic_load_##SUFFIX(const volatile TYPE *ptr) {      \
    HERC_ATOMIC_ASSUME_ALIGNED(ptr, TYPE);                 \
    if (!(IS_RELAXED) && !(IS_ACQUIRE)) {                  \
        __sync_synchronize();                              \
    }                                                      \
    TYPE val = *(const volatile TYPE * volatile)ptr;       \
    if (!(IS_RELAXED) && (IS_ACQUIRE)) {                   \
        __sync_synchronize();                              \
    }                                                      \
    return val;                                            \
}                                                          \
                                                           \
HERC_ATOMIC_SPECIFIER                                      \
void atomic_store_##SUFFIX(volatile TYPE *ptr, TYPE val) { \
    HERC_ATOMIC_ASSUME_ALIGNED(ptr, TYPE);                 \
    if (!(IS_RELAXED) && (IS_RELEASE)) {                   \
        __sync_synchronize();                              \
    }                                                      \
    *(volatile TYPE * volatile)ptr = val;                  \
    if (!(IS_RELAXED) && !(IS_RELEASE)) {                  \
        __sync_synchronize();                              \
    }                                                      \
}

#define DEFINE_ATOMIC_OPERATION(OP_NAME, OP, TYPE, MSVC_TYPE, MSVC_FUNC, C11_FUNC, C11_ORDER, GCC_FUNC, GCC_ORDER, SYNC_FUNC) \
HERC_ATOMIC_SPECIFIER                                            \
TYPE atomic_##OP_NAME##_##TYPE(volatile TYPE *ptr, TYPE value) { \
    return SYNC_FUNC(ptr, value) OP value;                       \
}

#define DEFINE_ATOMIC_EXCHANGE(TYPE, MSVC_TYPE, MSVC_FUNC, C11_ORDER, GCC_ORDER) \
HERC_ATOMIC_SPECIFIER                                                            \
TYPE atomic_exchange_##TYPE(volatile TYPE *ptr, TYPE val) {                      \
    __sync_synchronize();                                                        \
    TYPE old_val = __sync_lock_test_and_set(ptr, val);                           \
    __sync_synchronize();                                                        \
    return old_val;                                                              \
}

#define DEFINE_ATOMIC_COMPARE_EXCHANGE(TYPE, MSVC_TYPE, MSVC_FUNC, C11_SUCCESS_ORDER, C11_FAIL_ORDER, GCC_SUCCESS_ORDER, GCC_FAIL_ORDER) \
HERC_ATOMIC_SPECIFIER                                                                   \
bool atomic_compare_exchange_##TYPE(volatile TYPE *ptr, TYPE *expected, TYPE desired) { \
    TYPE orig_expected = *expected;                                                     \
    TYPE actual = __sync_val_compare_and_swap(ptr, orig_expected, desired);             \
    if (actual == orig_expected) {                                                      \
        return true;                                                                    \
    } else {                                                                            \
        *expected = actual;                                                             \
        return false;                                                                   \
    }                                                                                   \
}

#define DEFINE_ATOMIC_MASKSET(TYPE, MSVC_TYPE, MSVC_FUNC, C11_SUCCESS_ORDER, C11_FAIL_ORDER, GCC_SUCCESS_ORDER, GCC_FAIL_ORDER) \
HERC_ATOMIC_SPECIFIER                                                       \
TYPE atomic_mask_or_##TYPE(volatile TYPE *ptr, TYPE andbits, TYPE orbits) { \
    TYPE current = *ptr;                                                    \
    TYPE target, old_value;                                                 \
    do {                                                                    \
        target = (current & andbits) | orbits;                              \
        old_value = __sync_val_compare_and_swap(ptr, current, target);      \
        if (old_value == current) break;                                    \
        current = old_value;                                                \
    } while (1);                                                            \
    return current;                                                         \
}

#else

#define DEFINE_ATOMIC_LOADSTORE(TYPE, SUFFIX, C11_LOAD_ORDER, C11_STORE_ORDER, GCC_LOAD_ORDER, GCC_STORE_ORDER, IS_RELAXED, IS_ACQUIRE, IS_RELEASE) \
HERC_ATOMIC_SPECIFIER                                      \
TYPE atomic_load_##SUFFIX(const volatile TYPE *ptr) {      \
    HERC_ATOMIC_ASSUME_ALIGNED(ptr, TYPE);                 \
    return (TYPE)*ptr;                                     \
}                                                          \
                                                           \
HERC_ATOMIC_SPECIFIER                                      \
void atomic_store_##SUFFIX(volatile TYPE *ptr, TYPE val) { \
    HERC_ATOMIC_ASSUME_ALIGNED(ptr, TYPE);                 \
    *ptr = val;                                            \
}

#define DEFINE_ATOMIC_OPERATION(OP_NAME, OP, TYPE, MSVC_TYPE, MSVC_FUNC, C11_FUNC, C11_ORDER, GCC_FUNC, GCC_ORDER, SYNC_FUNC) \
HERC_ATOMIC_SPECIFIER                                   \
TYPE atomic_##OP_NAME##_##TYPE(TYPE *ptr, TYPE value) { \
    *ptr OP##= value;                                   \
    return *ptr;                                        \
}

#define DEFINE_ATOMIC_EXCHANGE(TYPE, MSVC_TYPE, MSVC_FUNC, C11_ORDER, GCC_ORDER) \
HERC_ATOMIC_SPECIFIER                                                            \
TYPE atomic_exchange_##TYPE(TYPE *ptr, TYPE val) {                               \
    TYPE old_val = *ptr;                                                         \
    *ptr = val;                                                                  \
    return old_val;                                                              \
}

#define DEFINE_ATOMIC_COMPARE_EXCHANGE(TYPE, MSVC_TYPE, MSVC_FUNC, C11_SUCCESS_ORDER, C11_FAIL_ORDER, GCC_SUCCESS_ORDER, GCC_FAIL_ORDER) \
HERC_ATOMIC_SPECIFIER                                                                   \
bool atomic_compare_exchange_##TYPE(volatile TYPE *ptr, TYPE *expected, TYPE desired) { \
    bool success = false;                                                               \
    if (*ptr == *expected) {                                                            \
        *ptr = desired;                                                                 \
        success = true;                                                                 \
    } else {                                                                            \
        *expected = *ptr;                                                               \
    }                                                                                   \
    return success;                                                                     \
}

#define DEFINE_ATOMIC_MASKSET(TYPE, MSVC_TYPE, MSVC_FUNC, C11_SUCCESS_ORDER, C11_FAIL_ORDER, GCC_SUCCESS_ORDER, GCC_FAIL_ORDER) \
HERC_ATOMIC_SPECIFIER                                              \
TYPE atomic_mask_or_##TYPE(TYPE *ptr, TYPE andbits, TYPE orbits) { \
    TYPE old_value = *ptr;                                         \
    *ptr = (old_value & andbits) | orbits;                         \
    return old_value;                                              \
}

#endif

/*
 * Atomically loads-from and stores-to *ptr. The generated functions use a
 * compile-time selected memory ordering.
 *
 * Conceptually:
 *     value = *ptr; // load
 *     *ptr = val;   // store
 */

/*
 * memory_order_seq_cst
 *
 * A load with this memory order performs an acquire operation and a store
 * performs a release operation. In addition, all sequentially consistent
 * operations participate in a single total order that is consistent with the
 * happens-before relationship and all threads observe these operations in the
 * same order.
 */
#define DEFINE_SEQ_CST_LOADSTORE(TYPE, SUFFIX) \
  DEFINE_ATOMIC_LOADSTORE(TYPE, SUFFIX, memory_order_seq_cst, memory_order_seq_cst, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST, 0, 0, 0)

/*
 * memory_order_acquire/memory_order_release
 *
 * A load with this memory order performs an acquire operation and a store
 * performs a release operation. When an acquired load observes a value stored
 * by a release operation, the release synchronizes with the acquire,
 * establishing a happens-before relationship between the operations preceding
 * the store and those following the load.
 */
#define DEFINE_ACQ_REL_LOADSTORE(TYPE, SUFFIX) \
  DEFINE_ATOMIC_LOADSTORE(TYPE, SUFFIX, memory_order_acquire, memory_order_release, __ATOMIC_ACQUIRE, __ATOMIC_RELEASE, 0, 1, 1)

/*
 * memory_order_relaxed
 *
 * No synchronization or ordering constraints are imposed on other reads or
 * writes and only the atomicity of the operations themselves are guaranteed.
 */
#define DEFINE_RELAXED_LOADSTORE(TYPE, SUFFIX) \
  DEFINE_ATOMIC_LOADSTORE(TYPE, SUFFIX, memory_order_relaxed, memory_order_relaxed, __ATOMIC_RELAXED, __ATOMIC_RELAXED, 1, 0, 0)

/*
 * Hercules Default Ordering: SEQ_CST vs. ACQ_REL
 */
#define DEFINE_DEFAULT_LOADSTORE(TYPE) \
  DEFINE_ACQ_REL_LOADSTORE(TYPE, TYPE)

/*
 * Atomically applies an operation to *ptr using value and returns the
 * previous value of *ptr (i.e., immediately before the modification).
 *
 * Conceptually:
 *     old = *ptr;
 *     *ptr = old <op> value;
 *     return old;
 */
#define DEFINE_ATOMIC_OP(OP_NAME, OP, TYPE, MSVC_TYPE, MSVC_FUNC, C11_FUNC, GCC_FUNC, SYNC_FUNC) \
  DEFINE_ATOMIC_OPERATION(OP_NAME, OP, TYPE, MSVC_TYPE, MSVC_FUNC, C11_FUNC, memory_order_acq_rel, GCC_FUNC, __ATOMIC_ACQ_REL, SYNC_FUNC)

/*
 * Atomically replaces *ptr with val and returns the previous value. The
 * exchange is performed as a single atomic read-modify-write operation,
 * ensuring no intermediate state is observable by other threads.
 *
 * Conceptually:
 *     old = *ptr;
 *     *ptr = val;
 *     return old;
 */
#define DEFINE_ATOMIC_XCHG(TYPE, MSVC_TYPE, MSVC_FUNC) \
  DEFINE_ATOMIC_EXCHANGE(TYPE, MSVC_TYPE, MSVC_FUNC, memory_order_acq_rel, __ATOMIC_ACQ_REL)

/*
 * Atomically compares *ptr against *expected and, if equal, replaces it with
 * desired.
 *
 * On success, *ptr is updated to desired, true is returned, and *expected
 * remains unchanged. On failure, false is returned and *expected is updated
 * with the current value of *ptr.
 *
 * Conceptually:
 *     if (*ptr == *expected) {
 *         *ptr = desired;
 *         return true;
 *     }
 *     *expected = *ptr;
 *     return false;
 *
 * This implementation may spuriously fail even when *ptr equals *expected.
 * Callers requiring guaranteed completion should retry until the operation
 * succeeds or the observed value no longer matches the expected value.
 */
#define DEFINE_ATOMIC_CAS(TYPE, MSVC_TYPE, MSVC_FUNC) \
    DEFINE_ATOMIC_COMPARE_EXCHANGE(TYPE, MSVC_TYPE, MSVC_FUNC, memory_order_acq_rel, memory_order_relaxed, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED)

/*
 * Atomically applies a bitmask transformation to *ptr and returns the previous
 * (old_)value of *ptr.
 *
 * Conceptually:
 *     new_value = (old_value & andbits) | orbits
 *     return old_value 
 */
#define DEFINE_ATOMIC_MSET(TYPE, MSVC_TYPE, MSVC_FUNC) \
  DEFINE_ATOMIC_MASKSET(TYPE, MSVC_TYPE, MSVC_FUNC, memory_order_acq_rel, memory_order_relaxed, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED)

/* Type definition helpers */
typedef void* voidp;

/* Workaround for S128/U128 conflict in zvector */
#if defined( __SIZEOF_INT128__ )
typedef           __int128 _S128;
typedef  unsigned __int128 _U128;
#endif

/* Workaround "#define bool _Bool" */
#define atomic_load_bool  atomic_load__Bool
#define atomic_store_bool atomic_store__Bool

/* Generic Types */
DEFINE_DEFAULT_LOADSTORE(voidp)
DEFINE_DEFAULT_LOADSTORE(int)
DEFINE_DEFAULT_LOADSTORE(_Bool)

/* Fixed Types */
DEFINE_DEFAULT_LOADSTORE( S8)
DEFINE_DEFAULT_LOADSTORE(S16)
DEFINE_DEFAULT_LOADSTORE(S32)
DEFINE_DEFAULT_LOADSTORE(S64)

DEFINE_DEFAULT_LOADSTORE( U8)
DEFINE_DEFAULT_LOADSTORE(U16)
DEFINE_DEFAULT_LOADSTORE(U32)
DEFINE_DEFAULT_LOADSTORE(U64)

#if defined( __SIZEOF_INT128__ )
DEFINE_DEFAULT_LOADSTORE(_S128)
DEFINE_DEFAULT_LOADSTORE(_U128)
#endif

DEFINE_ATOMIC_OP(or, |,  U8,   char, _InterlockedOr8,  atomic_fetch_or_explicit, __atomic_fetch_or, __sync_fetch_and_or)
DEFINE_ATOMIC_OP(or, |, U32,   long, _InterlockedOr,   atomic_fetch_or_explicit, __atomic_fetch_or, __sync_fetch_and_or)
DEFINE_ATOMIC_OP(or, |, U64, LONG64, _InterlockedOr64, atomic_fetch_or_explicit, __atomic_fetch_or, __sync_fetch_and_or)

DEFINE_ATOMIC_OP(and, &,  U8,   char, _InterlockedAnd8,  atomic_fetch_and_explicit, __atomic_fetch_and, __sync_fetch_and_and)
DEFINE_ATOMIC_OP(and, &, U32,   long, _InterlockedAnd,   atomic_fetch_and_explicit, __atomic_fetch_and, __sync_fetch_and_and)
DEFINE_ATOMIC_OP(and, &, U64, LONG64, _InterlockedAnd64, atomic_fetch_and_explicit, __atomic_fetch_and, __sync_fetch_and_and)

DEFINE_ATOMIC_OP(xor, ^,  U8,   char, _InterlockedXor8,  atomic_fetch_xor_explicit, __atomic_fetch_xor, __sync_fetch_and_xor)
DEFINE_ATOMIC_OP(xor, ^, U32,   long, _InterlockedXor,   atomic_fetch_xor_explicit, __atomic_fetch_xor, __sync_fetch_and_xor)
DEFINE_ATOMIC_OP(xor, ^, U64, LONG64, _InterlockedXor64, atomic_fetch_xor_explicit, __atomic_fetch_xor, __sync_fetch_and_xor)

DEFINE_ATOMIC_OP(add, +,  S8,   char, _InterlockedExchangeAdd8,  atomic_fetch_add_explicit, __atomic_fetch_add, __sync_fetch_and_add)
DEFINE_ATOMIC_OP(add, +, S16,  SHORT, _InterlockedExchangeAdd16, atomic_fetch_add_explicit, __atomic_fetch_add, __sync_fetch_and_add)
DEFINE_ATOMIC_OP(add, +, S32,   long, _InterlockedExchangeAdd,   atomic_fetch_add_explicit, __atomic_fetch_add, __sync_fetch_and_add)
DEFINE_ATOMIC_OP(add, +, S64, LONG64, _InterlockedExchangeAdd64, atomic_fetch_add_explicit, __atomic_fetch_add, __sync_fetch_and_add)

DEFINE_ATOMIC_OP(add, +,  U8,   char, _InterlockedExchangeAdd8,  atomic_fetch_add_explicit, __atomic_fetch_add, __sync_fetch_and_add)
DEFINE_ATOMIC_OP(add, +, U16,  SHORT, _InterlockedExchangeAdd16, atomic_fetch_add_explicit, __atomic_fetch_add, __sync_fetch_and_add)
DEFINE_ATOMIC_OP(add, +, U32,   long, _InterlockedExchangeAdd,   atomic_fetch_add_explicit, __atomic_fetch_add, __sync_fetch_and_add)
DEFINE_ATOMIC_OP(add, +, U64, LONG64, _InterlockedExchangeAdd64, atomic_fetch_add_explicit, __atomic_fetch_add, __sync_fetch_and_add)

DEFINE_ATOMIC_XCHG( U8,   char, _InterlockedExchange8)
DEFINE_ATOMIC_XCHG(U32,   long, _InterlockedExchange)
DEFINE_ATOMIC_XCHG(U64, LONG64, _InterlockedExchange64)

DEFINE_ATOMIC_CAS( U8,   char, _InterlockedCompareExchange8)
DEFINE_ATOMIC_CAS(U32,   long, _InterlockedCompareExchange)
DEFINE_ATOMIC_CAS(U64, LONG64, _InterlockedCompareExchange64)

DEFINE_ATOMIC_MSET( U8,   char, _InterlockedCompareExchange8)
DEFINE_ATOMIC_MSET(U32,   long, _InterlockedCompareExchange)
DEFINE_ATOMIC_MSET(U64, LONG64, _InterlockedCompareExchange64)

/* Cleanup internal macros */

#undef HERC_ATOMIC_SPECIFIER
#undef HERC_ATOMIC_IS_ALIGNED
#undef HERC_ATOMIC_ASSUME_ALIGNED

#undef DEFINE_ATOMIC_LOADSTORE
#undef DEFINE_ATOMIC_OPERATION
#undef DEFINE_ATOMIC_EXCHANGE
#undef DEFINE_ATOMIC_COMPARE_EXCHANGE
#undef DEFINE_ATOMIC_MASKSET

#undef DEFINE_SEQ_CST_LOADSTORE
#undef DEFINE_ACQ_REL_LOADSTORE
#undef DEFINE_RELAXED_LOADSTORE
#undef DEFINE_DEFAULT_LOADSTORE

#undef DEFINE_ATOMIC_OP
#undef DEFINE_ATOMIC_XCHG
#undef DEFINE_ATOMIC_CAS
#undef DEFINE_ATOMIC_MSET

/*-------------------------------------------------------------------*/

#endif /* _JPH_HATOMIC_H */
