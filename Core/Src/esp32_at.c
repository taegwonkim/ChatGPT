#include "esp32_at.h"

#include <stdio.h>
#include <string.h>

#define ESP_RX_SLICE_MS 20U

static bool elapsed(uint32_t start, uint32_t timeout)
{
    return (uint32_t)(HAL_GetTick() - start) >= timeout;
}

static void drain_uart(Esp32At *dev)
{
    uint8_t byte;
    while (HAL_UART_Receive(dev->uart, &byte, 1U, 1U) == HAL_OK) {
        /* Discard stale replies and unsolicited result codes. */
    }
}

static Esp32AtStatus collect(Esp32At *dev, uint32_t timeout_ms,
                             const char *success_token)
{
    uint32_t start = HAL_GetTick();
    uint8_t byte;

    dev->response_length = 0U;
    dev->response[0] = '\0';
    while (!elapsed(start, timeout_ms)) {
        HAL_StatusTypeDef hal = HAL_UART_Receive(dev->uart, &byte, 1U,
                                                 ESP_RX_SLICE_MS);
        if (hal == HAL_OK) {
            if (dev->response_length + 1U >= sizeof(dev->response)) {
                dev->response[sizeof(dev->response) - 1U] = '\0';
                return ESP32_AT_OVERFLOW;
            }
            dev->response[dev->response_length++] = (char)byte;
            dev->response[dev->response_length] = '\0';

            if (strstr(dev->response, "\r\nERROR\r\n") != NULL ||
                strstr(dev->response, "\r\nFAIL\r\n") != NULL) {
                return ESP32_AT_ERROR;
            }
            if (strstr(dev->response, success_token) != NULL) {
                return ESP32_AT_OK;
            }
        } else if (hal != HAL_TIMEOUT) {
            return ESP32_AT_HAL_ERROR;
        }
    }
    return ESP32_AT_TIMEOUT;
}

static Esp32AtStatus send_and_wait(Esp32At *dev, const char *command,
                                   uint32_t timeout_ms,
                                   const char *success_token)
{
    char tx[256];
    int length;

    if (dev == NULL || dev->uart == NULL || command == NULL) {
        return ESP32_AT_BAD_ARGUMENT;
    }
    length = snprintf(tx, sizeof(tx), "%s\r\n", command);
    if (length < 0 || (size_t)length >= sizeof(tx)) {
        return ESP32_AT_BAD_ARGUMENT;
    }
    drain_uart(dev);
    if (HAL_UART_Transmit(dev->uart, (uint8_t *)tx, (uint16_t)length,
                          1000U) != HAL_OK) {
        return ESP32_AT_HAL_ERROR;
    }
    return collect(dev, timeout_ms, success_token);
}

static bool safe_at_string(const char *value)
{
    if (value == NULL || *value == '\0') {
        return false;
    }
    /* Quoting/backslash escaping varies between AT firmware versions. Reject
       characters which could change the command instead of emitting bad AT. */
    return strchr(value, '"') == NULL && strchr(value, '\\') == NULL &&
           strchr(value, '\r') == NULL && strchr(value, '\n') == NULL;
}

void Esp32At_Init(Esp32At *dev, UART_HandleTypeDef *uart)
{
    if (dev != NULL) {
        memset(dev, 0, sizeof(*dev));
        dev->uart = uart;
    }
}

Esp32AtStatus Esp32At_Command(Esp32At *dev, const char *command,
                              uint32_t timeout_ms)
{
    return send_and_wait(dev, command, timeout_ms, "\r\nOK\r\n");
}

