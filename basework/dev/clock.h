/*
 * Copyright 2022 wtcat
 */
#ifndef BASEWORK_CLOCK_CLOCK_H_
#define BASEWORK_CLOCK_CLOCK_H_

#ifdef __cplusplus
extern "C"{
#endif

#ifndef __vtime_t_defined__
#define __vtime_t_defined__
typedef long vtime_t;
#endif

struct utime {
	vtime_t tv_sec;
	long tv_nsec;
};

struct tod_time {
	int tm_sec;
	int tm_min;  /* Minutes (0-59) */
	int tm_hour; /* Hours (0-23) */
	int tm_mday; /* Day of month (1-31) */
	int tm_mon;  /* Month (0-11; January = 0) */
	int tm_year;
	int tm_wday; /* Day of week (0-6; Sunday = 0) */
	int tm_yday; /* Day of year (0-365; January 1 = 0) */
};

struct clock_timer {
    int (*get_time)(struct utime *);
    int (*set_time)(const struct utime *);
    void (*stop)(void);
};

/*
 * clock_get_time - Get 
 */
int clock_get_time(struct utime *tv);
int clock_get_tod_time(const vtime_t *timep, struct tod_time *result);

/*
 * clock_register - Register a clock base timer
 * 
 * @timer: timer operations pointer
 * return 0 if success
 */
int clock_register(const struct clock_timer *timer);

/*
 * get_time - Get UTC time
 *
 * return UTC time
 */
static inline vtime_t get_time(void) {
    struct utime tv;
    clock_get_time(&tv);
    return tv.tv_sec;
}

/*
 * get_tod_time - Get the time of day
 *
 * @tod: time pointer
 * return 0 if success
 */
static inline int get_tod_time(struct tod_time *tod) {
    vtime_t tv = get_time();
    return clock_get_tod_time(&tv, tod); 
}

#ifdef __cplusplus
}
#endif
#endif /* BASEWORK_CLOCK_CLOCK_H_ */
