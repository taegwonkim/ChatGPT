#include "wifi_state.h"
uint32_t wifi_backoff_seconds(uint8_t attempts) {
    if (attempts >= 5u) return 30u;
    return UINT32_C(1) << attempts;
}
void wifi_init(WifiMachine *wifi, uint32_t now_ms) { wifi->state = WIFI_RESET; wifi->attempts = 0u; wifi->deadline_ms = now_ms; }
void wifi_disconnected(WifiMachine *wifi, uint32_t now_ms) {
    wifi->state = WIFI_BACKOFF;
    wifi->deadline_ms = now_ms + wifi_backoff_seconds(wifi->attempts) * 1000u;
    if (wifi->attempts < 255u) ++wifi->attempts;
}
bool wifi_backoff_expired(WifiMachine *wifi, uint32_t now_ms) {
    if (wifi->state != WIFI_BACKOFF || (int32_t)(now_ms - wifi->deadline_ms) < 0) return false;
    wifi->state = WIFI_RESET;
    return true;
}
