#include "surge_config.h"

#include <string.h>

static bool ipv4_valid(const char *s) {
  unsigned parts = 0, value = 0, digits = 0;
  if (s == NULL || *s == '\0') return false;
  for (;; ++s) {
    if (*s >= '0' && *s <= '9') {
      value = value * 10u + (unsigned)(*s - '0');
      if (++digits > 3u || value > 255u) return false;
    } else if (*s == '.' || *s == '\0') {
      if (digits == 0u || ++parts > 4u) return false;
      if (*s == '\0') return parts == 4u;
      value = digits = 0u;
    } else return false;
  }
}

static bool terminated_within(const char *s, size_t capacity) {
  return memchr(s, '\0', capacity) != NULL;
}

uint32_t Surge_Crc32(const void *data, size_t size) {
  const uint8_t *p = data;
  uint32_t crc = 0xFFFFFFFFu;
  while (size-- != 0u) {
    crc ^= *p++;
    for (unsigned i = 0; i < 8u; ++i)
      crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
  }
  return ~crc;
}

void SurgeConfig_Default(SurgeConfig *cfg) {
  memset(cfg, 0, sizeof(*cfg));
  cfg->version = SURGE_CONFIG_VERSION;
  cfg->server_port = 50001u;
  cfg->dhcp = true;
  strcpy(cfg->server_ip, "192.168.1.10");
  SurgeConfig_UpdateCrc(cfg);
}

bool SurgeConfig_Validate(const SurgeConfig *cfg) {
  if (cfg == NULL || cfg->version != SURGE_CONFIG_VERSION) return false;
  if (!terminated_within(cfg->ssid, sizeof(cfg->ssid)) ||
      !terminated_within(cfg->password, sizeof(cfg->password)))
    return false;
  if (!ipv4_valid(cfg->server_ip) || cfg->server_port == 0u) return false;
  return cfg->dhcp || (ipv4_valid(cfg->module_ip) && ipv4_valid(cfg->gateway) &&
                       ipv4_valid(cfg->netmask));
}

void SurgeConfig_UpdateCrc(SurgeConfig *cfg) {
  cfg->crc32 = Surge_Crc32(cfg, offsetof(SurgeConfig, crc32));
}

bool SurgeConfig_CheckCrc(const SurgeConfig *cfg) {
  return cfg != NULL && cfg->crc32 == Surge_Crc32(cfg, offsetof(SurgeConfig, crc32));
}
