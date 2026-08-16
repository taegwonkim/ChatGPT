#include "app_config.h"
#include "fpga_parser.h"
#include "pc_protocol.h"
#include "wifi_state.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static char reply_text[512];
static void reply(PcSource s, const char *text, void *ctx) { (void)s; (void)ctx; snprintf(reply_text, sizeof(reply_text), "%s", text); }
static bool save(const AppConfig *c, void *ctx) { (void)c; (void)ctx; return true; }
static void start(void *ctx) { *(bool *)ctx = true; }
static void put_le32(unsigned char *p, uint32_t x) { for (int i=0;i<4;i++) p[i]=(unsigned char)(x>>(8*i)); }

int main(void) {
    AppConfig c; app_config_defaults(&c);
    assert(app_config_set(&c, "SSID", "ap"));
    assert(app_config_set(&c, "SERVER_IP", "192.168.1.2"));
    char reason[24]; assert(app_config_validate(&c, reason, sizeof(reason)));
    assert(app_crc32("123456789", 9) == 0xCBF43926u);
    bool started = false; PcProtocol pc;
    pc_protocol_init(&pc, &c, reply, save, start, &started);
    pc_protocol_line(&pc, PC_SOURCE_USB, "SET SERVER_PORT=50001\r\n"); assert(!strcmp(reply_text,"OK\r\n"));
    pc_protocol_line(&pc, PC_SOURCE_USB, "GET CONFIG\r\n"); assert(strstr(reply_text,"PASSWORD=********") != NULL);
    pc_protocol_line(&pc, PC_SOURCE_USB, "START\r\n"); assert(started);
    unsigned char raw[15] = {0xA5,0x5A,3,0,7,0,0,0,1,2,3}; put_le32(raw+11, app_crc32(raw,11));
    FpgaParser parser; FpgaFrame frame; fpga_parser_reset(&parser); bool got=false;
    for (size_t i=0;i<sizeof(raw);i++) got |= fpga_parser_push(&parser,raw[i],&frame);
    assert(got && frame.sequence==7 && frame.length==3 && frame.payload[2]==3);
    assert(wifi_backoff_seconds(0)==1 && wifi_backoff_seconds(5)==30);
    puts("all tests passed");
}
