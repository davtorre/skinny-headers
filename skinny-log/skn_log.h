/*  skn_log.h — header-only logger
 *
 *  MODES
 *    SKN_LOG_PLAIN (1) : message only
 *    SKN_LOG_TIMED (2) : prepends [YYYY-MM-DD HH:mm:ss] to every message
 *    SKN_LOG_LEVEL (3) : prepends [YYYY-MM-DD HH:mm:ss] INFO/WARNING/ERROR -
 *
 *  LOG LEVELS (used with SKN_LOG_LEVEL mode)
 *    SKN_LOG_INFO, SKN_LOG_WARN, SKN_LOG_ERR
 *    Ignored in modes 1 and 2.
 *
 *  TIMER
 *    slog_start_timer / slog_end_timer bracket timed sections.
 *    Uses clock_gettime(CLOCK_MONOTONIC) — POSIX only (Linux, macOS).
 *    In the slog_end_timer message string, %t is replaced with the elapsed
 *    time formatted in the most readable unit (µs, ms, s, min, h, days).
 *    %% outputs a literal %; all other text is printed verbatim.
 *    Note: µs uses a UTF-8 encoded µ character; ensure your terminal is UTF-8.
 *
 *  TIMESTAMP DELIMITERS
 *    slog_init takes a ts_open/ts_close pair wrapped around every timestamp
 *    (default "[" / "]"); pass NULL for either to keep the default.
 *    slog_timestamp(log) formats the current time with those delimiters and
 *    returns a pointer to it, for callers that want the string directly
 *    instead of only the automatic slog_print/slog_end_timer prefix.
 *
 *  USAGE
 *    #define SKN_LOG_IMPLEMENTATION
 *    #include "skn_log.h"
 *
 *  EXAMPLE
 *    SknLog *log = slog_init(SKN_LOG_LEVEL, stdout, NULL, NULL);
 *    slog_print(log, SKN_LOG_INFO, "starting computation\n");
 *    slog_start_timer(log);
 *    heavy_work();
 *    slog_end_timer(log, SKN_LOG_INFO, "done, took %t\n");
 *    slog_free(log);
 */

#ifndef SKN_LOG_H
#define SKN_LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

/* -------------------------------------------------------------------------
 * Public types
 * ---------------------------------------------------------------------- */

typedef enum {
    SKN_LOG_PLAIN = 1,   /* message only */
    SKN_LOG_TIMED = 2,   /* [YYYY-MM-DD HH:mm:ss] prefix on every message */
    SKN_LOG_LEVEL = 3    /* [YYYY-MM-DD HH:mm:ss] INFO/WARNING/ERROR - message */
} SknLogMode;

typedef enum {
    SKN_LOG_INFO = 0,
    SKN_LOG_WARN = 1,
    SKN_LOG_ERR  = 2
} SknLogLevel;

typedef struct {
    FILE           *stream;
    SknLogMode      mode;
    char           *name;       /* optional label, set by slog_set_name */
    struct timespec t_start;   /* set by slog_start_timer */
    char           *ts_open;    /* timestamp opening delimiter, set at init */
    char           *ts_close;   /* timestamp closing delimiter, set at init */
    char            ts_buf[64]; /* buffer written to by slog_timestamp */
} SknLog;

/* -------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------- */

/* Allocate and initialise a logger writing to stream.
 * stream may be stdout, stderr, or any open FILE*.
 * ts_open/ts_close are the delimiters wrapped around every timestamp
 * (used internally in SKN_LOG_TIMED/SKN_LOG_LEVEL modes, and by
 * slog_timestamp()). Pass NULL for either to fall back to its default,
 * "[" / "]".
 * Returns NULL on allocation failure. */
SknLog *slog_init(SknLogMode mode, FILE *stream,
                   const char *ts_open, const char *ts_close);

/* Free the logger. Does not close the underlying stream. */
void slog_free(SknLog *log);

/* Assign a label to the logger. The name is copied internally and appears in
 * every subsequent message: "[name] message" in PLAIN mode,
 * "[timestamp] [name] message" in TIMED/LEVEL mode.
 * Pass NULL to clear a previously set name. */
void slog_set_name(SknLog *log, const char *name);

/* Format the current time as "YYYY-MM-DD HH:mm:ss", wrapped in the logger's
 * configured delimiters (ts_open/ts_close, default "[" / "]"), writes the
 * result into an internal buffer owned by log, and returns a pointer to it.
 * The returned pointer is valid until the next call to slog_timestamp on
 * the same logger, or until slog_free. Not reentrant/thread-safe. */
const char *slog_timestamp(SknLog *log);

/* Print a printf-style formatted message.
 * In SKN_LOG_TIMED mode a [YYYY-MM-DD HH:mm:ss] prefix is prepended.
 * In SKN_LOG_LEVEL mode the prefix also includes the level label.
 * level is ignored in SKN_LOG_PLAIN and SKN_LOG_TIMED modes. */
void slog_print(SknLog *log, SknLogLevel level, const char *fmt, ...);

/* Record the start of a timed section (wall-clock). */
void slog_start_timer(SknLog *log);

/* Print msg, replacing %t with the elapsed time since slog_start_timer.
 * The time unit is selected automatically for readability.
 * %% in msg outputs a literal %; all other text is printed verbatim.
 * In SKN_LOG_TIMED mode a timestamp prefix is prepended.
 * In SKN_LOG_LEVEL mode the prefix also includes the level label.
 * level is ignored in SKN_LOG_PLAIN and SKN_LOG_TIMED modes. */
