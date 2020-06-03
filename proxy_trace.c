#include <stdio.h>
#include <stdarg.h>

#define COLORIZATION_OUTPUT

#ifdef COLORIZATION_OUTPUT
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#else
#define COLOR_RESET   ""
#define COLOR_RED     ""
#define COLOR_GREEN   ""
#define COLOR_YELLOW  ""
#define COLOR_BLUE    ""
#endif

#define TRACE_END          COLOR_RESET "\n"
#define TRACE_START_OK     COLOR_GREEN "OK: "
#define TRACE_START_INFO   COLOR_BLUE "Info: "
#define TRACE_START_WARN   COLOR_YELLOW "Warning: "
#define TRACE_START_ERROR  COLOR_RED "Error: "

#include "proxy_trace.h"

int trace(TraceLevel tl, const char* format, ...) {
    va_list args;
    va_start(args, format);

    int rc;
    switch (tl) {
    case TL_OK:
        printf(TRACE_START_OK);
        rc = vprintf(format, args);
        printf(TRACE_END);
        break;
    case TL_INFO:
        printf(TRACE_START_INFO);
        rc = vprintf(format, args);
        printf(TRACE_END);
        break;
    case TL_WARN:
        fprintf(stderr, TRACE_START_WARN);
        rc = vfprintf(stderr, format, args);
        fprintf(stderr, TRACE_END);
        break;
    case TL_ERROR:
    default:
        fprintf(stderr, TRACE_START_ERROR);
        rc = vfprintf(stderr, format, args);
        fprintf(stderr, TRACE_END);
        break;
    }

    va_end(args);
    return rc;
}
