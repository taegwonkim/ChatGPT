#include "app_protocol.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

static bool json_string(const char *json, const char *key, char *dst, size_t cap)
{
    char pattern[40];
    if (snprintf(pattern, sizeof pattern, "\"%s\"", key) <= 0) return false;
    const char *p = strstr(json, pattern);
    if (!p || !(p = strchr(p + strlen(pattern), ':'))) return false;
    while (*++p == ' ') {}
    if (*p++ != '\"') return false;
    const char *end = strchr(p, '\"');
    size_t n = end ? (size_t)(end - p) : cap;
    if (!end || n >= cap || memchr(p, '\\', n)) return false;
    memcpy(dst, p, n); dst[n] = 0;
    return true;
}

static bool json_uint(const char *json, const char *key, unsigned long max, unsigned long *out)
{
    char pattern[40], *end;
    snprintf(pattern, sizeof pattern, "\"%s\"", key);
    const char *p = strstr(json, pattern);
    if (!p || !(p = strchr(p + strlen(pattern), ':'))) return false;
    errno = 0; unsigned long v = strtoul(p + 1, &end, 10);
    if (errno || end == p + 1 || v > max) return false;
    *out = v; return true;
}

bool App_ParseConfig(const char *line, AppWifiConfig *out)
{
    if (!line || !out || strncmp(line, "CFG,", 4)) return false;
    AppWifiConfig tmp = {0}; unsigned long port;
    if (!json_string(line + 4, "ssid", tmp.ssid, sizeof tmp.ssid) ||
        !json_string(line + 4, "password", tmp.password, sizeof tmp.password) ||
        !json_string(line + 4, "ip", tmp.server_ip, sizeof tmp.server_ip) ||
        !json_uint(line + 4, "port", 65535, &port) || port == 0) return false;
    const char *d = strstr(line + 4, "\"dhcp\"");
    if (!d || !(d = strchr(d, ':'))) return false;
    while (*++d == ' ') {}
    if (!strncmp(d, "true", 4)) tmp.dhcp = 1;
    else if (!strncmp(d, "false", 5)) tmp.dhcp = 0;
    else return false;
    tmp.server_port = (uint16_t)port; *out = tmp; return true;
}

bool App_ParseMeasurement(const char *line, AppMeasurement *out)
{
    unsigned long seq; long value; char tail;
    if (!line || !out || sscanf(line, "MEAS,%lu,%ld %c", &seq, &value, &tail) != 2) return false;
    out->sequence = (uint32_t)seq; out->value = (int32_t)value; return true;
}

int App_FormatMeasurement(char *dst, size_t cap, const AppMeasurement *m)
{
    return snprintf(dst, cap, "MEAS,%lu,%ld,%lu\r\n", (unsigned long)m->sequence,
                    (long)m->value, (unsigned long)m->timestamp_ms);
}
