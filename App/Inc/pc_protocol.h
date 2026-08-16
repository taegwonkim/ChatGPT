#ifndef PC_PROTOCOL_H
#define PC_PROTOCOL_H

#include "app_config.h"
#include <stddef.h>

typedef enum { PC_SOURCE_USB, PC_SOURCE_RS485 } PcSource;
typedef void (*PcReply)(PcSource source, const char *text, void *context);
typedef bool (*PcSave)(const AppConfig *config, void *context);
typedef void (*PcStart)(void *context);

typedef struct {
    AppConfig staging;
    PcReply reply;
    PcSave save;
    PcStart start;
    void *context;
} PcProtocol;

void pc_protocol_init(PcProtocol *protocol, const AppConfig *loaded, PcReply reply,
                      PcSave save, PcStart start, void *context);
void pc_protocol_line(PcProtocol *protocol, PcSource source, const char *line);

#endif
