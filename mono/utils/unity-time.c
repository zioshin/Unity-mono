/*
 * Time utility functions.
 * Author: Paolo Molaro (<lupus@ximian.com>)
 * Copyright (C) 2008 Novell, Inc.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

#include <config.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <utils/mono-time.h>

gint64
mono_msec_ticks (void)
{
	return (gint64) UnityPalGetTicksMillisecondsMonotonic();
}

/* Returns the number of 100ns ticks from unspecified time: this should be monotonic */
gint64
mono_100ns_ticks (void)
{
	return (gint64) UnityPalGetTicks100NanosecondsMonotonic();
}

/* Returns the number of 100ns ticks since 1/1/1601, UTC timezone */
gint64
mono_100ns_datetime (void)
{
	return (gint64) UnityPalGetTicks100NanosecondsDateTime();
}

gint64
mono_msec_boottime (void)
{
	g_assert_not_reached();
	return 0;
}

gint64
mono_100ns_datetime_from_timeval (struct timeval tv)
{
	g_assert_not_reached();
	return 0;
}

