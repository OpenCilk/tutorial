/* -*- c -*- */

/**
 * [Include-only header library]
 * C/C++ timer utilities using POSIX `clock_gettime()`.
 *
 * @sa <https://github.com/sillycross/mlpds/blob/master/fasttime.h>
 *
 * @file        ctimer.h
 * @version     1.0.0
 * @author      Alexandros-Stavros Iliopoulos
 * @license     MIT
 * @copyright   Copyright (c) 2021 Supertech Research Group, CSAIL, MIT
 */


/******************************************************************************/
/* MIT License                                                                */
/*                                                                            */
/* Copyright (c) 2021 Supertech Research Group, CSAIL, MIT                    */
/*                                                                            */
/* Permission is hereby granted, free of charge, to any person obtaining      */
/* a copy of this software and associated documentation files (the            */
/* "Software"), to deal in the Software without restriction, including        */
/* without limitation the rights to use, copy, modify, merge, publish,        */
/* distribute, sublicense, and/or sell copies of the Software, and to         */
/* permit persons to whom the Software is furnished to do so, subject to      */
/* the following conditions:                                                  */
/*                                                                            */
/* The above copyright notice and this permission notice shall be             */
/* included in all copies or substantial portions of the Software.            */
/*                                                                            */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,            */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF         */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.     */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY       */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,       */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE          */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                     */
/******************************************************************************/


/**
 * @mainpage
 *
 * @section overview Overview
 *
 * CTimer is an include-only header library for timing C/C++ code.  CTimer uses
 * the [POSIX `clock_gettime()`][url-man-clock-gettime] function with a
 * `CLOCK_MONOTONIC` clock to get relative time stamps.
 *
 * [url-man-clock-gettime]: https://man7.org/linux/man-pages/man3/clock_gettime.3.html
 *
 * Stopwatch utilities:
 * - `ctimer_t`         :: type of CTimer stopwatch struct
 * - `ctimer_start()`   :: start stopwatch
 * - `ctimer_stop()`    :: stop stopwatch
 * - `ctimer_reset()`   :: reset elapsed time
 * - `ctimer_measure()` :: measure elapsed time between start & stop
 * - `ctimer_lap()`     :: accumulate elapsed time between start & stop
 * - `ctimer_print()`   :: print elapsed time in sec with fixed format
 *
 * Timespec struct utilities
 * - `timespec_sub()`   :: calculate difference between 2 timespecs
 * - `timespec_add()`   :: calculate sum of 2 timespecs
 * - `timespec_sec()`   :: timespec tv time in sec (double)
 * - `timespec_msec()`  :: timespec tv time in msec (long)
 * - `timespec_usec()`  :: timespec tv time in usec (long)
 * - `timespec_nsec()`  :: timespec tv time in nsec (long)
 *
 * @section usage Using CTimer
 *
 * @subsection c_std C standard
 *
 * C compilers may require standard `gnu99` or later.  Older compilers may also
 * require linking with `-lrt`.
 *
 * @subsection init Initialization
 *
 * There are no guarantees regarding the initial values of timespec fields in a
 * `ctimer_t` stopwatch.  Querying timespecs that haven't been initialized or
 * measured may return arbitrary results; this includes measuring the elapsed
 * time of an unstopped stopwatch.
 *
 * The `elapsed` timespec of a `ctimer_t` stopwatch can be reset to 0 using the
 * `ctimer_reset()` function.  This is not necessary if timings are only
 * measured using `ctimer_measure()`, but it *is* necessary before using
 * `ctimer_lap()` with an otherwise un-measured stopwatch.
 *
 * @subsection measure_on_stop Automatic elapsed-time measurement on stop
 *
 * If the preprocessor macro `CTIMER_MEASURE_ON_STOP` is defined, then
 * `ctimer_stop()` also calls `ctimer_measure()` internally to calculate and
 * store the elapsed time in the input `ctimer_t` object.
 *
 * @subsection example Example usage in C/C++
 *
 * @snippet ctimer_example.c ctimer_example
 */


#ifndef __H_CTIMER__
#define __H_CTIMER__


#include <time.h>
#include <unistd.h>
#include <stdio.h>


/**
 * @defgroup ctimer CTimer
 *
 * C/C++ timer utilities.
 *
 * @{
 */


