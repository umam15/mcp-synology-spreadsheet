#include "catat.h"

#include <stdarg.h>
#include <stdio.h>

static void catat(const char *tingkat, const char *format, va_list argumen)
{
    fprintf(stderr, "[mcp-synology-spreadsheet] %s: ", tingkat);
    vfprintf(stderr, format, argumen);
    fputc('\n', stderr);
    fflush(stderr);
}

void catat_info(const char *format, ...)
{
    va_list argumen;
    va_start(argumen, format);
    catat("info", format, argumen);
    va_end(argumen);
}

void catat_galat(const char *format, ...)
{
    va_list argumen;
    va_start(argumen, format);
    catat("galat", format, argumen);
    va_end(argumen);
}
