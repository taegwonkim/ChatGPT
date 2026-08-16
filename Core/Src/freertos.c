#include "freertos.h"
#include "main.h"
#include "surge_rtos.h"

/* Queue item passed by USB CDC/USART3 callbacks to PcCommandTask. */
typedef struct {
    uint8_t source;
    uint16_t length;
    uint8_t data[256];
} PcCommandMessage;

osThreadId_t fpgaRxTaskHandle;
osMessageQueueId_t pcCommandQueueHandle;

static osThreadId_t dataRouterTaskHandle;
static osThreadId_t wifiTaskHandle;
static osThreadId_t pcCommandTaskHandle;
static osThreadId_t flashTaskHandle;
static osThreadId_t supervisorTaskHandle;

static osMessageQueueId_t wifiTxQueueHandle;
static osMessageQueueId_t flashRequestQueueHandle;
static osMutexId_t configMutexHandle;

/* Queue payload is a reference to a fixed frame-pool block, not ADC data. */
typedef struct {
    void *block;
    uint16_t length;
    uint16_t flags;
} FrameRef;

typedef struct {
    uint8_t operation;
    void *buffer;
    uint16_t length;
} FlashRequest;

static const osThreadAttr_t fpgaRxTaskAttributes = {
    .name = "FpgaRxTask", .priority = osPriorityAboveNormal, .stack_size = 3072
};
static const osThreadAttr_t dataRouterTaskAttributes = {
    .name = "DataRouterTask", .priority = osPriorityHigh, .stack_size = 3072
};
static const osThreadAttr_t wifiTaskAttributes = {
    .name = "WifiTask", .priority = osPriorityNormal, .stack_size = 4096
};
static const osThreadAttr_t pcCommandTaskAttributes = {
    .name = "PcCommandTask", .priority = osPriorityNormal, .stack_size = 3072
};
static const osThreadAttr_t flashTaskAttributes = {
    .name = "FlashTask", .priority = osPriorityLow, .stack_size = 2048
};
static const osThreadAttr_t supervisorTaskAttributes = {
    .name = "SupervisorTask", .priority = osPriorityLow, .stack_size = 1536
};

static void require_object(const void *object)
{
    if (object == NULL) {
        /* Creation failure occurs before scheduler start, so do not osDelay. */
        Error_Handler();
    }
}

void MX_FREERTOS_Init(void)
{
    const osMutexAttr_t mutexAttributes = { .name = "configMutex" };

    configMutexHandle = osMutexNew(&mutexAttributes);
    pcCommandQueueHandle = osMessageQueueNew(8, sizeof(PcCommandMessage), NULL);
    wifiTxQueueHandle = osMessageQueueNew(16, sizeof(FrameRef), NULL);
    flashRequestQueueHandle = osMessageQueueNew(4, sizeof(FlashRequest), NULL);
    require_object(configMutexHandle);
    require_object(pcCommandQueueHandle);
    require_object(wifiTxQueueHandle);
    require_object(flashRequestQueueHandle);

    fpgaRxTaskHandle = osThreadNew(Surge_FpgaRxTask, NULL, &fpgaRxTaskAttributes);
    dataRouterTaskHandle = osThreadNew(Surge_DataRouterTask, NULL, &dataRouterTaskAttributes);
    wifiTaskHandle = osThreadNew(Surge_WifiTask, NULL, &wifiTaskAttributes);
    pcCommandTaskHandle = osThreadNew(Surge_PcCommandTask, NULL, &pcCommandTaskAttributes);
    flashTaskHandle = osThreadNew(Surge_FlashTask, NULL, &flashTaskAttributes);
    supervisorTaskHandle = osThreadNew(Surge_SupervisorTask, NULL, &supervisorTaskAttributes);
    require_object(fpgaRxTaskHandle);
    require_object(dataRouterTaskHandle);
    require_object(wifiTaskHandle);
    require_object(pcCommandTaskHandle);
    require_object(flashTaskHandle);
    require_object(supervisorTaskHandle);
}
