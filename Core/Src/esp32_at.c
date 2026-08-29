#include "esp32_at.h"

#include <stdio.h>
#include <string.h>

#define COMMAND_TIMEOUT_MS  5000U
#define JOIN_TIMEOUT_MS    20000U
#define SEND_TIMEOUT_MS    10000U
#define BACKOFF_MAX_MS     30000U

typedef enum { TX_IDLE, TX_WAIT_PROMPT, TX_WAIT_RESULT } TxState;

static ESP_AT_Config cfg;
static volatile uint8_t rx_irq_byte;
static volatile uint16_t rx_head;
static volatile uint16_t rx_tail;
static uint8_t rx_ring[ESP_AT_RX_BUFFER_SIZE];
static char line[256];
static size_t line_len;
static size_t ipd_remaining;
static uint8_t ipd_data[ESP_AT_MAX_PAYLOAD];
static size_t ipd_len;
static ESP_AT_State state = ESP_AT_STOPPED;
static uint8_t config_step;
static uint32_t deadline;
static uint32_t retry_at;
static uint32_t backoff_ms = 1000U;
static bool command_pending;
static bool saw_wifi_disconnect;
static TxState tx_state;
static uint8_t tx_data[ESP_AT_MAX_PAYLOAD];
static size_t tx_len;

static bool expired(uint32_t time)
{
    return (int32_t)(HAL_GetTick() - time) >= 0;
}

static void set_state(ESP_AT_State next)
{
    if (state != next) {
        state = next;
        if (cfg.on_state != NULL) {
            cfg.on_state(next);
        }
    }
}

static void arm_rx(void)
{
    (void)HAL_UART_Receive_IT(cfg.uart, (uint8_t *)&rx_irq_byte, 1U);
}

static void write_bytes(const uint8_t *data, size_t length)
{
    (void)HAL_UART_Transmit(cfg.uart, (uint8_t *)data, (uint16_t)length, 1000U);
}

static void command(const char *text, uint32_t timeout_ms)
{
    write_bytes((const uint8_t *)text, strlen(text));
    write_bytes((const uint8_t *)"\r\n", 2U);
    command_pending = true;
    deadline = HAL_GetTick() + timeout_ms;
}

static void retry_later(void)
{
    command_pending = false;
    tx_state = TX_IDLE;
    retry_at = HAL_GetTick() + backoff_ms;
    if (backoff_ms < BACKOFF_MAX_MS / 2U) {
        backoff_ms *= 2U;
    } else {
        backoff_ms = BACKOFF_MAX_MS;
    }
    set_state(ESP_AT_BACKOFF);
}

static void handle_ok(void)
{
    command_pending = false;
    switch (state) {
    case ESP_AT_SYNC:
        config_step = 0U;
        set_state(ESP_AT_CONFIG);
        break;
    case ESP_AT_CONFIG:
        config_step++;
        break;
    case ESP_AT_JOIN_AP:
        set_state(ESP_AT_CONNECT_TCP);
        break;
    case ESP_AT_CONNECT_TCP:
        backoff_ms = 1000U;
        set_state(ESP_AT_ONLINE);
        break;
    default:
        break;
    }
}

static void handle_line(const char *value)
{
    if (strcmp(value, "OK") == 0) {
        handle_ok();
    } else if (strcmp(value, "SEND OK") == 0) {
        tx_state = TX_IDLE;
        command_pending = false;
    } else if (strstr(value, "WIFI DISCONNECT") != NULL) {
        saw_wifi_disconnect = true;
        retry_later();
    } else if (strcmp(value, "CLOSED") == 0 || strstr(value, ",CLOSED") != NULL) {
        retry_later();
    } else if (strcmp(value, "ERROR") == 0 || strcmp(value, "FAIL") == 0 ||
               strstr(value, "link is not valid") != NULL) {
        retry_later();
    } else if (state == ESP_AT_CONNECT_TCP &&
               (strcmp(value, "CONNECT") == 0 || strstr(value, "ALREADY CONNECTED") != NULL)) {
        /* ESP-AT 버전에 따라 CIPSTART는 CONNECT 뒤 OK 또는 ALREADY CONNECTED를 냅니다. */
        if (strstr(value, "ALREADY CONNECTED") != NULL) {
            command_pending = false;
            backoff_ms = 1000U;
            set_state(ESP_AT_ONLINE);
        }
    }
}

