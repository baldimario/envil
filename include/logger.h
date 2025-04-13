#ifndef ENVIL_LOGGER_H
#define ENVIL_LOGGER_H

#include "types.h"
#include "config.h"

void logger(LogLevel level, const char* message, ...);

#endif // ENVIL_LOGGER_H