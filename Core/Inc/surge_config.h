#ifndef SURGE_CONFIG_H
#define SURGE_CONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SURGE_CONFIG_VERSION 1u
#define SURGE_SSID_MAX 32u
#define SURGE_PASSWORD_MAX 63u

typedef struct {
  uint32_t version;
  uint32_t sequence;
  char ssid[SURGE_SSID_MAX + 1u];
  char password[SURGE_PASSWORD_MAX + 1u];
  char server_ip[16];
  uint16_t server_port;
  bool dhcp;
  char module_ip[16];
  char gateway[16];
  char netmask[16];
  uint32_t crc32;
} SurgeConfig;

void SurgeConfig_Default(SurgeConfig *cfg);
bool SurgeConfig_Validate(const SurgeConfig *cfg);
uint32_t Surge_Crc32(const void *data, size_t size);
void SurgeConfig_UpdateCrc(SurgeConfig *cfg);
bool SurgeConfig_CheckCrc(const SurgeConfig *cfg);

#endif
