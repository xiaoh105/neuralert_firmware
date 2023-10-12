/****************************************************************************
* The confidential and proprietary information contained in this file may   *
* only be used by a person authorised under and to the extent permitted     *
* by a subsisting licensing agreement from Arm Limited (or its affiliates). *
*     (C) COPYRIGHT [2001-2018] Arm Limited (or its affiliates).            *
*         ALL RIGHTS RESERVED                                               *
* This entire notice must be reproduced on all copies of this file          *
* and copies of this file may only be made by a person if such person is    *
* permitted to do so under the terms of a subsisting license agreement      *
* from Arm Limited (or its affiliates).                                     *
*****************************************************************************/

/*!
 @file
 @brief This file contains CryptoCell PAL platform-dependant compiler-related definitions.
 @defgroup cc_pal_compiler CryptoCell PAL platform-dependant compiler-specific definitions
 @brief Contains CryptoCell PAL platform-dependant compiler-related definitions.
 @{
 @ingroup cc_pal
*/
#ifndef __CC_PAL_COMPILER_H__
#define __CC_PAL_COMPILER_H__

#ifdef __GNUC__

/* *********************** Defines ******************************/

/*! Associate a symbol with a link section. */
#define CC_PAL_COMPILER_SECTION(sectionName)  __attribute__((section(sectionName)))

/*! Mark symbol as used, that is, prevent the garbage collector from dropping it. */
#define CC_PAL_COMPILER_KEEP_SYMBOL __attribute__((used))

/*! Make a given data item aligned (alignment in Bytes). */
#define CC_PAL_COMPILER_ALIGN(alignement)  __attribute__((aligned(alignement)))

/*! Mark a function that never returns. */
#define CC_PAL_COMPILER_FUNC_NEVER_RETURNS __attribute__((noreturn))

/*! Prevent a function from being inlined. */
#define CC_PAL_COMPILER_FUNC_DONT_INLINE __attribute__((noinline))

/*! Given data type might serve as an alias for another data-type pointer. */
/* (this is used for "superclass" struct casting)             */
#define CC_PAL_COMPILER_TYPE_MAY_ALIAS __attribute__((__may_alias__))

/*! Get the size of a structure-type member. */
#define CC_PAL_COMPILER_SIZEOF_STRUCT_MEMBER(type_name, member_name) \
    sizeof(((type_name *)0)->member_name)

/*! Definition of assertion. */
#define CC_ASSERT_CONCAT_(a, b) a##b
/*! Definition of assertion. */
#define CC_ASSERT_CONCAT(a, b) CC_ASSERT_CONCAT_(a, b)
/*! Definition of assertion. */
#define CC_PAL_COMPILER_ASSERT(cond, message) \
    enum { CC_ASSERT_CONCAT(assert_line_, __LINE__) = 1/(!!(cond)) }

#elif defined(__ARM_DSM__)
#define inline

/*! Associate a symbol with a link section. */
#define CC_PAL_COMPILER_SECTION(sectionName)  __attribute__((section(sectionName)))

/*! Mark a symbol as used, i.e., prevent garbage collector from dropping it. */
#define CC_PAL_COMPILER_KEEP_SYMBOL __attribute__((used))

/*! Make a given data item aligned (alignment in Bytes). */
#define CC_PAL_COMPILER_ALIGN(alignement)  __attribute__((aligned(alignement)))

/*! Mark a function that never returns. */
#define CC_PAL_COMPILER_FUNC_NEVER_RETURNS __attribute__((noreturn))

/*! Prevent a function from being inlined. */
#define CC_PAL_COMPILER_FUNC_DONT_INLINE __attribute__((noinline))

/*! Given data type might serve as an alias for another data-type pointer. */
/* (this is used for "superclass" struct casting)             */
#define CC_PAL_COMPILER_TYPE_MAY_ALIAS __attribute__((__may_alias__))

/*! Get the size of a structure-type member. */
#define CC_PAL_COMPILER_SIZEOF_STRUCT_MEMBER(type_name, member_name) \
    sizeof(((type_name *)0)->member_name)

/*! Definition of assertion. */
#define CC_ASSERT_CONCAT_(a, b) a##b
#define CC_ASSERT_CONCAT(a, b) CC_ASSERT_CONCAT_(a, b)
#define CC_PAL_COMPILER_ASSERT(cond, message) \
    enum { CC_ASSERT_CONCAT(assert_line_, __LINE__) = 1/(!!(cond)) }


#elif defined(__ARM_DS__)
#define inline

/*! Associate a symbol with a link section. */
#define CC_PAL_COMPILER_SECTION(sectionName)  __attribute__((section(sectionName)))

/*! Mark a symbol as used, that is, prevent garbage collector from dropping it. */
#define CC_PAL_COMPILER_KEEP_SYMBOL __attribute__((used))

