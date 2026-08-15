#include "app_protocol.h"
#include <assert.h>
#include <string.h>
int main(void){AppWifiConfig c;assert(App_ParseConfig("CFG,{\"ssid\":\"lab\",\"password\":\"pw\",\"ip\":\"10.0.0.2\",\"port\":5000,\"dhcp\":true}",&c));assert(!strcmp(c.ssid,"lab")&&c.server_port==5000&&c.dhcp);AppMeasurement m={0};assert(App_ParseMeasurement("MEAS,7,-42\n",&m));assert(m.sequence==7&&m.value==-42);assert(!App_ParseMeasurement("bad",&m));return 0;}
