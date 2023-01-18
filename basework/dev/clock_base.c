/*
 * Copyright 2022 wtcat
 */

#include <errno.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "basework/dev/clock.h"
#include "basework/container/list.h"


static const struct clock_timer *ctimer;


/** Convert a UNIX time to civil time.
 *
 * This converts integral seconds since (before) 1970-01-01T00:00:00
 * to the POSIX standard civil time representation.  Any adjustments
 * due to time zone, leap seconds, or a different epoch must be
 * applied to @p time before invoking this function.
 *
 * @param time the time represented as seconds.
 *
 * @return the time information for corresponding to the provided
 * instant.
 *
 * @see http://howardhinnant.github.io/date_algorithms.html#civil_from_days
 */
static void time_civil_from_days(vtime_t z, struct tod_time *tp) {
	tp->tm_wday = (z >= -4) ? ((z + 4) % 7) : ((z + 5) % 7 + 6);
	z += 719468;

	vtime_t era = ((z >= 0) ? z : (z - 146096)) / 146097;
	unsigned int doe = (z - era * (vtime_t)146097);
	unsigned int yoe = (doe - doe / 1460U + doe / 36524U - doe / 146096U)
		/ 365U;
	vtime_t y = (vtime_t)yoe + era * 400;
	unsigned int doy = doe - (365U * yoe + yoe / 4U - yoe / 100U);
	unsigned int mp = (5U * doy + 2U) / 153U;
	unsigned int d = doy - (153U * mp + 2U) / 5U + 1U;
	unsigned int m = mp + ((mp < 10) ? 3 : -9);

	tp->tm_year = y + (m <= 2) - 1900;
	tp->tm_mon = m - 1;
	tp->tm_mday = d;

	/* Everything above is explained on the referenced page, but
	 * doy is relative to --03-01 and we need it relative to
	 * --01-01.
	 *
	 * doy=306 corresponds to --01-01, doy=364 to --02-28, and
	 * doy=365 to --02-29.  So we can just subtract 306 to handle
	 * January and February.
	 *
	 * For doy<306 we have to add the number of days before
	 * --03-01, which is 59 in a common year and 60 in a leap
	 * year.  Note that the first year in the era is a leap year.
	 */
	if (doy >= 306U) {
		tp->tm_yday = doy - 306U;
	} else {
		tp->tm_yday = doy + 59U + (((yoe % 4U == 0U) && (yoe % 100U != 0U)) || (yoe == 0U));
	}
}

/* Convert a UNIX time to civil time.
 *
 * This converts integral seconds since (before) 1970-01-01T00:00:00
 * to the POSIX standard civil time representation.  Any adjustments
 * due to time zone, leap seconds, or a different epoch must be
 * applied to @p time before invoking this function.
 */
int clock_get_tod_time(const vtime_t *timep, struct tod_time *result) {
    if (timep == NULL || result == NULL)
        return -EINVAL;

    vtime_t z = *timep;
	vtime_t days = (z >= 0 ? z : z - 86399) / 86400;
	unsigned int rem = z - days * 86400;

	time_civil_from_days(days, result);
	result->tm_hour = rem / 60U / 60U;
	rem -= result->tm_hour * 60 * 60;
	result->tm_min = rem / 60;
	result->tm_sec = rem - result->tm_min * 60;

	return 0;
}

/*
 * clock_get_time - Get UTC time
 */
int clock_get_time(struct utime *tv) {
    const struct clock_timer *timer = ctimer;
    assert(timer != NULL);
    assert(timer->get_time != NULL);

    return timer->get_time(tv);
}

int clock_register(const struct clock_timer *timer) {
	if (!timer || !timer->get_time)
		return -EINVAL;

	if (ctimer && ctimer->stop) 
		ctimer->stop();

    ctimer = timer;
    return 0;
}