void slog_end_timer(SknLog *log, SknLogLevel level, const char *msg);

/* You found it. */
void _found_secret(void);

/* -------------------------------------------------------------------------
 * Implementation
 * ---------------------------------------------------------------------- */

#ifdef SKN_LOG_IMPLEMENTATION

static void log__print_timestamp(SknLog *log)
{
    fprintf(log->stream, "%s ", slog_timestamp(log));
}

static void log__print_name(FILE *stream, const char *name)
{
    if (name) fprintf(stream, "[%s] ", name);
}

static void log__print_level(FILE *stream, SknLogLevel level)
{
    static const char *labels[] = { "INFO", "WARNING", "ERROR" };
    fprintf(stream, "%s - ", labels[level]);
}

static long long log__elapsed_ns(struct timespec *start)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (long long)(now.tv_sec  - start->tv_sec)  * 1000000000LL
                    + (now.tv_nsec - start->tv_nsec);
}

static void log__format_elapsed(long long ns, char *buf, size_t bufsz)
{
    if      (ns < 1000000LL)        snprintf(buf, bufsz, "%.2f \xc2\xb5s",  ns / 1e3);
    else if (ns < 1000000000LL)     snprintf(buf, bufsz, "%.2f ms",  ns / 1e6);
    else if (ns < 60000000000LL)    snprintf(buf, bufsz, "%.2f s",   ns / 1e9);
    else if (ns < 3600000000000LL)  snprintf(buf, bufsz, "%.2f min", ns / 6e10);
    else if (ns < 86400000000000LL) snprintf(buf, bufsz, "%.2f h",   ns / 3.6e12);
    else                            snprintf(buf, bufsz, "%.2f days", ns / 8.64e13);
}

SknLog *slog_init(SknLogMode mode, FILE *stream,
                   const char *ts_open, const char *ts_close)
{
    SknLog *log = (SknLog *)malloc(sizeof(SknLog));
    if (!log) return NULL;
    log->stream          = stream;
    log->mode            = mode;
    log->name            = NULL;
    log->t_start.tv_sec  = 0;
    log->t_start.tv_nsec = 0;
    log->ts_buf[0]       = '\0';

    if (!ts_open)  ts_open  = "[";
    if (!ts_close) ts_close = "]";
    size_t open_len  = strlen(ts_open);
    size_t close_len = strlen(ts_close);
    log->ts_open  = (char *)malloc(open_len + 1);
    log->ts_close = (char *)malloc(close_len + 1);
    if (!log->ts_open || !log->ts_close) {
        free(log->ts_open);
        free(log->ts_close);
        free(log);
        return NULL;
    }
    memcpy(log->ts_open,  ts_open,  open_len + 1);
    memcpy(log->ts_close, ts_close, close_len + 1);

    return log;
}

void slog_free(SknLog *log)
{
    if (log) {
        free(log->name);
        free(log->ts_open);
        free(log->ts_close);
    }
    free(log);
}

void slog_set_name(SknLog *log, const char *name)
{
    free(log->name);
    if (name) {
        size_t len = strlen(name);
        log->name = (char *)malloc(len + 1);
        if (log->name) memcpy(log->name, name, len + 1);
    } else {
        log->name = NULL;
    }
}

const char *slog_timestamp(SknLog *log)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char buf[20]; /* "YYYY-MM-DD HH:mm:ss\0" */
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", t);
    snprintf(log->ts_buf, sizeof(log->ts_buf), "%s%s%s",
             log->ts_open, buf, log->ts_close);
    return log->ts_buf;
}

void slog_print(SknLog *log, SknLogLevel level, const char *fmt, ...)
{
    if (log->mode == SKN_LOG_PLAIN) {
        log__print_name(log->stream, log->name);
    } else if (log->mode == SKN_LOG_TIMED) {
        log__print_timestamp(log);
        log__print_name(log->stream, log->name);
    } else if (log->mode == SKN_LOG_LEVEL) {
        log__print_timestamp(log);
        log__print_name(log->stream, log->name);
        log__print_level(log->stream, level);
    }
    va_list ap;
    va_start(ap, fmt);
    vfprintf(log->stream, fmt, ap);
    va_end(ap);
}

void slog_start_timer(SknLog *log)
{
    clock_gettime(CLOCK_MONOTONIC, &log->t_start);
}

void slog_end_timer(SknLog *log, SknLogLevel level, const char *msg)
{
    long long ns = log__elapsed_ns(&log->t_start);
    char elapsed[32];
    log__format_elapsed(ns, elapsed, sizeof(elapsed));

    if (log->mode == SKN_LOG_PLAIN) {
        log__print_name(log->stream, log->name);
    } else if (log->mode == SKN_LOG_TIMED) {
        log__print_timestamp(log);
        log__print_name(log->stream, log->name);
    } else if (log->mode == SKN_LOG_LEVEL) {
        log__print_timestamp(log);
        log__print_name(log->stream, log->name);
        log__print_level(log->stream, level);
    }

    const char *p = msg;
    while (*p) {
        if (*p == '%') {
            if      (*(p + 1) == 't') { fputs(elapsed, log->stream); p += 2; }
            else if (*(p + 1) == '%') { fputc('%',     log->stream); p += 2; }
            else                      { fputc('%',     log->stream); p++;    }
        } else {
            fputc(*p, log->stream);
            p++;
        }
    }
}

void _found_secret(void)
{
#include ".print_data.h"
    puts(data);
}

#endif /* SKN_LOG_IMPLEMENTATION */
#endif /* SKN_LOG_H */