/*! Make a given data item aligned (alignment in Bytes). */
#define CC_PAL_COMPILER_ALIGN(alignement)  __attribute__((aligned(alignement)))

/*! Mark a function that never returns. */
#define CC_PAL_COMPILER_FUNC_NEVER_RETURNS __attribute__((noreturn))

/*! Prevent a function from being inlined. */
#define CC_PAL_COMPILER_FUNC_DONT_INLINE __attribute__((noinline))

/*! Given data type might serve as an alias for another data-type pointer. */
/* (this is used for "superclass" struct casting)             */
#define CC_PAL_COMPILER_TYPE_MAY_ALIAS

/*! Get the size of a structure-type member. */
#define CC_PAL_COMPILER_SIZEOF_STRUCT_MEMBER(type_name, member_name) \
    sizeof(((type_name *)0)->member_name)

/*! Definition of assertion. */
#define CC_ASSERT_CONCAT_(a, b) a##b
#define CC_ASSERT_CONCAT(a, b) CC_ASSERT_CONCAT_(a, b)
#define CC_PAL_COMPILER_ASSERT(cond, message) \
    enum { CC_ASSERT_CONCAT(assert_line_, __LINE__) = 1/(!!(cond)) }


#elif defined(__ARM_DS5__)
#define inline __inline


/*! Associate a symbol with a link section. */
#define CC_PAL_COMPILER_SECTION(sectionName)  __attribute__((section(sectionName)))

/*! Mark symbol as used, that is, prevent garbage collector from dropping it. */
#define CC_PAL_COMPILER_KEEP_SYMBOL __attribute__((used))

/*! Make a given data item aligned (alignment in Bytes). */
#define CC_PAL_COMPILER_ALIGN(alignement)  __attribute__((aligned(alignement)))

/*! Mark a function that never returns. */
#define CC_PAL_COMPILER_FUNC_NEVER_RETURNS __attribute__((noreturn))

/*! Prevent a function from being inlined. */
#define CC_PAL_COMPILER_FUNC_DONT_INLINE __attribute__((noinline))

/*! Given data type might serve as an alias for another data-type pointer. */
/* (this is used for "superclass" struct casting)             */
#define CC_PAL_COMPILER_TYPE_MAY_ALIAS

/*! Get the size of a structure-type member. */
#define CC_PAL_COMPILER_SIZEOF_STRUCT_MEMBER(type_name, member_name) \
    sizeof(((type_name *)0)->member_name)

/*! Definition of assertion. */
#define CC_ASSERT_CONCAT_(a, b) a##b
#define CC_ASSERT_CONCAT(a, b) CC_ASSERT_CONCAT_(a, b)
#define CC_PAL_COMPILER_ASSERT(cond, message) \
    enum { CC_ASSERT_CONCAT(assert_line_, __LINE__) = 1/(!!(cond)) }


#elif defined(__ICCARM__)
//IAR: #define inline __inline

#define	_QUOTEME(a)		#a

/*! Associate a symbol with a link section. */
#define CC_PAL_COMPILER_SECTION(sectionName)  _Pragma(_QUOTEME(location=sectionName))

/*! Mark symbol as used, that is, prevent garbage collector from dropping it. */
#define CC_PAL_COMPILER_KEEP_SYMBOL __attribute__((used))

/*! Make a given data item aligned (alignment in Bytes). */
#define CC_PAL_COMPILER_ALIGN(alignement) //_Pragma(_QUOTEME(data_alignment=alignement))

/*! Mark a function that never returns. */
#define CC_PAL_COMPILER_FUNC_NEVER_RETURNS __attribute__((noreturn))

/*! Prevent a function from being inlined. */
#define CC_PAL_COMPILER_FUNC_DONT_INLINE __attribute__((noinline))

/*! Given data type might serve as an alias for another data-type pointer. */
/* (this is used for "superclass" struct casting)             */
#define CC_PAL_COMPILER_TYPE_MAY_ALIAS

/*! Get the size of a structure-type member. */
#define CC_PAL_COMPILER_SIZEOF_STRUCT_MEMBER(type_name, member_name) \
    sizeof(((type_name *)0)->member_name)

/*! Definition of assertion. */
#define CC_ASSERT_CONCAT_(a, b) a##b
#define CC_ASSERT_CONCAT(a, b) CC_ASSERT_CONCAT_(a, b)
#define CC_PAL_COMPILER_ASSERT(cond, message) \
    enum { CC_ASSERT_CONCAT(assert_line_, __LINE__) = 1/(!!(cond)) }


#else
#error Unsupported compiler.
#endif


#endif /*__CC_PAL_COMPILER_H__*/
/**
  @}
 */

