/**
 ****************************************************************************************
 *
 * @file da16x_time.h
 *
 * @brief Header file for time structure
 *
 * Copyright (c) 2016-2022 Renesas Electronics. All rights reserved.
 *
 * This software ("Software") is owned by Renesas Electronics.
 *
 * By using this Software you agree that Renesas Electronics retains all
 * intellectual property and proprietary rights in and to this Software and any
 * use, reproduction, disclosure or distribution of the Software without express
 * written permission or a license agreement from Renesas Electronics is
 * strictly prohibited. This Software is solely for use on or in conjunction
 * with Renesas Electronics products.
 *
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, THE
 * SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. EXCEPT AS OTHERWISE
 * PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, IN NO EVENT SHALL
 * RENESAS ELECTRONICS BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THE SOFTWARE.
 *
 ****************************************************************************************
 */


#ifndef	__DA16X_TIME__
#define	__DA16X_TIME__

#include <stddef.h>
#include <time.h>

#define __TIME64__	/* Support 64bit time */

typedef unsigned long clock_t;

#if defined (__TIME64__)
typedef unsigned long long __time64_t;
typedef __time64_t	da16x_time_t;
#else
typedef unsigned long	__time32_t;
typedef __time32_t	da16x_time_t;
#endif // __TIME64__


#if defined (__TIME64__)
/// Convert time_t to tm as UTC time
#define gmtime(p)	da16x_gmtime64(p)
/// Convert tm structure to time_t
#define localtime(p)	da16x_localtime64(p)

/**
 ****************************************************************************************
 * @brief Get current time / Set current time (for 64bit)
 * @param[in]	p	Set the time of p value to the current time.\n
 * If NULL, no current time is set.
 * @param[out]	now Get the current time.
 ****************************************************************************************
 */
void da16x_time64(__time64_t *p, __time64_t *now);


/**
 ****************************************************************************************
 * @brief Convert __time64_t to tm as UTC time
 * @param[in]	tod	Pointer to an object of type time_t that contains a time value.\n
 * __time64_t is an alias of a fundamental arithmetic type capable of representing times as returned by function time.
 * @return		A pointer to a tm structure with its members filled with the values that correspond to the UTC time representation of tod.
 ****************************************************************************************
 */
struct tm *da16x_gmtime64(const __time64_t *tod);


/**
 ****************************************************************************************
 * @brief Convert __time64_t to tm as local time
 * @param[in]	tod	Pointer to an object of type __time64_t that contains a time value.\n
 * time_t is an alias of a fundamental arithmetic type capable of representing times as returned by function time.
 * @return		A pointer to a tm structure with its members filled with the values that correspond to the local time representation of tod.
 ****************************************************************************************
 */
struct tm *da16x_localtime64(const __time64_t *tod);


/**
 ****************************************************************************************
 * @brief Convert tm structure to __time64_t
 * @param[in]	t	Pointer to a tm structure that contains a calendar time broken down into its components (see struct tm)
 * @param[out]	now	Returns the value of type __time64_t that represents the local time described by the tm structure pointed by t 
 ****************************************************************************************
 */
void da16x_mktime64(struct tm *t, __time64_t *now);
#else
/// Get current time
#define time(p)	da16x_time32(p)
/// Convert time_t to tm as UTC time
#define gmtime(p)	da16x_gmtime32(p)
/// Convert time_t to tm as local time
#define localtime(p)	da16x_localtime32(p)
/// Convert tm structure to __time64_
#define mktime(p)	da16x_mktime32(p);


/**
 ****************************************************************************************
 * @brief Get current time / Set current time (for 32bit) 
 * @param[in]	p	Set the time of p value to the current time.\n
 * If NULL, no current time is set.
 * @param[out]	now	Get the current time.
 ****************************************************************************************
 */
__time32_t da16x_time32(__time32_t *p);


/**
 ****************************************************************************************
 * @brief Convert __time32_t to tm as UTC time
 * @param[in]	tod	Pointer to an object of type time_t that contains a time value.\n
 * __time32_t is an alias of a fundamental arithmetic type capable of representing times as returned by function time.
 * @return		A pointer to a tm structure with its members filled with the values that correspond to the UTC time representation of tod.
 ****************************************************************************************
 */
struct tm *da16x_gmtime32(const __time32_t *tod);


/**
 ****************************************************************************************
 * @brief Convert __time32_t to tm as local time
 * @param[in]	tod	Pointer to an object of type __time32_t that contains a time value.\n
 * time_t is an alias of a fundamental arithmetic type capable of representing times as returned by function time.
 * @return		A pointer to a tm structure with its members filled with the values that correspond to the local time representation of tod.
 ****************************************************************************************
 */
struct tm *da16x_localtime32(const __time32_t *tod);


/**
 ****************************************************************************************
 * @brief Convert tm structure to __time32_t
 * @param[in]	t	Pointer to a tm structure that contains a calendar time broken down into its components (see struct tm)
 * @return			Returns the value of type __time32_t that represents the local time described by the tm structure pointed by t 
 ****************************************************************************************
 */
__time32_t da16x_mktime32(struct tm *t);
#endif // __TIME64__


/**
 ****************************************************************************************
 * @brief Get timezone offset from GMT
 * @return	Returns a value of type time_t(long) that represents a timezone offset value.
 ****************************************************************************************
 */
long da16x_Tzoff(void);


/**
 ****************************************************************************************
 * @brief Set the timezone offset from GMT
 * param[in]	offset	Set time zone offset value of type time_t(long)
 ****************************************************************************************
 */
void da16x_SetTzoff(long offset);


/**
 ****************************************************************************************
 * @brief Format time as string
 * @param[out] s Pointer to the destination array where the resulting C string is copied.
 * @param[in] maxsize Maximum number of characters to be copied to ptr, including the terminating null-character.
 * @param[in] format C string containing any combination of regular characters and special format specifiers.\n
 * These format specifiers are replaced by the function to the corresponding values to represent the time specified in timeptr. 
 * @return	If the length of the resulting C string, including the terminating null-character, doesn't exceed maxsize,\n
 * the function returns the total number of characters copied to ptr (not including the terminating null-character).\n
 * Otherwise, it returns zero, and the contents of the array pointed by ptr are indeterminate.
 ****************************************************************************************
 */
size_t da16x_strftime(char *ptr, size_t maxsize, const char *format,
							const struct tm *t);


#if defined (__TIME64__)
/**
 ****************************************************************************************
 * @brief Get elapsed time after system power up(for 64bit)
 * param[out]	time	Pointer to an object of type time_t that contains a time value.
 ****************************************************************************************
 */
void __uptime(__time64_t *time);


/**
 ****************************************************************************************
 * @brief Get elapsed time after RTOS boot(for 64bit)
 * param[out]	boottime	Pointer to an object of type time_t that contains a time value.
 ****************************************************************************************
 */
void __boottime(__time64_t *boottime);
#else
/**
 ****************************************************************************************
 * @brief Get elapsed time after system power up(for 32bit)
 * param[out]	time	Pointer to an object of type time_t that contains a time value.
 ****************************************************************************************
 */
time_t __uptime(void);


/**
 ****************************************************************************************
 * @brief Get elapsed time after RTOS boot(for 32bit)
 * @return	Returns a value of type __time32_t that represents the boot time.
 ****************************************************************************************
 */
time_t __boottime(void);
#endif // __TIME64__

/**
 ****************************************************************************************
 * @brief Get elapsed time after RTOS boot(for 32bit)
 * @return	Returns a value of type int representing the time setting state.
 ****************************************************************************************
 */
int chk_time_set_status(void);

#endif // __DA16X_TIME__

/* EOF */
