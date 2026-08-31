#include "fpga_protocol.h"
#include "surge_config.h"
#include "wifi_state.h"
#include <assert.h>
#include <string.h>

int main(void) {
  SurgeConfig c; SurgeConfig_Default(&c);
  assert(SurgeConfig_Validate(&c)); assert(SurgeConfig_CheckCrc(&c));
  c.server_port=0; assert(!SurgeConfig_Validate(&c));
  assert(WifiMachine_BackoffMs(0,0)==1000); assert(WifiMachine_BackoffMs(9,0)==16000);

  uint8_t raw[2+8+24+4]={0xA5,0x5A,1,6,1,0,42,0,0,0};
  uint32_t crc=Surge_Crc32(raw+2,8+24);
  raw[34]=(uint8_t)crc; raw[35]=(uint8_t)(crc>>8); raw[36]=(uint8_t)(crc>>16); raw[37]=(uint8_t)(crc>>24);
  FpgaParser p; FpgaFrameView f; FpgaParser_Init(&p); bool got=false;
  for(size_t i=0;i<sizeof(raw);i++) got=FpgaParser_Push(&p,raw[i],&f)||got;
  assert(got && f.sequence==42 && f.samples_per_channel==1 && f.payload_size==24);
  raw[20]^=1; FpgaParser_Init(&p); got=false;
  for(size_t i=0;i<sizeof(raw);i++) got=FpgaParser_Push(&p,raw[i],&f)||got;
  assert(!got);
  return 0;
}
