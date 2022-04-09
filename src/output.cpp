#include <Arduino.h>
#include <stdio.h>  /* printf */
#include <stdarg.h> /* va_list, va_start, va_arg, va_end */
#include "output.h"

#define BUFF_SIZE 256

void output(const char *format, ...)
{
    char buffer[BUFF_SIZE] = {0};
    va_list argp;
    va_start(argp, format);
    vsnprintf(buffer, BUFF_SIZE, format, argp);
    va_end(argp);

    Serial.println(buffer);
}