#ifndef ESP32C3_AT_H
#define ESP32C3_AT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_AT_SSID_MAX       32U
#define ESP_AT_PASSWORD_MAX   64U
#define ESP_AT_IPV4_MAX       15U
#define ESP_AT_RX_BUFFER_SIZE 1024U

typedef enum {
    ESP_AT_TCP,
    ESP_AT_UDP
} esp_at_protocol_t;

typedef struct {
    char ssid[ESP_AT_SSID_MAX + 1U];
    char password[ESP_AT_PASSWORD_MAX + 1U];
    bool dhcp;
    char local_ip[ESP_AT_IPV4_MAX + 1U];
    char gateway[ESP_AT_IPV4_MAX + 1U];
    char netmask[ESP_AT_IPV4_MAX + 1U];
    char server_ip[ESP_AT_IPV4_MAX + 1U];
    uint16_t server_port;
    esp_at_protocol_t protocol;
    uint32_t command_timeout_ms;
    uint32_t ap_retry_ms;
    uint32_t server_retry_ms;
} esp_at_config_t;

/* Return the number of bytes transferred. read() may return zero on timeout. */
typedef size_t (*esp_at_write_fn)(const uint8_t *data, size_t length, void *user);
typedef size_t (*esp_at_read_fn)(uint8_t *data, size_t capacity,
                                 uint32_t timeout_ms, void *user);
typedef uint32_t (*esp_at_tick_fn)(void *user);
typedef void (*esp_at_delay_fn)(uint32_t delay_ms, void *user);
typedef void (*esp_at_background_fn)(void *user);

typedef struct {
    esp_at_write_fn write;
    esp_at_read_fn read;
    esp_at_tick_fn tick_ms;
    esp_at_delay_fn delay_ms;
    /* Optional: serviced while a blocking AT response is being awaited. */
    esp_at_background_fn background_process;
    void *user;
} esp_at_io_t;

typedef enum {
    ESP_AT_STATE_RESET,
    ESP_AT_STATE_CONFIGURE,
    ESP_AT_STATE_JOIN_AP,
    ESP_AT_STATE_CONNECT_SERVER,
    ESP_AT_STATE_ONLINE,
    ESP_AT_STATE_WAIT_RESET_RETRY,
    ESP_AT_STATE_WAIT_AP_RETRY,
    ESP_AT_STATE_WAIT_SERVER_RETRY
} esp_at_state_t;

typedef struct {
    esp_at_config_t config;
    esp_at_io_t io;
    esp_at_state_t state;
    uint32_t retry_at_ms;
    bool wifi_connected;
    bool server_connected;
    char rx[ESP_AT_RX_BUFFER_SIZE];
    size_t rx_length;
} esp_at_t;

bool esp_at_init(esp_at_t *ctx, const esp_at_config_t *config,
                 const esp_at_io_t *io);

/* Call repeatedly from the main loop. Configuration commands are blocking only
 * up to command_timeout_ms; retry waiting itself is non-blocking. */
void esp_at_process(esp_at_t *ctx);

bool esp_at_is_online(const esp_at_t *ctx);

/* Send payload with AT+CIPSEND. Call only while online. */
bool esp_at_send(esp_at_t *ctx, const uint8_t *data, size_t length);

#ifdef __cplusplus
}
#endif
#endif