/* prevent C++ compilers from mangling function names */
#ifdef __cplusplus
extern "C" {
#endif


/* ==================================================
 * CONSTANTS
 * ================================================== */


/* unit conversion constants */
#ifdef __cplusplus              /* C++ */
namespace {
    const long _MSEC_PER_SEC = 1000;
    const long _USEC_PER_SEC = 1000 * 1000;
    const long _NSEC_PER_SEC = 1000 * 1000 * 1000;
}
#else  /* C */
enum {
    _MSEC_PER_SEC = 1000,
    _USEC_PER_SEC = 1000 * 1000,
    _NSEC_PER_SEC = 1000 * 1000 * 1000
};
#endif  /* __cplusplus */


/* ==================================================
 * TIMESPEC API
 * ================================================== */


/**
 * @defgroup ctimer_timespec Timespec API
 *
 * Functions for manipulating and inspecting timespec structs.
 *
 * @{
 */


/**
 * Calculate the time difference between two `timespec` structs.  Store time in
 * sec and nsec in the `tv_sec` and `tv_nsec` field, respectively, of the output
 * `timespec`.
 *
 * @sa https://stackoverflow.com/a/53708448/1036677
 */
static inline
void timespec_sub(
    struct timespec       * t_diff, /**<[out] time difference */
    struct timespec const   t_end,  /**<[in]  end time */
    struct timespec const   t_start /**<[in]  start time */
) {
    t_diff->tv_nsec = t_end.tv_nsec - t_start.tv_nsec;
    t_diff->tv_sec  = t_end.tv_sec  - t_start.tv_sec;
    if ((t_diff->tv_sec > 0) && (t_diff->tv_nsec < 0)) {
        t_diff->tv_nsec += _NSEC_PER_SEC;
        t_diff->tv_sec--;
    } else if ((t_diff->tv_sec < 0) && (t_diff->tv_nsec > 0)) {
        t_diff->tv_nsec -= _NSEC_PER_SEC;
        t_diff->tv_sec++;
    }
    /* (s > 0 & ns > 0) : do nothing (t_start < t_end) */
    /* (s < 0 & ns < 0) : do nothing (t_start > t_end) */
}


/**
 * Calculate the time sum of two `timespec` structs.  Store time in sec and nsec
 * in the `tv_sec` and `tv_nsec` field, respectively, of the output `timespec`.
 */
static inline
void timespec_add(
    struct timespec       * t_sum, /**<[out] time sum */
    struct timespec const   t1,    /**<[in]  time 1 */
    struct timespec const   t2     /**<[in]  time 2 */
) {
    t_sum->tv_nsec = t1.tv_nsec + t2.tv_nsec;
    t_sum->tv_sec  = t1.tv_sec  + t2.tv_sec;
    if (t_sum->tv_nsec >= _NSEC_PER_SEC) {
        t_sum->tv_nsec -= _NSEC_PER_SEC;
        t_sum->tv_sec++;
    }
}


/**
 * Return `timespec` time in sec.
 *
 * @return (t.tv_sec + t.tv_nsec) in sec
 */
static inline
double timespec_sec(
    struct timespec const t
) {
    return (double)t.tv_sec
        + (double)t.tv_nsec / _NSEC_PER_SEC;
}


/**
 * Return `timespec` time in msec.
 *
 * @warning Sub-millisecond resolution (if supported by the system clock) is
 * lost with this function.
 *
 * @return (t.tv_sec + t.tv_nsec) in msec
 */
static inline
long timespec_msec(
    struct timespec const t
) {
    return t.tv_sec * _MSEC_PER_SEC
        + t.tv_nsec / _USEC_PER_SEC;
}


/**
 * Return `timespec` time in usec.
 *
 * @warning Sub-microsecond resolution (if supported by the system clock) is
 * lost with this function.
 *
 * @return (t.tv_sec + t.tv_nsec) in usec
 */
static inline
long timespec_usec(
    struct timespec const t
) {
    return t.tv_sec * _USEC_PER_SEC
        + t.tv_nsec / _MSEC_PER_SEC;
}


/**
 * Return `timespec` time in nsec.
 *
 * @return (t.tv_sec + t.tv_nsec) in nsec
 */
static inline
long timespec_nsec(
    struct timespec const t
) {
    return t.tv_sec * _NSEC_PER_SEC
        + t.tv_nsec;
}


/** @} */ /* end group ctimer_timespec */


/* ==================================================
 * STOPWATCH API
 * ================================================== */


/**
 * @defgroup ctimer_stopwatch Stopwatch API
 *
 * Functions for manipulating CTimer stopwatches.
 *
 * @{
 */


/**
 * Stopwatch timer struct using `clock_gettime()`.
 */
typedef struct {
    struct timespec start;      /**< Stopwatch start time  */
    struct timespec end;        /**< Stopwatch end time */
    struct timespec elapsed;    /**< Elapsed/measured time */
} ctimer_t;


/**
 * Measure elapsed time of `ctimer_t` stopwatch in s+ns and *store* it in the
 * `elapsed` timer.
 *
 * @warning The stopwatch must be started and stopped before the elapsed time is
 * measured.
 *
 * @note It is safe (albeit unnecessary) to measure the elapsed time of a
 * stopped timer multiple times.
 *
 * @note When measuring the cumulative execution time of non-contiguous chunks
 * of a program (e.g., the total time for a sub-computation of a loop body), use
 * `ctimer_lap()`.
 *
 * @sa ctimer_lap
 * @sa ctimer_start
 * @sa ctimer_stop
 */
static inline
void ctimer_measure(
    ctimer_t * t                /**<[in,out] stopwatch pointer */
) {
    timespec_sub(&t->elapsed, t->end, t->start);
}


/**
 * Measure elapsed time of `ctimer_t` stopwatch in s+ns and *add* it to the
 * `elapsed` timer.
 *
 * @warning The `elapsed` field of the input stopwatch must be properly
 * initialized (e.g. with `ctimer_reset()`) before calling `ctimer_lap()`.
 *
 * @warning The stopwatch must be started and stopped before the elapsed time is
 * measured.
 *
 * @note When measuring the execution time of a single, contiguous chunk of a
 * program, use `ctimer_measure()`.
 *
 * @sa ctimer_reset
 * @sa ctimer_measure
 * @sa ctimer_start
 * @sa ctimer_stop
 */
static inline
void ctimer_lap(
    ctimer_t * t                /**<[in,out] stopwatch pointer */
) {
    /* elapsed += end - start */
    timespec_add(&t->elapsed, t->elapsed, t->end);
    timespec_sub(&t->elapsed, t->elapsed, t->start);
}


/**
 * Zero out the `elapsed` timer of a `ctimer_t` stopwatch.
 *
 * @sa ctimer_lap
 */
static inline
void ctimer_reset(
    ctimer_t * t                /**<[in,out] stopwatch pointer */
) {
    t->elapsed = (struct timespec){0};
}


/**
 * Start a `ctimer_t` stopwatch.  Sets the the `start` timer of the stopwatch.
 *
 * @sa ctimer_stop
 * @sa ctimer_measure
 * @sa ctimer_lap
 */
static inline
void ctimer_start(
    ctimer_t * t                /**<[in,out] stopwatch pointer */
) {
    clock_gettime(CLOCK_MONOTONIC, &t->start);
}


/**
 * Stop a `ctimer_t` stopwatch.  Sets the `end` timer of the stopwatch.
 *
 * @note If the `CTIMER_MEASURE_ON_STOP` preprocessor macro is defined _before_
 * the `ctimer/ctimer.h` library is included in a program, then `ctimer_stop`
 * also calculates the elapsed time between `start` and `end` and stores it in
 * the `elapsed` field.
 *
 * @sa ctimer_start
 * @sa ctimer_measure
 * @sa ctimer_lap
 */
static inline
void ctimer_stop(
    ctimer_t * t                /**<[in,out] stopwatch pointer */
) {
    clock_gettime(CLOCK_MONOTONIC, &t->end);
#ifdef CTIMER_MEASURE_ON_STOP
    ctimer_measure(t);
#endif
}


/**
 * Print a line with the `elapsed` time of a `ctimer_t` stopwatch in seconds.
 *
 * The line is printed as:
 * ```
 * Time(<label>) = XX.XXXXXXXXX sec
 * ```
 *
 * If `label` is `NULL` or the empty string, the "(<label>)" tag is omitted
 * from the printed output.
 *
 * @note The elapsed time is always printed with 9 decimal digits, although the
 * system `CLOCK_MONOTONIC`, may have coarser resolution (e.g., microsecond
 * resolution).
 *
 * @sa ctimer_measure
 * @sa ctimer_lap
 */
static inline
void ctimer_print(
    ctimer_t const   t,         /**<[in] stopwatch pointer */
    char     const * label      /**<[in] label/description for printed time */
) {
    if ((label != NULL) && (label[0] != '\0'))
        printf("Time(%s) = ", label);
    else
        printf("Time = ");

    printf("%ld.%09ld sec\n", (long)t.elapsed.tv_sec, t.elapsed.tv_nsec);
}


/** @} */ /* end group ctimer_stopwatch */


#ifdef __cplusplus
} /* end extern "C" */
#endif


/** @} */ /* end group ctimer */


#endif  /* __H_CTIMER__ */
