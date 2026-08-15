#pragma once
#include "app_config.h"
#include <stddef.h>
#include <stdbool.h>

bool App_ParseMeasurement(const char *line, AppMeasurement *out);
bool App_ParseConfig(const char *line, AppWifiConfig *out);
int App_FormatMeasurement(char *dst, size_t capacity, const AppMeasurement *m);
