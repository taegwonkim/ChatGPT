# Measurement Monitor (Visual C# WinForms)

Visual Studio 2022와 .NET 8용 Windows Forms 프로그램입니다. `MeasurementMonitor/MeasurementMonitor.csproj`를 열어 빌드하십시오.

## 화면 구성

- **Serial Port**: COM port, baudrate, timeout, Open/Close, 포트 새로고침, 로그 Clear
- **Wi-Fi 설정**: AP, server, DHCP 및 static IPv4 설정의 Read/Write
- **Measurement 설정**: Reference, Offset, Resistance, Interval Time의 Read/Write
- **측정값/상태**: `STX + payload + CR + LF` frame 표시와 auto scroll

사양에 read command와 write command가 모두 `WIFI_W_ALL`, `MEAS_W_ALL`로 적혀 있으므로 이 구현은 **Read는 command만**, **Write는 command 뒤에 CSV 설정값을 붙여서** 구분합니다. MCU가 별도의 `WIFI_R_ALL`/`MEAS_R_ALL`을 요구한다면 `DeviceProtocol.cs`의 `WifiRead()`/`MeasurementRead()` 문자열만 변경하면 됩니다.

Write frame은 다음과 같습니다.

```text
<STX>WIFI_W_ALL,ssid,password,serverIp,serverPort,dhcp,localIp,gateway,netmask<CR><LF>
<STX>MEAS_W_ALL,referenceMv,offsetMv,resistanceMilliOhm,intervalSeconds<CR><LF>
```

수신 parser는 여러 serial read에 나뉘어 들어온 frame과 한 번에 연속 수신된 여러 frame을 모두 처리하며, 비정상 데이터는 다음 STX에서 다시 동기화합니다. MCU가 `WIFI_W_ALL,...`/`WIFI_R_ALL,...` 또는 `MEAS_W_ALL,...`/`MEAS_R_ALL,...` 설정 frame을 응답하면 해당 panel 입력값도 갱신합니다.
