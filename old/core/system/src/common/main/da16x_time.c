/**
 ****************************************************************************************
 * @file da16x_time.c
 * @brief Time handling module
 ****************************************************************************************
 */

/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley. The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Copyright (c) 2016-2022 Modified by Renesas Electronics.
 *
 */

#include "sdk_type.h"


#include <stdio.h>
#include <stdlib.h>

#include "oal.h"
#include "hal.h"
#include "monitor.h"
#include "command.h"
#include "clib.h"
#include "da16x_time.h"

#include <ctype.h>
#include <inttypes.h>
#include "locale.h"
#include "da16x_system.h"
#include "common_utils.h"
#include "da16x_compile_opt.h"
#include "da16x_sys_watchdog.h"


#undef DEBUG_DA16X_TIME
#undef IAR_TIMEZONE_DST

#define RTC_OVERFLOW		0xFFFFFFFFFULL /* Real RTC Counter 36bit */
#define RTC_OVERFLOW_STACK_SIZE	256

#define INT_MAX				((int)(~0U>>1))
#define INT_MIN				(-INT_MAX - 1)
#define WDAY				1 /* to get day of week right */

/* Bias between 1900 (struct tm) and 1970 time_t. */
#define _TBIAS_DAYS			(70 * 365L + 17)
#define _TBIAS				(_TBIAS_DAYS * 86400LU)

/* macros */
#define MONTAB(year)		((year) & 03 || (year) == 0 ? mos : lmos)

/* from retmem.h */
#define CLK2US(clk)			((((unsigned long long )clk) * 15625ULL) >> 9ULL)

#define CLK2MS(clk)			((CLK2US(clk))/1000ULL)
#define CLK2SEC(clk)		(((unsigned long long )clk) >> 15ULL)

#define US2CLK(us)			((((unsigned long long ) us) << 9ULL) / 15625ULL)
#define SEC2CLK(clk)		(((unsigned long long )clk) << 15ULL)

#define TYPE_SIGNED(type)	(!((type) 0 < (type) -1))
#define TYPE_BIT(type)		(sizeof (type) * 8)
#define INT64_STRLEN_MAX	22
#define INT_STRLEN_MAXIMUM(type) \
	((TYPE_BIT(type) - TYPE_SIGNED(type)) * 302 / 1000	\
	 + 1 + TYPE_SIGNED(type))

#define isleap(y)			(((y) % 4) == 0 && (((y) % 100) != 0 || ((y) % 400) == 0))
#define isleap_sum(a, b)	isleap((a) % 400 + (b) % 400)

#define HOURSPERDAY		24
#define DAYSPERWEEK		7
#define DAYSPERNYEAR	365
#define DAYSPERLYEAR	366
#define MONSPERYEAR		12

#define TM_YEAR_BASE	1900

/* External functions */
extern void ulltoa(unsigned long long value, char *buf, int radix);
extern char *lltoa(long long value, char *buf, int radix);
extern unsigned long long ALT_RTC_GET_COUNTER(void);
extern int chk_dpm_mode(void);
extern int chk_dpm_wakeup(void);

extern unsigned long long get_systimeoffset_from_rtm(void);
extern void 			  set_systimeoffset_to_rtm(unsigned long long offset);
extern unsigned long long get_rtc_oldtime_from_rtm(void);
extern void 			  set_rtc_oldtime_to_rtm(unsigned long long time);

/* Extern variable */
extern UCHAR	xtal_32KHz_rmv_flag;

