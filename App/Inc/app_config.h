#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define APP_SSID_MAX 32u
#define APP_PASSWORD_MAX 64u
#define APP_IPV4_TEXT_MAX 15u
#define APP_DEFAULT_SERVER_PORT 50001u
#define APP_CONFIG_SCHEMA 1u

typedef struct {
    uint16_t schema;
    bool dhcp;
    uint16_t server_port;
    char ssid[APP_SSID_MAX + 1u];
    char password[APP_PASSWORD_MAX + 1u];
    char server_ip[APP_IPV4_TEXT_MAX + 1u];
    char module_ip[APP_IPV4_TEXT_MAX + 1u];
    char gateway[APP_IPV4_TEXT_MAX + 1u];
    char netmask[APP_IPV4_TEXT_MAX + 1u];
} AppConfig;

void app_config_defaults(AppConfig *config);
bool app_config_set(AppConfig *config, const char *key, const char *value);
bool app_config_validate(const AppConfig *config, char *reason, size_t reason_size);
uint32_t app_crc32(const void *data, size_t size);

#endif
