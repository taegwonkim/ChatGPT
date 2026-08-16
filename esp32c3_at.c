#include "esp32c3_at.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static bool time_reached(uint32_t now, uint32_t target)
{
    return (int32_t)(now - target) >= 0;
}

static void clear_rx(esp_at_t *ctx)
{
    ctx->rx_length = 0U;
    ctx->rx[0] = '\0';
}

static bool append_rx(esp_at_t *ctx, const uint8_t *data, size_t length)
{
    if (length >= sizeof(ctx->rx)) {
        data += length - (sizeof(ctx->rx) - 1U);
        length = sizeof(ctx->rx) - 1U;
        clear_rx(ctx);
    } else if (ctx->rx_length + length >= sizeof(ctx->rx)) {
        size_t discard = ctx->rx_length + length - sizeof(ctx->rx) + 1U;
        memmove(ctx->rx, ctx->rx + discard, ctx->rx_length - discard);
        ctx->rx_length -= discard;
    }
    memcpy(ctx->rx + ctx->rx_length, data, length);
    ctx->rx_length += length;
    ctx->rx[ctx->rx_length] = '\0';
    return true;
}

static bool wait_for(esp_at_t *ctx, const char *success1, const char *success2,
                     uint32_t timeout_ms)
{
    uint32_t start = ctx->io.tick_ms(ctx->io.user);
    uint8_t chunk[64];

    while ((uint32_t)(ctx->io.tick_ms(ctx->io.user) - start) < timeout_ms) {
        size_t count = ctx->io.read(chunk, sizeof(chunk), 20U, ctx->io.user);
        if (count != 0U) {
            append_rx(ctx, chunk, count);
            if (strstr(ctx->rx, "ERROR") != NULL ||
                strstr(ctx->rx, "FAIL") != NULL) {
                return false;
            }
            if ((success1 != NULL && strstr(ctx->rx, success1) != NULL) ||
                (success2 != NULL && strstr(ctx->rx, success2) != NULL)) {
                return true;
            }
        }
    }
    return false;
}

static bool command(esp_at_t *ctx, const char *success1, const char *success2,
                    const char *format, ...)
{
    char line[256];
    va_list args;
    int length;

    va_start(args, format);
    length = vsnprintf(line, sizeof(line) - 2U, format, args);
    va_end(args);
    if (length < 0 || (size_t)length >= sizeof(line) - 2U) {
        return false;
    }
    line[length++] = '\r';
    line[length++] = '\n';
    clear_rx(ctx);
    if (ctx->io.write((const uint8_t *)line, (size_t)length, ctx->io.user) !=
        (size_t)length) {
        return false;
    }
    return wait_for(ctx, success1, success2, ctx->config.command_timeout_ms);
}

static bool valid_string(const char *value, size_t capacity)
{
    return value != NULL && memchr(value, '\0', capacity) != NULL;
}

bool esp_at_init(esp_at_t *ctx, const esp_at_config_t *config,
                 const esp_at_io_t *io)
{
    if (ctx == NULL || config == NULL || io == NULL || io->write == NULL ||
        io->read == NULL || io->tick_ms == NULL || io->delay_ms == NULL ||
        !valid_string(config->ssid, sizeof(config->ssid)) ||
        !valid_string(config->password, sizeof(config->password)) ||
        !valid_string(config->server_ip, sizeof(config->server_ip)) ||
        config->ssid[0] == '\0' || config->server_ip[0] == '\0' ||
        config->server_port == 0U || config->command_timeout_ms == 0U) {
        return false;
    }
    if (!config->dhcp &&
        (!valid_string(config->local_ip, sizeof(config->local_ip)) ||
         !valid_string(config->gateway, sizeof(config->gateway)) ||
         !valid_string(config->netmask, sizeof(config->netmask)) ||
         config->local_ip[0] == '\0' || config->gateway[0] == '\0' ||
         config->netmask[0] == '\0')) {
        return false;
    }
    memset(ctx, 0, sizeof(*ctx));
    ctx->config = *config;
    ctx->io = *io;
    ctx->state = ESP_AT_STATE_RESET;
    return true;
}

static bool configure(esp_at_t *ctx)
{
    if (!command(ctx, "OK", NULL, "ATE0") ||
        !command(ctx, "OK", NULL, "AT+CWMODE=1") ||
        !command(ctx, "OK", NULL, "AT+CIPMUX=0")) {
        return false;
    }
    if (ctx->config.dhcp) {
        return command(ctx, "OK", NULL, "AT+CWDHCP=1,1");
    }
    return command(ctx, "OK", NULL, "AT+CWDHCP=1,0") &&
           command(ctx, "OK", NULL, "AT+CIPSTA=\"%s\",\"%s\",\"%s\"",
                   ctx->config.local_ip, ctx->config.gateway,
                   ctx->config.netmask);
}

static bool join_ap(esp_at_t *ctx)
{
    return command(ctx, "WIFI GOT IP", "OK", "AT+CWJAP=\"%s\",\"%s\"",
                   ctx->config.ssid, ctx->config.password);
}