/* static data */
static const short lmos[] = {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335};
static const short mos[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

static struct tm _Ttotm_ts = {0};

long	__timezone = 0;

#if defined (__TIME64__)
int da16x_Daysto64(int year, int mon);
#else
struct tm *da16x_Ttotm32(struct tm *t, __time32_t secsarg, int isdst);
#endif // __TIME64__

#ifdef IAR_TIMEZONE_DST /* munchang.jung_20161005 */

/* type definitions */
typedef struct
{
	/* format strings for date and time */
	const char *_Am_pm;
	const char *_Days;
	const char *_Abday;
	const char *_Day;
	const char *_Months;
	const char *_Abmon;
	const char *_Mon;
	const char *_Formats;
	const char *_D_t_fmt;
	const char *_D_fmt;
	const char *_T_fmt;
	const char *_T_fmt_ampm;
	const char *_Era_Formats;
	const char *_Era_D_t_fmt;
	const char *_Era_D_fmt;
	const char *_Era_T_fmt;
	const char *_Era_T_fmt_ampm;
	const char *_Era;
	const char *_Alt_digits;
	const char *_Isdst;
	const char *_Tzone;
} _Tinfo;

typedef struct
{
	/* rule for daylight savings time */
	unsigned char wday, hour, day, mon, year;
} Dstrule;

typedef const char *_Isdst_dst_t;
typedef Dstrule *_Isdst_rules_t;
#endif /* IAR_TIMEZONE_DST */

clock_t clock(void)
{
	if (xtal_32KHz_rmv_flag == pdTRUE)
		return ALT_RTC_GET_COUNTER();
	else
		return RTC_GET_COUNTER();
}


void da16x_SetTzoff(long offset)
{
	__timezone = offset;
}

long da16x_Tzoff(void)
{
	return __timezone;
}


#if defined (__TIME64__)
#define __isleap(y)	( !((y) % 4) && ( ((y) % 100) || !((y) % 400) ) )

struct tm *da16x_Ttotm64(struct tm *t, __time64_t secsarg, int isdst)
{
	/* convert scalar time to time structure */
	int year;
	long days;
	signed long long secs;

	secsarg += _TBIAS;   /* changed to (wraparound) time since 1 Jan 1900 */

	if (t == 0)
	{
		//t = _TLS_DATA_PTR(struct tm, _Ttotm_ts);
		t = &_Ttotm_ts;
	}

	t->tm_isdst = isdst;

	for (secs = (signed long long)secsarg; 	;
			secs = (signed long long)secsarg + 3600)       /* loop to correct for DST */
	{

		/* If secs are negative we want to get a negative year that we then add
		month, day, etc to */
		days = secs / 86400;
		secs = secs % 86400;

		if (secs < 0)
		{
			days--;
			secs += 86400;
		}

		t->tm_wday = (days + WDAY) % 7;

		if (t->tm_wday < 0)
		{
			t->tm_wday += 7;
		}

		/* determine year */
		long i;
		year = days / 365;

		while (1)
		{
			i = da16x_Daysto64(year, 0) + 365L * year;

			if (days < i)
			{
				--year;       /* correct guess and recheck */
				continue;
			}

			if (days <= (i + 365))
			{
				break;
			}

			if (days == (i + 366) && __isleap(year + 1900))
			{
				break;
			}

			++year;
		}

		days -= i;
		t->tm_year = year;
		t->tm_yday = days;

		{
			/* determine month */
			int mon;
			const short *pm = MONTAB(year);	//@suppress("Suggested parenthesis around expression")

			for (mon = 12; days < pm[--mon]; )
			{
				;
			}

			t->tm_mon = mon;
			t->tm_mday = days - pm[mon] + 1;
		}

		t->tm_hour = secs / 3600;
		secs %= 3600;
		t->tm_min = secs / 60;
		t->tm_sec = secs % 60;

		return (t);       /* loop only if <0 => 1 */
	}

	return NULL;
}

void da16x_mktime64(struct tm *t, __time64_t *now)
{
	/* convert local time structure to scalar time */
	long cdays, csecs;
	int mon, year, ymon;
	__time64_t secs;

	ymon = t->tm_mon / 12;
	mon = t->tm_mon - ymon * 12;

	if (mon < 0)
	{
		mon += 12, --ymon;
	}

	if (((ymon < 0) && (t->tm_year < INT_MIN - ymon))
			|| ((0 < ymon) && (INT_MAX - ymon < t->tm_year)))
	{
		//return ((__time64_t)(-1));
		*now = ((__time64_t)(-1));
		return;
	}

	year = t->tm_year + ymon;

	/* Note that this is correct even with a 32 bit double. */

	/* Calculate number of days. */
	cdays = da16x_Daysto64(year, mon) - 1;
	cdays += 365L * year;
	cdays += t->tm_mday;

	/* Calculate number of seconds. */
	cdays += t->tm_hour / 24;
	csecs = 3600L * (t->tm_hour % 24);
	cdays += t->tm_min / (24 * 60);
	csecs += 60L * (t->tm_min % (24 * 60));
	cdays += t->tm_sec / 86400L;
	csecs += t->tm_sec % 86400L;

	/* Change from 1900 into 1970 representation. */
	cdays -= _TBIAS_DAYS;

	/* Add them together to get seconds since 1970. */
	secs = (__time64_t)cdays * 86400L + csecs;

	da16x_Ttotm64(t, secs, t->tm_isdst);

	if (0 < t->tm_isdst)
	{
		secs -= 3600;
	}

	*now = (secs - (__time64_t) da16x_Tzoff());
}

void da16x_time64_usec(__time64_t *p, __time64_t *cur_usec)
{
	unsigned long long time_us, time_ms;
	unsigned long long rtc;

	if (xtal_32KHz_rmv_flag == pdTRUE)
		rtc = ALT_RTC_GET_COUNTER();
	else
		rtc = RTC_GET_COUNTER();

	time_us = CLK2US(rtc); /* usec. */
	time_ms = CLK2MS(rtc); /* msec. */

	if (p != NULL) {
		/* msec : (time value to set) - (Elapsed time from Power On) */
		set_systimeoffset_to_rtm((unsigned long long)((*p) * 1000ULL - time_ms));
	} else if (get_rtc_oldtime_from_rtm() != 0 && get_rtc_oldtime_from_rtm() > time_ms) { /* Check Overflow */
		unsigned long long offset;

		offset = CLK2MS(RTC_OVERFLOW + 1ULL); // Overflow time

		/* RTC overflow 36bit + old offset = Elapsed time from Power On */
		set_systimeoffset_to_rtm(offset + get_systimeoffset_from_rtm()); /* newoffset */
		set_rtc_oldtime_to_rtm(time_ms);
	}

	// In case of non-DPM mode, update oldtime which is saved in RTM
	if (chk_dpm_mode() == 0) {
		set_rtc_oldtime_to_rtm(time_ms);
	}

#if defined (DEBUG_DA16X_TIME)
	PRINTF("RTC: %u ms + Offset: %u ms = RealTime: %u Sec\n",
		   (ULONG)time_ms,
		   (ULONG)get_systimeoffset_from_rtm(),
		   (ULONG)(time_ms / 1000ULL + get_systimeoffset_from_rtm() / 1000ULL));
#endif // DEBUG_DA16X_TIME

	time_us = time_us + ((unsigned long long)get_systimeoffset_from_rtm() * 1000ULL);

	*cur_usec = time_us; /* usec */
}


#ifndef BUILD_OPT_FC9050_ASIC
unsigned long long systime_offset;
#endif // BUILD_OPT_FC9050_ASIC

void da16x_time64_msec(__time64_t *p, __time64_t *cur_msec)
{
	unsigned long long time_ms;
	unsigned long long rtc;

	if (xtal_32KHz_rmv_flag == pdTRUE)
		rtc = ALT_RTC_GET_COUNTER();
	else
		rtc = RTC_GET_COUNTER();

	time_ms = CLK2MS(rtc); /* msec. */

	if (p != NULL) {
		/* msec : (time value to set) - (Elapsed time from Power On) */
		set_systimeoffset_to_rtm((unsigned long long)((*p) * 1000ULL - time_ms));
	} else if (get_rtc_oldtime_from_rtm() != 0 && get_rtc_oldtime_from_rtm() > time_ms) { /* Check Overflow */
		unsigned long long offset;

		offset = CLK2MS(RTC_OVERFLOW + 1ULL); // Overflow time

		/* RTC overflow 36bit + old offset = Elapsed time from Power On */
		set_systimeoffset_to_rtm(offset + get_systimeoffset_from_rtm()); /* newoffset */
		set_rtc_oldtime_to_rtm(time_ms);
	}

	// In case of non-DPM mode, update oldtime which is saved in RTM
	if (chk_dpm_mode() == 0) {
		set_rtc_oldtime_to_rtm(time_ms);
	}

#if defined (DEBUG_DA16X_TIME)
	PRINTF("RTC: %u ms + Offset: %u ms = RealTime: %u Sec\n",
		   (ULONG)time_ms,
		   (ULONG)get_systimeoffset_from_rtm(),
		   (ULONG)(time_ms / 1000ULL + get_systimeoffset_from_rtm() / 1000ULL));
#endif // DEBUG_DA16X_TIME

	time_ms = time_ms + (unsigned long long)get_systimeoffset_from_rtm();

	*cur_msec = time_ms; /* msec */
}

void da16x_time64_sec(__time64_t *p, __time64_t *cur_sec)
{
	__time64_t cur_msec;

	da16x_time64_msec(p, &cur_msec);
	if ( (long long)cur_msec < 0 ) {
		*cur_sec = ((long long)cur_msec / 1000LL); /* sec */
	} else {
		*cur_sec = (cur_msec / 1000ULL); /* sec */
	}
}

void da16x_time64(__time64_t *p, __time64_t *now)
{
	da16x_time64_sec(p, now);
}

struct tm *da16x_gmtime64(const __time64_t *tod)
{
	/* convert to Greenwich Mean Time (UTC) */
	return (da16x_Ttotm64(0, *tod, 0));
}

struct tm *da16x_localtime64(const __time64_t *tod)
{
	/* convert to local time structure */
	return (da16x_Ttotm64(0, *tod + (__time64_t) da16x_Tzoff(), -1));
}

int da16x_Daysto64(int year, int mon)
{
	/* year >= 1900: leapdays from 1900 up to (not including) year plus
		days to start of month.
	year <  1900: leapdays backwards from 1900 down to (including)
	year (negative) plus days to start of month from year. */

	int days;
	int y, ly;

	if (year >= 0)
	{
		y = year - 1;
		ly = y + 300;
	}
	else
	{
		y = year;
		ly = y - 100;
	}

	days  = y / 4;
	days -= y / 100;
	days += ly / 400;

	return (days + MONTAB(year)[mon]);	//@suppress("Suggested parenthesis around expression")
}

void __uptime(__time64_t *time)
{
#ifdef BUILD_OPT_DA16200_ASIC

	if (xtal_32KHz_rmv_flag == pdTRUE)
		*time = (da16x_time_t)(ALT_RTC_GET_COUNTER() >> 15);
	else
		*time = (da16x_time_t)(RTC_GET_COUNTER() >> 15);

#else /* BUILD_OPT_DA16200_ASIC */
	*time = tx_time_get() / NX_IP_PERIODIC_RATE;
#endif /* BUILD_OPT_DA16200_ASIC */
}

void __boottime(__time64_t *boottime)
{
	__time64_t time;
	__time64_t uptime;

#ifdef BUILD_OPT_DA16200_ASIC

	if (get_systimeoffset_from_rtm() == 0)
#else
	if (systime_offset == 0)
#endif /* BUILD_OPT_DA16200_ASIC */
	{
		boottime = 0;
	}
	else
	{
		da16x_time64(NULL, &time);
		__uptime(&uptime);
		*boottime = time - uptime;
	}
}

#else

struct tm *da16x_Ttotm32(struct tm *t, __time32_t secsarg, int isdst)
{
	/* convert scalar time to time structure */
	int year;
	long days;
	unsigned long secs;

	secsarg += _TBIAS;	/* changed to (wraparound) time since 1 Jan 1900 */

	if (t == 0)
	{
		//t = _TLS_DATA_PTR(struct tm, _Ttotm_ts);
		t = &_Ttotm_ts;
	}

	t->tm_isdst = isdst;

	for (secs = (unsigned long)secsarg; ;
			secs = (unsigned long)secsarg + 3600)  	/* loop to correct for DST */
	{
		days = secs / 86400;
		t->tm_wday = (days + WDAY) % 7;

		{
			/* determine year */
			long i;

			for (year = days / 365;
					days < (i = da16x_Daysto32(year, 0) + 365L * year); )
			{
				--year;	/* correct guess and recheck */
			}

			days -= i;
			t->tm_year = year;
			t->tm_yday = days;
		}

		{
			/* determine month */
			int mon;
			const short *pm = MONTAB(year);

			for (mon = 12; days < pm[--mon]; )
			{
				;
			}

			t->tm_mon = mon;
			t->tm_mday = days - pm[mon] + 1;
		}

		secs = secs % 86400;
		t->tm_hour = secs / 3600;
		secs %= 3600;
		t->tm_min = secs / 60;
		t->tm_sec = secs % 60;


		return (t);	/* loop only if <0 => 1 */
	}
}

__time32_t da16x_mktime32(struct tm *t)
{
	/* convert local time structure to scalar time */
	long cdays, csecs;
	__time32_t secs;
	int mon, year, ymon;

	ymon = t->tm_mon / 12;
	mon = t->tm_mon - ymon * 12;

	if (mon < 0)
	{
		mon += 12, --ymon;
	}

	if (ymon < 0 && t->tm_year < INT_MIN - ymon
			|| 0 < ymon && INT_MAX - ymon < t->tm_year)
	{
		return ((__time32_t)(-1));
	}

	year = t->tm_year + ymon;

	/* Note that this is correct even with a 32 bit double. */

	/* Calculate number of days. */
	cdays = da16x_Daysto32(year, mon) - 1;
	cdays += 365L * year;
	cdays += t->tm_mday;

	/* Calculate number of seconds. */
	cdays += t->tm_hour / 24;
	csecs = 3600L * (t->tm_hour % 24);
	cdays += t->tm_min / (24 * 60);
	csecs += 60L * (t->tm_min % (24 * 60));
	cdays += t->tm_sec / 86400L;
	csecs += t->tm_sec % 86400L;

	/* Change from 1900 into 1970 representation. */
	cdays -= _TBIAS_DAYS;

	/* Add them together to get seconds since 1970. */
	secs = cdays * 86400L + csecs;

	da16x_Ttotm32(t, (__time32_t)secs, t->tm_isdst);

	if (0 < t->tm_isdst)
	{
		secs -= 3600;
	}

	return (secs - (__time32_t) da16x_Tzoff());
}

__time32_t da16x_time32_usec(da16x_time_t *p)
{
	unsigned long long time_us, time_ms;
	unsigned long long rtc;

	if (xtal_32KHz_rmv_flag == pdTRUE)
		rtc = ALT_RTC_GET_COUNTER();
	else
		rtc = RTC_GET_COUNTER();

	time_us = CLK2US(rtc);	/* usec. */
	time_ms = CLK2MS(rtc);	/* msec. */

	if (p != NULL) {
		set_systimeoffset_to_rtm((unsigned long long)((*p) * 1000ULL - time_ms));
	}

	return (time_us); /* usec */
}

__time32_t da16x_time32_msec(da16x_time_t *p)
{
	unsigned long long time_ms;
	unsigned long long rtc;

	if (xtal_32KHz_rmv_flag == pdTRUE)
		rtc = ALT_RTC_GET_COUNTER();
	else
		rtc = RTC_GET_COUNTER();

	time_ms = CLK2MS(rtc); /* msec. */

	if (p != NULL) {
		set_systimeoffset_to_rtm((unsigned long long)((*p) * 1000ULL - time_ms));
	}

	return (time_ms); /* msec */
}

__time32_t da16x_time32_sec(da16x_time_t *now)
{
	unsigned long long time_sec;

	time_sec = da16x_time32_msec(now);
	time_sec = time_sec / 1000ULL;

	return time_sec;
}

__time32_t da16x_time32(da16x_time_t *now)
{
	return da16x_time32_sec(now);
}

struct tm *da16x_gmtime32(const __time32_t *tod)
{
	/* convert to Greenwich Mean Time (UTC) */
	return (da16x_Ttotm32(0, *tod, 0));
}

struct tm *da16x_localtime32(const __time32_t *tod)
{
	/* convert to local time structure */
	return (da16x_Ttotm32(0, (*tod + (__time32_t) da16x_Tzoff()), -1));
}

int da16x_Daysto32(int year, int mon)
{
	/* compute extra days to start of month */
	int days;

	if (0 < year)  	/* correct for leap year: 1801-2099 */
	{
		days = (year - 1) / 4;
	}
	else if (year <= -4)
	{
		//	days = 1 + (4 - year) / 4;
		days = year / 4;
	}
	else
	{
		days = 0;
	}

	return (days + MONTAB(year)[mon]);
}

da16x_time_t __uptime(void)
{
	da16x_time_t time;

#ifdef BUILD_OPT_DA16200_ASIC

	if (xtal_32KHz_rmv_flag == pdTRUE)
		*time = (da16x_time_t)(ALT_RTC_GET_COUNTER() >> 15);
	else
		*time = (da16x_time_t)(RTC_GET_COUNTER() >> 15);

#else /* BUILD_OPT_DA16200_ASIC */
	time = tx_time_get() / NX_IP_PERIODIC_RATE;
#endif /* BUILD_OPT_DA16200_ASIC */

	return time;
}

da16x_time_t __boottime(void)
{
	da16x_time_t boottime;

#ifdef BUILD_OPT_DA16200_ASIC

	if (get_systimeoffset_from_rtm() == 0)
#else
	if (systime_offset == 0)
#endif /* BUILD_OPT_DA16200_ASIC */
	{
		boottime = 0;
	}
	else
	{
		boottime = da16x_time32(NULL) - __uptime();
	}

	return boottime;
}
#endif // __TIME64__


void rtc_overflow_offset(void *pvParameters)
{
	int sys_wdog_id = -1;
	DA16X_UNUSED_ARG(pvParameters);

#if defined (__TIME64__)
	__time64_t now;
#endif // __TIME64__

	sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

	while (1)
	{
		da16x_sys_watchdog_notify(sys_wdog_id);

#if defined (__TIME64__)
		da16x_time64(NULL, &now);
#else
		da16x_time32(NULL);
#endif // __TIME64__

		da16x_sys_watchdog_suspend(sys_wdog_id);

		vTaskDelay(100 * 10);

		da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
	}
}


/* True if the arithmetic type T is signed.  */
#define	true		1
#define	false		0

#define IN_NONE		0
#define IN_SOME		1
#define IN_THIS		2
#define IN_ALL		3

struct lc_time_T
{
	const char 	*mon[MONSPERYEAR];
	const char 	*month[MONSPERYEAR];
	const char 	*wday[DAYSPERWEEK];
	const char 	*weekday[DAYSPERWEEK];
	const char 	*X_fmt;
	const char 	*x_fmt;
	const char 	*c_fmt;
	const char 	*am;
	const char 	*pm;
	const char 	*date_fmt;
};

static const struct lc_time_T	C_time_locale =
{
	{
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	}, {
		"January", "February", "March", "April", "May", "June",
		"July", "August", "September", "October", "November", "December"
	}, {
		"Sun", "Mon", "Tue", "Wed",
		"Thu", "Fri", "Sat"
	}, {
		"Sunday", "Monday", "Tuesday", "Wednesday",
		"Thursday", "Friday", "Saturday"
	},

	/* X_fmt */
	"%H:%M:%S",

	/*
	** x_fmt
	** C99 requires this format.
	** Using just numbers (as here) makes Quakers happier;
	** it's also compatible with SVR4.
	*/
	"%m/%d/%y",

	/*
	** c_fmt
	** C99 requires this format.
	** Previously this code used "%D %X", but we now conform to C99.
	** Note that
	**	"%a %b %d %H:%M:%S %Y"
	** is used by Solaris 2.3.
	*/
	"%a %b %e %T %Y",

	/* am */
	"AM",

	/* pm */
	"PM",

	/* date_fmt */
	"%a %b %e %H:%M:%S %Z %Y"
};

#define Locale	(&C_time_locale)

static char *_add(const char *str, char *pt, const char *ptlim)
{
	while (pt < ptlim && (*pt = *str++) != '\0')
	{
		++pt;
	}

	return pt;
}

static char *_conv(int n, const char *format, char *pt, const char *ptlim)
{
	char	buf[INT_STRLEN_MAXIMUM(int) + 1];

	sprintf(buf, format, n);
	return _add(buf, pt, ptlim);
}

/*
** POSIX and the C Standard are unclear or inconsistent about
** what %C and %y do if the year is negative or exceeds 9999.
** Use the convention that %C concatenated with %y yields the
** same output as %Y, and that %Y contains at least 4 bytes,
** with more only if necessary.
*/

static char *_yconv(int a, int b, char convert_top, char convert_yy,
					char *pt, const char *ptlim)
{
	register int	lead;
	register int	trail;

#define DIVISOR	100
	trail = a % DIVISOR + b % DIVISOR;
	lead = a / DIVISOR + b / DIVISOR + trail / DIVISOR;
	trail %= DIVISOR;

	if (trail < 0 && lead > 0)
	{
		trail += DIVISOR;
		--lead;
	}
	else if (lead < 0 && trail > 0)
	{
		trail -= DIVISOR;
		++lead;
	}

	if (convert_top)
	{
		if (lead == 0 && trail < 0)
		{
			pt = _add("-0", pt, ptlim);
		}
		else
		{
			pt = _conv(lead, "%02d", pt, ptlim);
		}
	}

	if (convert_yy)
	{
		pt = _conv(((trail < 0) ? -trail : trail), "%02d", pt, ptlim);
	}

	return pt;
}

static char *_fmt(const char *format, const struct tm *t, char *pt,
				  const char *ptlim, int *warnp)
{
	for ( ; *format; ++format)
	{
		if (*format == '%')
		{
label:

			switch (*++format)
			{
			case '\0':
				--format;
				break;

			case 'A':
				pt = _add((t->tm_wday < 0 ||
						   t->tm_wday >= DAYSPERWEEK) ?
						  "?" : Locale->weekday[t->tm_wday],
						  pt, ptlim);
				continue;

			case 'a':
				pt = _add((t->tm_wday < 0 ||
						   t->tm_wday >= DAYSPERWEEK) ?
						  "?" : Locale->wday[t->tm_wday],
						  pt, ptlim);
				continue;

			case 'B':
				pt = _add((t->tm_mon < 0 ||
						   t->tm_mon >= MONSPERYEAR) ?
						  "?" : Locale->month[t->tm_mon],
						  pt, ptlim);
				continue;

			case 'b':
			case 'h':
				pt = _add((t->tm_mon < 0 ||
						   t->tm_mon >= MONSPERYEAR) ?
						  "?" : Locale->mon[t->tm_mon],
						  pt, ptlim);
				continue;

			case 'C':
				/*
				** %C used to do a...
				**	_fmt("%a %b %e %X %Y", t);
				** ...whereas now POSIX 1003.2 calls for
				** something completely different.
				** (ado, 1993-05-24)
				*/
				pt = _yconv(t->tm_year, TM_YEAR_BASE,
							true, false, pt, ptlim);
				continue;

			case 'c':
			{
				int warn2 = IN_SOME;

				pt = _fmt(Locale->c_fmt, t, pt, ptlim, &warn2);

				if (warn2 == IN_ALL)
				{
					warn2 = IN_THIS;
				}

				if (warn2 > *warnp)
				{
					*warnp = warn2;
				}
			}

			continue;

			case 'D':
				pt = _fmt("%m/%d/%y", t, pt, ptlim, warnp);
				continue;

			case 'd':
				pt = _conv(t->tm_mday, "%02d", pt, ptlim);
				continue;

			case 'E':
			case 'O':
				/*
				** C99 locale modifiers.
				** The sequences
				**	%Ec %EC %Ex %EX %Ey %EY
				**	%Od %oe %OH %OI %Om %OM
				**	%OS %Ou %OU %OV %Ow %OW %Oy
				** are supposed to provide alternate
				** representations.
				*/
				goto label;

			case 'e':
				pt = _conv(t->tm_mday, "%2d", pt, ptlim);
				continue;

			case 'F':
				pt = _fmt("%Y-%m-%d", t, pt, ptlim, warnp);
				continue;

			case 'H':
				pt = _conv(t->tm_hour, "%02d", pt, ptlim);
				continue;

			case 'I':
				pt = _conv((t->tm_hour % 12) ?
						   (t->tm_hour % 12) : 12,
						   "%02d", pt, ptlim);
				continue;

			case 'j':
				pt = _conv(t->tm_yday + 1, "%03d", pt, ptlim);
				continue;

			case 'k':
				/*
				** This used to be...
				**	_conv(t->tm_hour % 12 ?
				**		t->tm_hour % 12 : 12, 2, ' ');
				** ...and has been changed to the below to
				** match SunOS 4.1.1 and Arnold Robbins'
				** strftime version 3.0. That is, "%k" and
				** "%l" have been swapped.
				** (ado, 1993-05-24)
				*/
				pt = _conv(t->tm_hour, "%2d", pt, ptlim);
				continue;

			case 'l':
				/*
				** This used to be...
				**	_conv(t->tm_hour, 2, ' ');
				** ...and has been changed to the below to
				** match SunOS 4.1.1 and Arnold Robbin's
				** strftime version 3.0. That is, "%k" and
				** "%l" have been swapped.
				** (ado, 1993-05-24)
				*/
				pt = _conv((t->tm_hour % 12) ?
						   (t->tm_hour % 12) : 12,
						   "%2d", pt, ptlim);
				continue;

			case 'M':
				pt = _conv(t->tm_min, "%02d", pt, ptlim);
				continue;

			case 'm':
				pt = _conv(t->tm_mon + 1, "%02d", pt, ptlim);
				continue;

			case 'n':
				pt = _add("\n", pt, ptlim);
				continue;

			case 'p':
				pt = _add((t->tm_hour >= (HOURSPERDAY / 2)) ?
						  Locale->pm :
						  Locale->am,
						  pt, ptlim);
				continue;

			case 'R':
				pt = _fmt("%H:%M", t, pt, ptlim, warnp);
				continue;

			case 'r':
				pt = _fmt("%I:%M:%S %p", t, pt, ptlim, warnp);
				continue;

			case 'S':
				pt = _conv(t->tm_sec, "%02d", pt, ptlim);
				continue;

			case 's':
			{
				struct tm	tm;
#if defined (__TIME64__)
				char		buf[INT64_STRLEN_MAX];
#else
				char		buf[INT_STRLEN_MAXIMUM(
									da16x_time_t) + 1];
#endif // __TIME64__
				da16x_time_t		mkt;

				tm = *t;
#if defined (__TIME64__)
				da16x_mktime64(&tm, &mkt);
#else
				mkt = mktime(&tm);
#endif // __TIME64__
#if defined (__TIME64__)
				{
					char ull_str[21];

					if (TYPE_SIGNED(da16x_time_t))
					{
						lltoa(mkt, ull_str, 10);
					}
					else
					{
						ulltoa(mkt, ull_str, 10);
					}

					sprintf(buf, "%s", ull_str);
				}
#else

				if (TYPE_SIGNED(da16x_time_t))
				{
					sprintf(buf, "%" PRIdMAX, (intmax_t) mkt);
				}
				else
				{
					sprintf(buf, "%"PRIuMAX, (uintmax_t) mkt);
				}

#endif // __TIME64__
				pt = _add(buf, pt, ptlim);
			}

			continue;

			case 'T':
				pt = _fmt("%H:%M:%S", t, pt, ptlim, warnp);
				continue;

			case 't':
				pt = _add("\t", pt, ptlim);
				continue;

			case 'U':
				pt = _conv((t->tm_yday + DAYSPERWEEK -
							t->tm_wday) / DAYSPERWEEK,
						   "%02d", pt, ptlim);
				continue;

			case 'u':
				/*
				** From Arnold Robbins' strftime version 3.0:
				** "ISO 8601: Weekday as a decimal number
				** [1 (Monday) - 7]"
				** (ado, 1993-05-24)
				*/
				pt = _conv((t->tm_wday == 0) ?
						   DAYSPERWEEK : t->tm_wday,
						   "%d", pt, ptlim);
				continue;

			case 'V':	/* ISO 8601 week number */
			case 'G':	/* ISO 8601 year (four digits) */
			case 'g':	/* ISO 8601 year (two digits) */
				/*
				** From Arnold Robbins' strftime version 3.0: "the week number of the
				** year (the first Monday as the first day of week 1) as a decimal number
				** (01-53)."
				** (ado, 1993-05-24)
				**
				** From <http://www.ft.uni-erlangen.de/~mskuhn/iso-time.html> by Markus Kuhn:
				** "Week 01 of a year is per definition the first week which has the
				** Thursday in this year, which is equivalent to the week which contains
				** the fourth day of January. In other words, the first week of a new year
				** is the week which has the majority of its days in the new year. Week 01
				** might also contain days from the previous year and the week before week
				** 01 of a year is the last week (52 or 53) of the previous year even if
				** it contains days from the new year. A week starts with Monday (day 1)
				** and ends with Sunday (day 7). For example, the first week of the year
				** 1997 lasts from 1996-12-30 to 1997-01-05..."
				** (ado, 1996-01-02)
				*/
			{
				int	year;
				int	base;
				int	yday;
				int	wday;
				int	w;

				year = t->tm_year;
				base = TM_YEAR_BASE;
				yday = t->tm_yday;
				wday = t->tm_wday;

				for ( ; ; )
				{
					int	len;
					int	bot;
					int	top;

					len = isleap_sum(year, base) ?
						  DAYSPERLYEAR :
						  DAYSPERNYEAR;
					/*
					** What yday (-3 ... 3) does
					** the ISO year begin on?
					*/
					bot = ((yday + 11 - wday) %
						   DAYSPERWEEK) - 3;
					/*
					** What yday does the NEXT
					** ISO year begin on?
					*/
					top = bot -
						  (len % DAYSPERWEEK);

					if (top < -3)
					{
						top += DAYSPERWEEK;
					}

					top += len;

					if (yday >= top)
					{
						++base;
						w = 1;
						break;
					}

					if (yday >= bot)
					{
						w = 1 + ((yday - bot) /
								 DAYSPERWEEK);
						break;
					}

					--base;
					yday += isleap_sum(year, base) ?
							DAYSPERLYEAR :
							DAYSPERNYEAR;
				}

#ifdef XPG4_1994_04_09

				if ((w == 52 &&
						t->tm_mon == TM_JANUARY) ||
						(w == 1 &&
						 t->tm_mon == TM_DECEMBER))
				{
					w = 53;
				}

#endif /* defined XPG4_1994_04_09 */

				if (*format == 'V')
					pt = _conv(w, "%02d",
							   pt, ptlim);
				else if (*format == 'g')
				{
					*warnp = IN_ALL;
					pt = _yconv(year, base,
								false, true,
								pt, ptlim);
				}
				else
					pt = _yconv(year, base,
								true, true,
								pt, ptlim);
			}

			continue;

			case 'v':
				/*
				** From Arnold Robbins' strftime version 3.0:
				** "date as dd-bbb-YYYY"
				** (ado, 1993-05-24)
				*/
				pt = _fmt("%e-%b-%Y", t, pt, ptlim, warnp);
				continue;

			case 'W':
				pt = _conv((t->tm_yday + DAYSPERWEEK -
							(t->tm_wday ?
							 (t->tm_wday - 1) :
							 (DAYSPERWEEK - 1))) / DAYSPERWEEK,
						   "%02d", pt, ptlim);
				continue;

			case 'w':
				pt = _conv(t->tm_wday, "%d", pt, ptlim);
				continue;

			case 'X':
				pt = _fmt(Locale->X_fmt, t, pt, ptlim, warnp);
				continue;

			case 'x':
			{
				int	warn2 = IN_SOME;

				pt = _fmt(Locale->x_fmt, t, pt, ptlim, &warn2);

				if (warn2 == IN_ALL)
				{
					warn2 = IN_THIS;
				}

				if (warn2 > *warnp)
				{
					*warnp = warn2;
				}
			}

			continue;

			case 'y':
				*warnp = IN_ALL;
				pt = _yconv(t->tm_year, TM_YEAR_BASE, false, true, pt, ptlim);
				continue;

			case 'Y':
				pt = _yconv(t->tm_year, TM_YEAR_BASE, true, true, pt, ptlim);
				continue;

			case '+':
				pt = _fmt(Locale->date_fmt, t, pt, ptlim, warnp);
				continue;

			case '%':

			/*
			** X311J/88-090 (4.12.3.5): if conversion char is
			** undefined, behavior is undefined. Print out the
			** character itself as printf(3) also does.
			*/
			default:
				break;
			}
		}

		if (pt == ptlim)
		{
			break;
		}

		*pt++ = *format;
	}

	return pt;
}

#ifndef YEAR_2000_NAME
#define YEAR_2000_NAME	"CHECK_STRFTIME_FORMATS_FOR_TWO_DIGIT_YEARS"
#endif /* !defined YEAR_2000_NAME */

size_t da16x_strftime(char *ptr, size_t maxsize, const char *format, const struct tm *t)
{
	char 	*p;
	int	warn;

	warn = IN_NONE;
	p = _fmt(((format == NULL) ? "%c" : format), t, ptr, ptr + maxsize, &warn);
#ifndef NO_RUN_TIME_WARNINGS_ABOUT_YEAR_2000_PROBLEMS_THANK_YOU

	if (warn != IN_NONE)
	{
		PRINTF("\n");

		if (format == NULL)
		{
			PRINTF("NULL strftime format ");
		}
		else
			PRINTF("strftime format \"%s\" ",
				   format);

		PRINTF("yields only two digits of years in ");

		if (warn == IN_SOME)
		{
			PRINTF("some locales");
		}
		else if (warn == IN_THIS)
		{
			PRINTF("the current locale");
		}
		else
		{
			PRINTF("all locales");
		}

		PRINTF("\n");
	}

#endif /* !defined NO_RUN_TIME_WARNINGS_ABOUT_YEAR_2000_PROBLEMS_THANK_YOU */

	if (p == ptr + maxsize)
	{
		return 0;
	}

	*p = '\0';
	return p - ptr;
}


UINT checkRTC_OverflowCreate(void)
{
	UINT status = 0;
#if 0
	int dpm_status = 0;
	UINT use = 0;
#endif	/* 0 */
#if defined (__TIME64__)
	__time64_t now;
#endif // __TIME64__

	if (chk_dpm_mode() && chk_dpm_wakeup())
	{
#if 0	/* Sleep daemon is executed not yet, so don't need it */

		if (chk_dpm_reg_state(REG_NAME_DPM_RTC_OVERFLOW) == DPM_NOT_REGISTERED)
		{

			status = reg_dpm_sleep(REG_NAME_DPM_RTC_OVERFLOW, NULL);

			if (status == DPM_REG_ERR)
			{
				PRINTF("[%d] DPM_RTC_OVERFLOW reg_dpm_sleep fail\n",
					   __func__);
				return status;
			}
		}

		clr_dpm_sleep(REG_NAME_DPM_RTC_OVERFLOW);
#endif
#if defined (__TIME64__)
		da16x_time64(NULL, &now);
#else
		da16x_time32(NULL);
#endif // __TIME64__

#if 0	/* Sleep daemon is executed not yet, so don't need it */
		dpm_status = set_dpm_sleep(REG_NAME_DPM_RTC_OVERFLOW);

		if (DPM_SET_OK != dpm_status)
		{
			PRINTF(ANSI_COLOR_RED "[%d] dpm_status=%d\n" ANSI_COLOR_DEFULT, __func__, dpm_status);
		}

#endif
	}
	else
	{

		/* Create the sntp client thread */
		status = xTaskCreate(rtc_overflow_offset,
							"RTC_OVER_Thd",
							RTC_OVERFLOW_STACK_SIZE,
							(void *)NULL,
							OS_TASK_PRIORITY_LOWEST,
							NULL);
	}

	return status;
}

// Check the time setting status
int chk_time_set_status(void)
{
	struct tm tm_diff;
#ifdef __TIME64__
	__time64_t now, diff;
#else
	time_t now, diff;
#endif /* __TIME64__ */

	/* current time */
#ifdef __TIME64__
	da16x_time64(NULL, &now);
#else
	now = da16x_time32(NULL);
#endif /* __TIME64__ */

	// 1971.01.01 00:00:00
	tm_diff.tm_year = 1971 - 1900;
	tm_diff.tm_mon  = 1 - 1;
	tm_diff.tm_mday = 1;
	tm_diff.tm_hour = 0;
	tm_diff.tm_min  = 0;
	tm_diff.tm_sec  = 0;
	tm_diff.tm_isdst = -1;

#ifdef __TIME64__
	da16x_mktime64(&tm_diff, &diff);
#else
	diff = da16x_mktime32(&tm_diff);
#endif

	if (now < diff) {
		return pdFALSE;
	} else {
		return pdTRUE;
	}
}

/* EOF */
