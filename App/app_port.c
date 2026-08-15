#include "app.h"
#include "app_flash.h"
#include <string.h>

static SPI_HandleTypeDef *spi;
static UART_HandleTypeDef *u1, *u2, *u3;
static uint8_t rx1[APP_LINE_MAX], rx2[APP_LINE_MAX], rx3[APP_LINE_MAX];

__attribute__((weak)) void Board_FlashCs(int active) { (void)active; }
__attribute__((weak)) void Board_Rs485De(int active) { (void)active; }
__attribute__((weak)) int Board_UsbTransmit(const uint8_t *p, uint16_t n) { (void)p; (void)n; return -1; }

static bool spi_cmd(uint8_t *tx, uint8_t *rx, uint16_t n) {
    Board_FlashCs(1); HAL_StatusTypeDef s = HAL_SPI_TransmitReceive(spi, tx, rx, n, 1000); Board_FlashCs(0); return s == HAL_OK;
}
static bool ready(void) {
    uint32_t deadline = HAL_GetTick() + 3000U; uint8_t tx[2] = {5, 0}, rx[2];
    do { if (!spi_cmd(tx, rx, 2)) return false; if (!(rx[1] & 1U)) return true; } while ((int32_t)(HAL_GetTick()-deadline) < 0);
    return false;
}
static bool write_enable(void) { uint8_t tx=6, rx; return spi_cmd(&tx, &rx, 1); }

bool Board_FlashRead(uint32_t a, void *data, uint32_t n) {
    uint8_t cmd[4] = {3, a>>16, a>>8, a}; Board_FlashCs(1);
    bool ok = HAL_SPI_Transmit(spi, cmd, 4, 1000)==HAL_OK && HAL_SPI_Receive(spi, data, n, 1000)==HAL_OK;
    Board_FlashCs(0); return ok;
}
bool Board_FlashEraseSector(uint32_t a) {
    uint8_t cmd[4]={0x20,a>>16,a>>8,a}; if (!write_enable()) return false; Board_FlashCs(1);
    bool ok=HAL_SPI_Transmit(spi,cmd,4,1000)==HAL_OK; Board_FlashCs(0); return ok && ready();
}
bool Board_FlashProgram(uint32_t a, const void *vp, uint32_t n) {
    const uint8_t *p=vp; while(n) { uint32_t chunk=APP_FLASH_PAGE_SIZE-(a&(APP_FLASH_PAGE_SIZE-1U)); if(chunk>n)chunk=n;
        uint8_t cmd[4]={2,a>>16,a>>8,a}; if(!write_enable())return false; Board_FlashCs(1);
        bool ok=HAL_SPI_Transmit(spi,cmd,4,1000)==HAL_OK && HAL_SPI_Transmit(spi,(uint8_t*)p,chunk,1000)==HAL_OK; Board_FlashCs(0);
        if(!ok||!ready())return false; a+=chunk;p+=chunk;n-=chunk; } return true;
}

void App_Init(UART_HandleTypeDef *esp, UART_HandleTypeDef *fpga, UART_HandleTypeDef *pc, SPI_HandleTypeDef *flash) {
    u1=esp;u2=fpga;u3=pc;spi=flash;
    HAL_UARTEx_ReceiveToIdle_DMA(u1,rx1,sizeof rx1); HAL_UARTEx_ReceiveToIdle_DMA(u2,rx2,sizeof rx2); HAL_UARTEx_ReceiveToIdle_DMA(u3,rx3,sizeof rx3);
    __HAL_DMA_DISABLE_IT(u1->hdmarx,DMA_IT_HT); __HAL_DMA_DISABLE_IT(u2->hdmarx,DMA_IT_HT); __HAL_DMA_DISABLE_IT(u3->hdmarx,DMA_IT_HT);
}
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *h, uint16_t n) { App_UartRxEventFromISR(h,n); }

/* Accessors keep generated peripheral handles out of application modules. */
UART_HandleTypeDef *AppPort_EspUart(void){return u1;} UART_HandleTypeDef *AppPort_FpgaUart(void){return u2;} UART_HandleTypeDef *AppPort_PcUart(void){return u3;}
uint8_t *AppPort_RxBuffer(UART_HandleTypeDef *h){return h==u1?rx1:h==u2?rx2:rx3;}
