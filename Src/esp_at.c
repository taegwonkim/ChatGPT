#include "esp_at.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool elapsed(uint32_t started, uint32_t timeout_ms)
{
    return (uint32_t)(HAL_GetTick() - started) >= timeout_ms;
}

static bool unsafe_quoted_argument(const char *text)
{
    if (text == NULL || *text == '\0') {
        return true;
    }
    return strchr(text, '"') != NULL || strchr(text, '\r') != NULL ||
           strchr(text, '\n') != NULL;
}

static void finish_line(ESP_AT_Handle *handle)
{
    if (handle->line_length == 0U) {
        return;
    }

    handle->line[handle->line_length] = '\0';
    if (strcmp(handle->line, "OK") == 0 ||
        strcmp(handle->line, "SEND OK") == 0) {
        handle->response_ok = true;
    } else if (strcmp(handle->line, "ERROR") == 0 ||
               strcmp(handle->line, "FAIL") == 0 ||
               strncmp(handle->line, "busy ", 5U) == 0 ||
               strcmp(handle->line, "SEND FAIL") == 0) {
        handle->response_error = true;
    }

    if (handle->line_callback != NULL) {
        handle->line_callback(handle->line, handle->callback_context);
    }
    handle->line_length = 0U;
}

static bool begin_ipd_payload(ESP_AT_Handle *handle)
{
    char *comma;
    char *end;
    unsigned long length;

    if (handle->line_length < 6U ||
        strncmp(handle->line, "+IPD,", 5U) != 0) {
        return false;
    }

    handle->line[handle->line_length] = '\0';
    comma = strrchr(handle->line, ',');
    if (comma == NULL) {
        return false;
    }
    if (comma[1] < '0' || comma[1] > '9') {
        return false;
    }
    length = strtoul(comma + 1, &end, 10);
    if (end == comma + 1 || *end != '\0' || length > SIZE_MAX) {
        return false;
    }

    handle->ipd_remaining = (size_t)length;
    handle->ipd_chunk_length = 0U;
    handle->line_length = 0U;
    return true;
}

static void consume_byte(ESP_AT_Handle *handle, uint8_t byte)
{
    if (handle->ipd_remaining > 0U) {
        handle->ipd_chunk[handle->ipd_chunk_length++] = byte;
        handle->ipd_remaining--;
        if (handle->ipd_chunk_length == ESP_AT_IPD_CHUNK_SIZE ||
            handle->ipd_remaining == 0U) {
            if (handle->data_callback != NULL) {
                handle->data_callback(handle->ipd_chunk,
                                      handle->ipd_chunk_length,
                                      handle->callback_context);
            }
            handle->ipd_chunk_length = 0U;
        }
        return;
    }

    if (byte == '>') {
        handle->prompt_received = true;
        return;
    }
    if (byte == '\r') {
        return;
    }
    if (byte == '\n') {
        finish_line(handle);
        return;
    }
    if (byte == ':' && begin_ipd_payload(handle)) {
        return;
    }
    if (handle->line_length < (ESP_AT_LINE_BUFFER_SIZE - 1U)) {
        handle->line[handle->line_length++] = (char)byte;
    } else {
        handle->line_length = 0U;
        handle->response_error = true;
    }
}

static ESP_AT_Status wait_response(ESP_AT_Handle *handle, uint32_t timeout_ms,
                                   bool wait_prompt)
{
    const uint32_t started = HAL_GetTick();

    while (!elapsed(started, timeout_ms)) {
        ESP_AT_Status status = ESP_AT_Process(handle);
        if (status == ESP_AT_OVERFLOW) {
            handle->command_active = false;
            return status;
        }
        if (handle->response_error) {
            handle->command_active = false;
            return ESP_AT_ERROR;
        }
        if ((wait_prompt && handle->prompt_received) ||
            (!wait_prompt && handle->response_ok)) {
            handle->command_active = false;
            return ESP_AT_OK;
        }
    }
    handle->command_active = false;
    return ESP_AT_TIMEOUT;
}