Esp32AtStatus Esp32At_Begin(Esp32At *dev, char mac[ESP32_AT_MAC_STRING_SIZE])
{
    const char *prefix = "+CIPSTAMAC:\"";
    char *begin;
    char *end;
    Esp32AtStatus status;

    if (dev == NULL || mac == NULL) {
        return ESP32_AT_BAD_ARGUMENT;
    }
    status = Esp32At_Command(dev, "AT", 1000U);
    if (status != ESP32_AT_OK) return status;
    status = Esp32At_Command(dev, "ATE0", 1000U);
    if (status != ESP32_AT_OK) return status;
    /* AT+RST first answers OK and later emits "ready".  Waiting for ready
       prevents the following command from being sent while the module boots. */
    status = send_and_wait(dev, "AT+RST", 5000U, "ready\r\n");
    if (status != ESP32_AT_OK) return status;
    status = Esp32At_Command(dev, "AT", 1000U);
    if (status != ESP32_AT_OK) return status;
    status = Esp32At_Command(dev, "ATE0", 1000U);
    if (status != ESP32_AT_OK) return status;
    status = Esp32At_Command(dev, "AT+CWMODE=1", 2000U);
    if (status != ESP32_AT_OK) return status;
    status = Esp32At_Command(dev, "AT+CIPMUX=0", 1000U);
    if (status != ESP32_AT_OK) return status;
    status = Esp32At_Command(dev, "AT+CIPSTAMAC?", 1000U);
    if (status != ESP32_AT_OK) return status;

    begin = strstr(dev->response, prefix);
    if (begin == NULL) return ESP32_AT_ERROR;
    begin += strlen(prefix);
    end = strchr(begin, '"');
    if (end == NULL || (size_t)(end - begin) != ESP32_AT_MAC_STRING_SIZE - 1U) {
        return ESP32_AT_ERROR;
    }
    memcpy(mac, begin, ESP32_AT_MAC_STRING_SIZE - 1U);
    mac[ESP32_AT_MAC_STRING_SIZE - 1U] = '\0';
    return ESP32_AT_OK;
}

Esp32AtStatus Esp32At_SetDhcp(Esp32At *dev, bool enable,
                              const char *ip, const char *gateway,
                              const char *netmask)
{
    char command[160];
    int length;
    Esp32AtStatus status;

    status = Esp32At_Command(dev, enable ? "AT+CWDHCP=1,1" : "AT+CWDHCP=1,0",
                             2000U);
    if (status != ESP32_AT_OK || enable) return status;
    if (!safe_at_string(ip) || !safe_at_string(gateway) ||
        !safe_at_string(netmask)) {
        return ESP32_AT_BAD_ARGUMENT;
    }
    length = snprintf(command, sizeof(command), "AT+CIPSTA=\"%s\",\"%s\",\"%s\"",
                      ip, gateway, netmask);
    if (length < 0 || (size_t)length >= sizeof(command)) {
        return ESP32_AT_BAD_ARGUMENT;
    }
    return Esp32At_Command(dev, command, 3000U);
}

Esp32AtStatus Esp32At_JoinAp(Esp32At *dev, const char *ssid,
                             const char *password)
{
    char command[192];
    int length;
    if (!safe_at_string(ssid) || !safe_at_string(password)) {
        return ESP32_AT_BAD_ARGUMENT;
    }
    length = snprintf(command, sizeof(command), "AT+CWJAP=\"%s\",\"%s\"",
                      ssid, password);
    if (length < 0 || (size_t)length >= sizeof(command)) {
        return ESP32_AT_BAD_ARGUMENT;
    }
    return Esp32At_Command(dev, command, 30000U);
}

Esp32AtStatus Esp32At_OpenTcp(Esp32At *dev, const char *host, uint16_t port)
{
    char command[160];
    int length;
    if (!safe_at_string(host) || port == 0U) return ESP32_AT_BAD_ARGUMENT;
    length = snprintf(command, sizeof(command), "AT+CIPSTART=\"TCP\",\"%s\",%u",
                      host, (unsigned int)port);
    if (length < 0 || (size_t)length >= sizeof(command)) {
        return ESP32_AT_BAD_ARGUMENT;
    }
    return Esp32At_Command(dev, command, 15000U);
}

bool Esp32At_IsApConnected(Esp32At *dev)
{
    if (Esp32At_Command(dev, "AT+CWSTATE?", 2000U) != ESP32_AT_OK) return false;
    return strstr(dev->response, "+CWSTATE:2,") != NULL;
}

bool Esp32At_IsTcpConnected(Esp32At *dev)
{
    if (Esp32At_Command(dev, "AT+CIPSTATUS", 2000U) != ESP32_AT_OK) return false;
    return strstr(dev->response, "STATUS:3") != NULL;
}

Esp32AtStatus Esp32At_CloseTcp(Esp32At *dev)
{
    return Esp32At_Command(dev, "AT+CIPCLOSE", 3000U);
}
