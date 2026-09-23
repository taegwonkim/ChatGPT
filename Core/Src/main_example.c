/* Copy the marked sections into the CubeMX-generated Core/Src/main.c.
 * Keep this file out of the build to avoid defining a second main(). */
#include "main.h"
#include "wifi_app.h"

extern UART_HandleTypeDef huart1;
static WifiApp wifi;

static const WifiAppConfig wifi_config = {
    .ssid = "YOUR_SSID",
    .password = "YOUR_PASSWORD",
    .server_ip = "192.168.0.10",
    .server_port = 5000U,
    .dhcp_enabled = true,
    .static_ip = "192.168.0.50",
    .gateway = "192.168.0.1",
    .netmask = "255.255.255.0",
    .health_check_ms = 5000U,
    .retry_ms = 3000U
};

/* USER CODE BEGIN 2
WifiApp_Init(&wifi, &huart1, &wifi_config);
USER CODE END 2 */

/* USER CODE BEGIN WHILE
while (1)
{
    WifiApp_Process(&wifi);
    HAL_Delay(10U);
}
USER CODE END WHILE */
