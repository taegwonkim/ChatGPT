#include "app_flash.h"
#include <string.h>
#include <stddef.h>

#define CFG_MAGIC 0x43464731UL
#define CFG_VERSION 1U
typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t payload_size;
    uint32_t sequence;
    AppWifiConfig payload;
    uint32_t crc;
} ConfigRecord;

static uint32_t crc32(const void *data, size_t length)
{
    const uint8_t *p = data; uint32_t crc = 0xFFFFFFFFU;
    while (length--) {
        crc ^= *p++;
        for (unsigned i = 0; i < 8; ++i) crc = (crc >> 1) ^ (0xEDB88320U & (0U - (crc & 1U)));
    }
    return ~crc;
}

static bool read_valid(uint32_t address, ConfigRecord *r)
{
    if (!Board_FlashRead(address, r, sizeof *r)) return false;
    return r->magic == CFG_MAGIC && r->version == CFG_VERSION &&
           r->payload_size == sizeof(AppWifiConfig) &&
           r->crc == crc32(r, offsetof(ConfigRecord, crc));
}

bool AppFlash_LoadConfig(AppWifiConfig *cfg)
{
    ConfigRecord a, b; bool av = read_valid(APP_CFG_SLOT_A, &a), bv = read_valid(APP_CFG_SLOT_B, &b);
    if (!av && !bv) return false;
    const ConfigRecord *best = (!bv || (av && (int32_t)(a.sequence - b.sequence) > 0)) ? &a : &b;
    *cfg = best->payload; return true;
}

bool AppFlash_SaveConfig(const AppWifiConfig *cfg)
{
    ConfigRecord a, b, verify; bool av = read_valid(APP_CFG_SLOT_A, &a), bv = read_valid(APP_CFG_SLOT_B, &b);
    uint32_t seq = av ? a.sequence : 0; if (bv && (int32_t)(b.sequence - seq) > 0) seq = b.sequence;
    uint32_t target;
    if (av && !bv) target = APP_CFG_SLOT_B;
    else if (!av && bv) target = APP_CFG_SLOT_A;
    else if (!av) target = APP_CFG_SLOT_A;
    else target = ((int32_t)(a.sequence - b.sequence) > 0) ? APP_CFG_SLOT_B : APP_CFG_SLOT_A;
    ConfigRecord n = {.magic=CFG_MAGIC, .version=CFG_VERSION, .payload_size=sizeof *cfg, .sequence=seq + 1U, .payload=*cfg};
    n.crc = crc32(&n, offsetof(ConfigRecord, crc));
    return Board_FlashEraseSector(target) && Board_FlashProgram(target, &n, sizeof n) &&
           read_valid(target, &verify) && verify.sequence == n.sequence;
}