static bool connect_server(esp_at_t *ctx)
{
    const char *protocol = ctx->config.protocol == ESP_AT_UDP ? "UDP" : "TCP";
    return command(ctx, "CONNECT", "ALREADY CONNECTED",
                   "AT+CIPSTART=\"%s\",\"%s\",%u", protocol,
                   ctx->config.server_ip, (unsigned int)ctx->config.server_port);
}

static void poll_events(esp_at_t *ctx)
{
    uint8_t chunk[64];
    size_t count = ctx->io.read(chunk, sizeof(chunk), 0U, ctx->io.user);
    if (count == 0U) {
        return;
    }
    append_rx(ctx, chunk, count);
    if (strstr(ctx->rx, "WIFI DISCONNECT") != NULL) {
        ctx->wifi_connected = false;
        ctx->server_connected = false;
        ctx->state = ESP_AT_STATE_WAIT_AP_RETRY;
        ctx->retry_at_ms = ctx->io.tick_ms(ctx->io.user) + ctx->config.ap_retry_ms;
        clear_rx(ctx);
    } else if (strstr(ctx->rx, "CLOSED") != NULL) {
        ctx->server_connected = false;
        ctx->state = ESP_AT_STATE_WAIT_SERVER_RETRY;
        ctx->retry_at_ms = ctx->io.tick_ms(ctx->io.user) +
                           ctx->config.server_retry_ms;
        clear_rx(ctx);
    }
}

void esp_at_process(esp_at_t *ctx)
{
    uint32_t now;
    if (ctx == NULL) {
        return;
    }
    now = ctx->io.tick_ms(ctx->io.user);
    switch (ctx->state) {
    case ESP_AT_STATE_RESET:
        (void)command(ctx, "ready", "OK", "AT+RST");
        ctx->state = ESP_AT_STATE_CONFIGURE;
        break;
    case ESP_AT_STATE_CONFIGURE:
        if (configure(ctx)) {
            ctx->state = ESP_AT_STATE_JOIN_AP;
        } else {
            ctx->state = ESP_AT_STATE_RESET;
            ctx->io.delay_ms(1000U, ctx->io.user);
        }
        break;
    case ESP_AT_STATE_JOIN_AP:
        if (join_ap(ctx)) {
            ctx->wifi_connected = true;
            ctx->state = ESP_AT_STATE_CONNECT_SERVER;
        } else {
            ctx->state = ESP_AT_STATE_WAIT_AP_RETRY;
            ctx->retry_at_ms = now + ctx->config.ap_retry_ms;
        }
        break;
    case ESP_AT_STATE_CONNECT_SERVER:
        if (connect_server(ctx)) {
            ctx->server_connected = true;
            clear_rx(ctx);
            ctx->state = ESP_AT_STATE_ONLINE;
        } else {
            ctx->state = ESP_AT_STATE_WAIT_SERVER_RETRY;
            ctx->retry_at_ms = now + ctx->config.server_retry_ms;
        }
        break;
    case ESP_AT_STATE_ONLINE:
        poll_events(ctx);
        break;
    case ESP_AT_STATE_WAIT_AP_RETRY:
        if (time_reached(now, ctx->retry_at_ms)) {
            ctx->state = ESP_AT_STATE_JOIN_AP;
        }
        break;
    case ESP_AT_STATE_WAIT_SERVER_RETRY:
        poll_events(ctx);
        if (ctx->state == ESP_AT_STATE_WAIT_SERVER_RETRY &&
            time_reached(now, ctx->retry_at_ms)) {
            ctx->state = ESP_AT_STATE_CONNECT_SERVER;
        }
        break;
    default:
        ctx->state = ESP_AT_STATE_RESET;
        break;
    }
}

bool esp_at_is_online(const esp_at_t *ctx)
{
    return ctx != NULL && ctx->wifi_connected && ctx->server_connected &&
           ctx->state == ESP_AT_STATE_ONLINE;
}

bool esp_at_send(esp_at_t *ctx, const uint8_t *data, size_t length)
{
    char line[40];
    int line_length;
    if (!esp_at_is_online(ctx) || data == NULL || length == 0U) {
        return false;
    }
    line_length = snprintf(line, sizeof(line), "AT+CIPSEND=%u\r\n",
                           (unsigned int)length);
    clear_rx(ctx);
    if (line_length <= 0 ||
        ctx->io.write((const uint8_t *)line, (size_t)line_length, ctx->io.user) !=
            (size_t)line_length ||
        !wait_for(ctx, ">", NULL, ctx->config.command_timeout_ms) ||
        ctx->io.write(data, length, ctx->io.user) != length ||
        !wait_for(ctx, "SEND OK", NULL, ctx->config.command_timeout_ms)) {
        ctx->server_connected = false;
        ctx->state = ESP_AT_STATE_WAIT_SERVER_RETRY;
        ctx->retry_at_ms = ctx->io.tick_ms(ctx->io.user) +
                           ctx->config.server_retry_ms;
        return false;
    }
    clear_rx(ctx);
    return true;
}