static ESP_AT_Status transmit(ESP_AT_Handle *handle, const uint8_t *data,
                              size_t length)
{
    if (length > UINT16_MAX) {
        return ESP_AT_INVALID_ARG;
    }
    return HAL_UART_Transmit(handle->uart, (uint8_t *)data, (uint16_t)length,
                             1000U) == HAL_OK
               ? ESP_AT_OK
               : ESP_AT_HAL_ERROR;
}

void ESP_AT_Init(ESP_AT_Handle *handle, UART_HandleTypeDef *uart,
                 ESP_AT_LineCallback line_callback,
                 ESP_AT_DataCallback data_callback, void *context)
{
    if (handle == NULL) {
        return;
    }
    memset(handle, 0, sizeof(*handle));
    handle->uart = uart;
    handle->line_callback = line_callback;
    handle->data_callback = data_callback;
    handle->callback_context = context;
}

ESP_AT_Status ESP_AT_StartReceive(ESP_AT_Handle *handle)
{
    if (handle == NULL || handle->uart == NULL) {
        return ESP_AT_INVALID_ARG;
    }
    return HAL_UART_Receive_IT(handle->uart, &handle->rx_byte, 1U) == HAL_OK
               ? ESP_AT_OK
               : ESP_AT_HAL_ERROR;
}

void ESP_AT_RxCpltCallback(ESP_AT_Handle *handle, UART_HandleTypeDef *uart)
{
    uint16_t next;

    if (handle == NULL || uart != handle->uart) {
        return;
    }
    next = (uint16_t)((handle->rx_head + 1U) % ESP_AT_RX_BUFFER_SIZE);
    if (next == handle->rx_tail) {
        handle->rx_overflow = true;
    } else {
        handle->rx_buffer[handle->rx_head] = handle->rx_byte;
        handle->rx_head = next;
    }
    (void)HAL_UART_Receive_IT(handle->uart, &handle->rx_byte, 1U);
}

void ESP_AT_ErrorCallback(ESP_AT_Handle *handle, UART_HandleTypeDef *uart)
{
    if (handle != NULL && uart == handle->uart) {
        (void)HAL_UART_Receive_IT(handle->uart, &handle->rx_byte, 1U);
    }
}

ESP_AT_Status ESP_AT_Process(ESP_AT_Handle *handle)
{
    if (handle == NULL || handle->uart == NULL) {
        return ESP_AT_INVALID_ARG;
    }
    if (handle->rx_overflow) {
        handle->rx_overflow = false;
        return ESP_AT_OVERFLOW;
    }
    while (handle->rx_tail != handle->rx_head) {
        uint8_t byte = handle->rx_buffer[handle->rx_tail];
        handle->rx_tail =
            (uint16_t)((handle->rx_tail + 1U) % ESP_AT_RX_BUFFER_SIZE);
        consume_byte(handle, byte);
    }
    return ESP_AT_OK;
}

ESP_AT_Status ESP_AT_Command(ESP_AT_Handle *handle, const char *command,
                             uint32_t timeout_ms)
{
    ESP_AT_Status status;
    size_t length;

    if (handle == NULL || handle->uart == NULL || command == NULL ||
        timeout_ms == 0U || strchr(command, '\r') != NULL ||
        strchr(command, '\n') != NULL) {
        return ESP_AT_INVALID_ARG;
    }
    if (handle->command_active) {
        return ESP_AT_BUSY;
    }
    /* Consume unsolicited/stale input before arming this command's result. */
    status = ESP_AT_Process(handle);
    if (status != ESP_AT_OK) {
        return status;
    }
    length = strlen(command);
    handle->command_active = true;
    handle->response_ok = false;
    handle->response_error = false;
    handle->prompt_received = false;
    status = transmit(handle, (const uint8_t *)command, length);
    if (status == ESP_AT_OK) {
        status = transmit(handle, (const uint8_t *)"\r\n", 2U);
    }
    if (status != ESP_AT_OK) {
        handle->command_active = false;
        return status;
    }
    return wait_response(handle, timeout_ms, false);
}

