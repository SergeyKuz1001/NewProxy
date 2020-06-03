#ifndef PROXY_TRACE_H
#define PROXY_TRACE_H

typedef enum {
    TL_OK,
    TL_INFO,
    TL_WARN,
    TL_ERROR
} TraceLevel;

int trace(TraceLevel tl, const char* format, ...);

#endif
