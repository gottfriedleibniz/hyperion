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

/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Interlocked Access Facility 2 Configuration                       */
/*-------------------------------------------------------------------*/

#if !defined( DISABLE_IAF2 )
  #define CAN_IAF2          HERC_ATOMICS
#else /* defined( DISABLE_IAF2 ) */
  #define CAN_IAF2          IAF2_ATOMICS_UNAVAILABLE
#endif

#if CAN_IAF2 == IAF2_ATOMICS_UNAVAILABLE
  #define H_ATOMIC_OP( ptr, imm, op, Op, fallback )                 \
    (*ptr fallback ## = imm)
#elif CAN_IAF2 == IAF2_C11_STANDARD_ATOMICS
  #define H_ATOMIC_OP( ptr, imm, op, Op, fallback )                 \
    (atomic_fetch_ ## op( (_Atomic BYTE*)ptr, imm ) fallback imm)
#elif CAN_IAF2 == IAF2_ATOMIC_INTRINSICS
  #define H_ATOMIC_OP( ptr, imm, op, Op, fallback )                 \
    (__atomic_ ## op ## _fetch( ptr, imm, __ATOMIC_SEQ_CST ))
#elif defined( HAVE_SYNC_BUILTINS )
  #define H_ATOMIC_OP( ptr, imm, op, Op, fallback )                 \
    (__sync_ ## op ## _and_fetch( ptr, imm ))
#elif CAN_IAF2 == IAF2_MICROSOFT_INTRINSICS
  /* Microsoft functions, as per Fish                            */

  /* The  casts  of  the  function arguments here are to silence */
  /* pointer  warnings for the three cases that are not true and */
  /* which the compiler later deletes as dead code.              */

  /* Due to MSVC's _Interlocked intrinsics being typed to return */
  /* a signed value coupled with C language rules pertaining to  */
  /* second and third operands of the ?: conditional operator as */
  /* well as the order of our sizeof() tests the results of each */
  /* intrinsic must also be cast to unsigned in order to prevent */
  /* sign extension of the intrinsic return value with one bits, */
  /* thereby causing an incorrect condition code 1 instead of 0. */

  /* Note that the 64-bit intrinsic function is not available in */
  /* 32 bit windows, hence the library function is used; it will */
  /* compile as the intrinsic function on 64 bit.                */

  #define H_ATOMIC_OP(ptr, imm, op, Op, fallback)                                                  \
  (                                                                                                \
    (sizeof(*(ptr)) == 8) ? (((U64)  Interlocked ## Op ## 64 ((U64*) ptr, imm )) fallback imm ) :  \
    (sizeof(*(ptr)) == 4) ? (((U32) _Interlocked ## Op       ((U32*) ptr, imm )) fallback imm ) :  \
    (sizeof(*(ptr)) == 2) ? (((U16) _Interlocked ## Op ## 16 ((U16*) ptr, imm )) fallback imm ) :  \
    (sizeof(*(ptr)) == 1) ? (((U8)  _Interlocked ## Op ## 8  ((U8*)  ptr, imm )) fallback imm ) :  \
    (assert(0) /* returns void */, 0 /* to get integral result */)                                 \
  )
#else /* (none of the above) */
  #error LOGIC ERROR! in header file hatomic.h!
#endif /* CAN_IAF2 ... */

/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Atomic Operations                                                 */
/*-------------------------------------------------------------------*/

#define atomic_or_U8(ptr, imm)    H_ATOMIC_OP( (volatile U8*)(ptr), (imm),  or,  Or, | )
#define atomic_and_U8(ptr, imm)   H_ATOMIC_OP( (volatile U8*)(ptr), (imm), and, And, & )
#define atomic_xor_U8(ptr, imm)   H_ATOMIC_OP( (volatile U8*)(ptr), (imm), xor, Xor, ^ )

#define atomic_add_U32(ptr, imm) ((void)H_ATOMIC_OP( (ptr), (imm), add, ExchangeAdd, + ))
#define atomic_add_U64(ptr, imm) ((void)H_ATOMIC_OP( (ptr), (imm), add, ExchangeAdd, + ))
#define atomic_add_S64(ptr, imm) atomic_add_U64(ptr, imm)

/*-------------------------------------------------------------------*/

#endif /* _JPH_HATOMIC_H */
