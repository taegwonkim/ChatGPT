#include "wifi_app.h"

#include <string.h>

static bool time_reached(uint32_t deadline)
{
    return (int32_t)(HAL_GetTick() - deadline) >= 0;
}

static void retry(WifiApp *app, WifiAppState target)
{
    app->retry_target = target;
    app->next_action_tick = HAL_GetTick() + app->config.retry_ms;
    app->state = WIFI_APP_RETRY_WAIT;
}

void WifiApp_Init(WifiApp *app, UART_HandleTypeDef *uart,
                  const WifiAppConfig *config)
{
    memset(app, 0, sizeof(*app));
    Esp32At_Init(&app->esp, uart);
    app->config = *config;
    app->state = WIFI_APP_INIT;
}

void WifiApp_Process(WifiApp *app)
{
    Esp32AtStatus status;

    switch (app->state) {
    case WIFI_APP_INIT:
        status = Esp32At_Begin(&app->esp, app->mac);
        if (status != ESP32_AT_OK) {
            retry(app, WIFI_APP_INIT);
            break;
        }
        status = Esp32At_SetDhcp(&app->esp, app->config.dhcp_enabled,
                                 app->config.static_ip, app->config.gateway,
                                 app->config.netmask);
        if (status == ESP32_AT_OK) app->state = WIFI_APP_AP_CONNECT;
        else retry(app, WIFI_APP_INIT);
        break;

    case WIFI_APP_AP_CONNECT:
        status = Esp32At_JoinAp(&app->esp, app->config.ssid,
                                app->config.password);
        if (status == ESP32_AT_OK) app->state = WIFI_APP_SERVER_CONNECT;
        else retry(app, WIFI_APP_AP_CONNECT);
        break;

    case WIFI_APP_SERVER_CONNECT:
        if (!Esp32At_IsApConnected(&app->esp)) {
            retry(app, WIFI_APP_AP_CONNECT);
            break;
        }
        status = Esp32At_OpenTcp(&app->esp, app->config.server_ip,
                                 app->config.server_port);
        if (status == ESP32_AT_OK) {
            app->state = WIFI_APP_ONLINE;
            app->next_action_tick = HAL_GetTick() + app->config.health_check_ms;
        } else {
            retry(app, WIFI_APP_SERVER_CONNECT);
        }
        break;

    case WIFI_APP_ONLINE:
        if (!time_reached(app->next_action_tick)) break;
        if (!Esp32At_IsApConnected(&app->esp)) {
            retry(app, WIFI_APP_AP_CONNECT);
        } else if (!Esp32At_IsTcpConnected(&app->esp)) {
            retry(app, WIFI_APP_SERVER_CONNECT);
        } else {
            app->next_action_tick = HAL_GetTick() + app->config.health_check_ms;
        }
        break;

    case WIFI_APP_RETRY_WAIT:
        if (time_reached(app->next_action_tick)) app->state = app->retry_target;
        break;

    default:
        retry(app, WIFI_APP_INIT);
        break;
    }
}

bool WifiApp_IsOnline(const WifiApp *app)
{
    return app != NULL && app->state == WIFI_APP_ONLINE;
}
