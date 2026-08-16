#include "pc_protocol.h"
#include <stdio.h>
#include <string.h>

static void emit_config(PcProtocol *p, PcSource source) {
    char output[384];
    snprintf(output, sizeof(output),
             "SSID=%s\r\nPASSWORD=********\r\nSERVER_IP=%s\r\nSERVER_PORT=%u\r\n"
             "DHCP=%u\r\nMODULE_IP=%s\r\nGATEWAY=%s\r\nNETMASK=%s\r\nOK\r\n",
             p->staging.ssid, p->staging.server_ip, p->staging.server_port,
             p->staging.dhcp ? 1u : 0u, p->staging.module_ip,
             p->staging.gateway, p->staging.netmask);
    p->reply(source, output, p->context);
}

void pc_protocol_init(PcProtocol *p, const AppConfig *loaded, PcReply reply,
                      PcSave save, PcStart start, void *context) {
    p->staging = *loaded;
    p->reply = reply;
    p->save = save;
    p->start = start;
    p->context = context;
}

void pc_protocol_line(PcProtocol *p, PcSource source, const char *line) {
    char copy[256], *equals;
    size_t length = strcspn(line, "\r\n");
    if (length >= sizeof(copy)) { p->reply(source, "ERR line-too-long\r\n", p->context); return; }
    memcpy(copy, line, length); copy[length] = '\0';
    if (strcmp(copy, "GET CONFIG") == 0) { emit_config(p, source); return; }
    if (strcmp(copy, "START") == 0) { p->start(p->context); p->reply(source, "OK\r\n", p->context); return; }
    if (strcmp(copy, "SAVE") == 0) {
        char reason[24];
        if (!app_config_validate(&p->staging, reason, sizeof(reason))) {
            char response[48]; snprintf(response, sizeof(response), "ERR %s\r\n", reason);
            p->reply(source, response, p->context); return;
        }
        p->reply(source, p->save(&p->staging, p->context) ? "OK\r\n" : "ERR flash\r\n", p->context);
        return;
    }
    if (strncmp(copy, "SET ", 4u) != 0 || (equals = strchr(copy + 4, '=')) == NULL) {
        p->reply(source, "ERR command\r\n", p->context); return;
    }
    *equals++ = '\0';
    p->reply(source, app_config_set(&p->staging, copy + 4, equals) ? "OK\r\n" : "ERR value\r\n", p->context);
}