static void consume_byte(uint8_t byte)
{
    if (ipd_remaining != 0U) {
        if (ipd_len < sizeof(ipd_data)) {
            ipd_data[ipd_len++] = byte;
        }
        ipd_remaining--;
        if (ipd_remaining == 0U) {
            if (cfg.on_data != NULL) {
                cfg.on_data(ipd_data, ipd_len);
            }
            ipd_len = 0U;
        }
        return;
    }

    if (tx_state == TX_WAIT_PROMPT && byte == '>') {
        write_bytes(tx_data, tx_len);
        tx_state = TX_WAIT_RESULT;
        deadline = HAL_GetTick() + SEND_TIMEOUT_MS;
        line_len = 0U;
        return;
    }

    if (byte == '\r') {
        return;
    }
    if (byte == '\n') {
        if (line_len != 0U) {
            line[line_len] = '\0';
            handle_line(line);
            line_len = 0U;
        }
        return;
    }
    if (line_len < sizeof(line) - 1U) {
        line[line_len++] = (char)byte;
    } else {
        line_len = 0U;
    }

    /* single-connection 형식: +IPD,<len>:data */
    if (byte == ':' && line_len > 6U && strncmp(line, "+IPD,", 5U) == 0) {
        unsigned long length = 0UL;
        if (sscanf(line + 5, "%lu:", &length) == 1 && length > 0UL) {
            ipd_remaining = (size_t)length;
            ipd_len = 0U;
        }
        line_len = 0U;
    }
}

void ESP_AT_Init(const ESP_AT_Config *config)
{
    cfg = *config;
    rx_head = rx_tail = 0U;
    line_len = ipd_remaining = ipd_len = 0U;
    command_pending = false;
    saw_wifi_disconnect = false;
    tx_state = TX_IDLE;
    state = ESP_AT_STOPPED;
    arm_rx();
}

void ESP_AT_Start(void)
{
    backoff_ms = 1000U;
    command_pending = false;
    set_state(ESP_AT_SYNC);
}

void ESP_AT_Process(void)
{
    while (rx_tail != rx_head) {
        uint8_t byte = rx_ring[rx_tail];
        rx_tail = (uint16_t)((rx_tail + 1U) % ESP_AT_RX_BUFFER_SIZE);
        consume_byte(byte);
    }

    if ((command_pending || tx_state != TX_IDLE) && expired(deadline)) {
        retry_later();
        return;
    }

    if (command_pending || tx_state != TX_IDLE) {
        return;
    }

    char cmd[256];
    switch (state) {
    case ESP_AT_SYNC:
        command("AT", COMMAND_TIMEOUT_MS);
        break;
    case ESP_AT_CONFIG:
        if (config_step == 0U) command("ATE0", COMMAND_TIMEOUT_MS);
        else if (config_step == 1U) command("AT+CWMODE=1", COMMAND_TIMEOUT_MS);
        else if (config_step == 2U) command("AT+CIPMUX=0", COMMAND_TIMEOUT_MS);
        else set_state(ESP_AT_JOIN_AP);
        break;
    case ESP_AT_JOIN_AP:
        (void)snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"", cfg.ssid, cfg.password);
        command(cmd, JOIN_TIMEOUT_MS);
        break;
    case ESP_AT_CONNECT_TCP:
        (void)snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",%u",
                       cfg.server_host, (unsigned int)cfg.server_port);
        command(cmd, JOIN_TIMEOUT_MS);
        break;
    case ESP_AT_BACKOFF:
        if (expired(retry_at)) {
            saw_wifi_disconnect = false;
            set_state(ESP_AT_SYNC);
        }
        break;
    default:
        break;
    }
}

bool ESP_AT_Send(const uint8_t *data, size_t length)
{
    char cmd[32];
    if (state != ESP_AT_ONLINE || tx_state != TX_IDLE || data == NULL ||
        length == 0U || length > sizeof(tx_data)) {
        return false;
    }
    memcpy(tx_data, data, length);
    tx_len = length;
    (void)snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%u", (unsigned int)length);
    command(cmd, COMMAND_TIMEOUT_MS);
    tx_state = TX_WAIT_PROMPT;
    return true;
}

bool ESP_AT_IsOnline(void) { return state == ESP_AT_ONLINE; }
ESP_AT_State ESP_AT_GetState(void) { return state; }

void ESP_AT_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == cfg.uart) {
        uint16_t next = (uint16_t)((rx_head + 1U) % ESP_AT_RX_BUFFER_SIZE);
        if (next != rx_tail) {
            rx_ring[rx_head] = rx_irq_byte;
            rx_head = next;
        }
        arm_rx();
    }
}

void ESP_AT_UartErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart == cfg.uart) {
        arm_rx();
    }
}
