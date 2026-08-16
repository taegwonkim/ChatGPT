#include "app_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool copy_text(char *dst, size_t capacity, const char *src) {
    const size_t length = strlen(src);
    if (length >= capacity) return false;
    memcpy(dst, src, length + 1u);
    return true;
}

static bool valid_ipv4(const char *text) {
    unsigned a, b, c, d;
    char tail;
    if (sscanf(text, "%u.%u.%u.%u%c", &a, &b, &c, &d, &tail) != 4) return false;
    return a <= 255u && b <= 255u && c <= 255u && d <= 255u;
}

void app_config_defaults(AppConfig *config) {
    memset(config, 0, sizeof(*config));
    config->schema = APP_CONFIG_SCHEMA;
    config->dhcp = true;
    config->server_port = APP_DEFAULT_SERVER_PORT;
}

bool app_config_set(AppConfig *config, const char *key, const char *value) {
    if (strcmp(key, "SSID") == 0) return copy_text(config->ssid, sizeof(config->ssid), value);
    if (strcmp(key, "PASSWORD") == 0) return copy_text(config->password, sizeof(config->password), value);
    if (strcmp(key, "SERVER_IP") == 0) return copy_text(config->server_ip, sizeof(config->server_ip), value);
    if (strcmp(key, "MODULE_IP") == 0) return copy_text(config->module_ip, sizeof(config->module_ip), value);
    if (strcmp(key, "GATEWAY") == 0) return copy_text(config->gateway, sizeof(config->gateway), value);
    if (strcmp(key, "NETMASK") == 0) return copy_text(config->netmask, sizeof(config->netmask), value);
    if (strcmp(key, "DHCP") == 0) {
        if (strcmp(value, "0") != 0 && strcmp(value, "1") != 0) return false;
        config->dhcp = value[0] == '1';
        return true;
    }
    if (strcmp(key, "SERVER_PORT") == 0) {
        char *end;
        const unsigned long port = strtoul(value, &end, 10);
        if (*value == '\0' || *end != '\0' || port == 0u || port > 65535u) return false;
        config->server_port = (uint16_t)port;
        return true;
    }
    return false;
}

bool app_config_validate(const AppConfig *config, char *reason, size_t reason_size) {
#define REJECT(message) do { if (reason_size) snprintf(reason, reason_size, "%s", message); return false; } while (0)
    if (config->schema != APP_CONFIG_SCHEMA) REJECT("schema");
    if (config->ssid[0] == '\0') REJECT("ssid");
    if (!valid_ipv4(config->server_ip)) REJECT("server-ip");
    if (config->server_port == 0u) REJECT("server-port");
    if (!config->dhcp && (!valid_ipv4(config->module_ip) ||
                         !valid_ipv4(config->gateway) || !valid_ipv4(config->netmask))) {
        REJECT("static-ip");
    }
    if (reason_size) reason[0] = '\0';
    return true;
#undef REJECT
}

uint32_t app_crc32(const void *data, size_t size) {
    const uint8_t *bytes = data;
    uint32_t crc = UINT32_C(0xFFFFFFFF);
    while (size-- != 0u) {
        crc ^= *bytes++;
        for (unsigned bit = 0; bit < 8u; ++bit)
            crc = (crc >> 1u) ^ (UINT32_C(0xEDB88320) & (uint32_t)-(int32_t)(crc & 1u));
    }
    return ~crc;
}
