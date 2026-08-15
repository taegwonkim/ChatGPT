#include "app.h"
#include "app_config.h"
#include "app_protocol.h"
#include "app_flash.h"
#include "cmsis_os2.h"
#include <string.h>
#include <stdio.h>

extern UART_HandleTypeDef *AppPort_EspUart(void), *AppPort_FpgaUart(void), *AppPort_PcUart(void);
extern uint8_t *AppPort_RxBuffer(UART_HandleTypeDef *h);
extern int Board_UsbTransmit(const uint8_t *, uint16_t);

typedef struct { uint8_t source; uint16_t length; char data[APP_LINE_MAX]; } RxMessage;
static osMessageQueueId_t rxQ, measureQ, wifiTxQ, pcTxQ, cfgQ;
static osMutexId_t cfgMutex;
static AppWifiConfig config;

static void uart_send(UART_HandleTypeDef *u, const char *s) { HAL_UART_Transmit(u,(uint8_t*)s,strlen(s),1000); }

void App_UartRxEventFromISR(UART_HandleTypeDef *h, uint16_t n) {
    RxMessage m={.source=h==AppPort_EspUart()?1:h==AppPort_FpgaUart()?2:3}; if(n>=sizeof m.data)n=sizeof(m.data)-1;
    memcpy(m.data,AppPort_RxBuffer(h),n);m.data[n]=0;m.length=n; osMessageQueuePut(rxQ,&m,0,0);
    HAL_UARTEx_ReceiveToIdle_DMA(h,AppPort_RxBuffer(h),APP_LINE_MAX); __HAL_DMA_DISABLE_IT(h->hdmarx,DMA_IT_HT);
}
void App_UsbRx(const uint8_t *p,uint32_t n){RxMessage m={.source=4};if(n>=sizeof m.data)n=sizeof(m.data)-1;memcpy(m.data,p,n);m.data[n]=0;m.length=n;osMessageQueuePut(rxQ,&m,0,0);}

static void Supervisor(void *arg){(void)arg;osDelay(500);uart_send(AppPort_FpgaUart(),"START\r\n");for(;;)osDelay(1000);}
static void RxTask(void *arg){(void)arg;RxMessage r;for(;;){osMessageQueueGet(rxQ,&r,0,osWaitForever);
    if(r.source==2){AppMeasurement m={.timestamp_ms=osKernelGetTickCount()};if(App_ParseMeasurement(r.data,&m)){if(osMessageQueuePut(measureQ,&m,0,0)!=osOK){AppMeasurement old;osMessageQueueGet(measureQ,&old,0,0);osMessageQueuePut(measureQ,&m,0,0);}}}
    else if(r.source==3||r.source==4){AppWifiConfig c;if(App_ParseConfig(r.data,&c))osMessageQueuePut(cfgQ,&c,0,0);}
}}
static void Router(void *arg){(void)arg;AppMeasurement m;for(;;){osMessageQueueGet(measureQ,&m,0,osWaitForever);osMessageQueuePut(wifiTxQ,&m,0,0);osMessageQueuePut(pcTxQ,&m,0,0);}}

static int at(const char *cmd){uart_send(AppPort_EspUart(),cmd);/* Production: match OK/ERROR from USART1 RX stream with timeout. */osDelay(200);return 1;}
static int connect_wifi(void){char cmd[180];if(!at("AT\r\n")||!at("ATE0\r\n")||!at("AT+CWMODE=1\r\n"))return 0;
    osMutexAcquire(cfgMutex,osWaitForever);snprintf(cmd,sizeof cmd,"AT+CWJAP=\"%s\",\"%s\"\r\n",config.ssid,config.password);osMutexRelease(cfgMutex);if(!at(cmd))return 0;
    osMutexAcquire(cfgMutex,osWaitForever);snprintf(cmd,sizeof cmd,"AT+CIPSTART=\"TCP\",\"%s\",%u\r\n",config.server_ip,config.server_port);osMutexRelease(cfgMutex);return at(cmd);}
static void EspTask(void *arg){(void)arg;uint32_t backoff=1000;AppMeasurement m;char line[96],cmd[32];int online=0;for(;;){if(!online){online=connect_wifi();if(!online){osDelay(backoff);if(backoff<30000)backoff*=2;continue;}backoff=1000;}
    if(osMessageQueueGet(wifiTxQ,&m,0,1000)==osOK){int n=App_FormatMeasurement(line,sizeof line,&m);snprintf(cmd,sizeof cmd,"AT+CIPSEND=%d\r\n",n);if(!at(cmd)){online=0;continue;}uart_send(AppPort_EspUart(),line);}}
}
static void PcTask(void *arg){(void)arg;AppMeasurement m;char line[96];for(;;){if(osMessageQueueGet(pcTxQ,&m,0,osWaitForever)==osOK){int n=App_FormatMeasurement(line,sizeof line,&m);uart_send(AppPort_PcUart(),line);Board_UsbTransmit((uint8_t*)line,n);}}}
static void FlashTask(void *arg){(void)arg;AppWifiConfig c;for(;;){osMessageQueueGet(cfgQ,&c,0,osWaitForever);if(AppFlash_SaveConfig(&c)){osMutexAcquire(cfgMutex,osWaitForever);config=c;osMutexRelease(cfgMutex);}}}

void App_RTOS_Init(void){AppWifiConfig defaults={.ssid="",.password="",.server_ip="192.168.0.10",.server_port=5000,.dhcp=1};config=defaults;AppFlash_LoadConfig(&config);
    rxQ=osMessageQueueNew(12,sizeof(RxMessage),0);measureQ=osMessageQueueNew(APP_MEAS_QUEUE_DEPTH,sizeof(AppMeasurement),0);wifiTxQ=osMessageQueueNew(APP_TX_QUEUE_DEPTH,sizeof(AppMeasurement),0);pcTxQ=osMessageQueueNew(APP_TX_QUEUE_DEPTH,sizeof(AppMeasurement),0);cfgQ=osMessageQueueNew(2,sizeof(AppWifiConfig),0);cfgMutex=osMutexNew(0);
    const osThreadAttr_t sup={.name="Supervisor",.priority=osPriorityAboveNormal,.stack_size=768*4},rx={.name="FpgaRx",.priority=osPriorityHigh,.stack_size=512*4},route={.name="Router",.priority=osPriorityNormal,.stack_size=768*4},esp={.name="EspAt",.priority=osPriorityNormal,.stack_size=1024*4},pc={.name="Pc",.priority=osPriorityBelowNormal,.stack_size=768*4},fl={.name="Flash",.priority=osPriorityLow,.stack_size=768*4};
    osThreadNew(Supervisor,0,&sup);osThreadNew(RxTask,0,&rx);osThreadNew(Router,0,&route);osThreadNew(EspTask,0,&esp);osThreadNew(PcTask,0,&pc);osThreadNew(FlashTask,0,&fl);
}