ESP_AT_Status ESP_AT_Reset(ESP_AT_Handle *handle, uint32_t timeout_ms)
{
    return ESP_AT_Command(handle, "AT+RST", timeout_ms);
}

ESP_AT_Status ESP_AT_SetStationMode(ESP_AT_Handle *handle,
                                    uint32_t timeout_ms)
{
    return ESP_AT_Command(handle, "AT+CWMODE=1", timeout_ms);
}

ESP_AT_Status ESP_AT_JoinAP(ESP_AT_Handle *handle, const char *ssid,
                            const char *password, uint32_t timeout_ms)
{
    char command[ESP_AT_LINE_BUFFER_SIZE];
    int count;

    if (unsafe_quoted_argument(ssid) || unsafe_quoted_argument(password)) {
        return ESP_AT_INVALID_ARG;
    }
    count = snprintf(command, sizeof(command), "AT+CWJAP=\"%s\",\"%s\"",
                     ssid, password);
    if (count < 0 || (size_t)count >= sizeof(command)) {
        return ESP_AT_INVALID_ARG;
    }
    return ESP_AT_Command(handle, command, timeout_ms);
}

ESP_AT_Status ESP_AT_TcpConnect(ESP_AT_Handle *handle, const char *host,
                                uint16_t port, uint32_t timeout_ms)
{
    char command[ESP_AT_LINE_BUFFER_SIZE];
    int count;

    if (unsafe_quoted_argument(host) || port == 0U) {
        return ESP_AT_INVALID_ARG;
    }
    count = snprintf(command, sizeof(command),
                     "AT+CIPSTART=\"TCP\",\"%s\",%u", host,
                     (unsigned int)port);
    if (count < 0 || (size_t)count >= sizeof(command)) {
        return ESP_AT_INVALID_ARG;
    }
    return ESP_AT_Command(handle, command, timeout_ms);
}

ESP_AT_Status ESP_AT_Send(ESP_AT_Handle *handle, const uint8_t *data,
                          size_t length, uint32_t timeout_ms)
{
    char command[32];
    int count;
    ESP_AT_Status status;

    if (handle == NULL || handle->uart == NULL || data == NULL || length == 0U ||
        length > UINT16_MAX || timeout_ms == 0U) {
        return ESP_AT_INVALID_ARG;
    }
    if (handle->command_active) {
        return ESP_AT_BUSY;
    }
    status = ESP_AT_Process(handle);
    if (status != ESP_AT_OK) {
        return status;
    }
    count = snprintf(command, sizeof(command), "AT+CIPSEND=%u\r\n",
                     (unsigned int)length);
    if (count < 0 || (size_t)count >= sizeof(command)) {
        return ESP_AT_INVALID_ARG;
    }
    handle->command_active = true;
    handle->response_ok = false;
    handle->response_error = false;
    handle->prompt_received = false;
    status = transmit(handle, (const uint8_t *)command, (size_t)count);
    if (status != ESP_AT_OK) {
        handle->command_active = false;
        return status;
    }
    status = wait_response(handle, timeout_ms, true);
    if (status != ESP_AT_OK) {
        return status;
    }

    handle->command_active = true;
    handle->response_ok = false;
    handle->response_error = false;
    status = transmit(handle, data, length);
    if (status != ESP_AT_OK) {
        handle->command_active = false;
        return status;
    }
    return wait_response(handle, timeout_ms, false);
}

ESP_AT_Status ESP_AT_Close(ESP_AT_Handle *handle, uint32_t timeout_ms)
{
    return ESP_AT_Command(handle, "AT+CIPCLOSE", timeout_ms);
}
