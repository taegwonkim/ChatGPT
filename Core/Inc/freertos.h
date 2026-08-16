#ifndef FREERTOS_H
#define FREERTOS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Called by main() after osKernelInitialize() and before osKernelStart(). */
void MX_FREERTOS_Init(void);

#ifdef __cplusplus
}
#endif

#endif
