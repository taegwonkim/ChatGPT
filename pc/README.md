# Measurement Monitor (Visual C# WinForms)

Visual Studio 2022와 .NET 8용 Windows Forms 프로그램입니다.

## Visual Studio 2022에서 열기

1. Visual Studio Installer에서 **.NET 데스크톱 개발** workload와 **.NET 8 SDK**를 설치합니다. .NET 8 프로젝트이므로 Visual Studio 2022 17.8 이상을 권장합니다.
2. 저장소의 `pc/MeasurementMonitor.sln`을 더블 클릭하거나 Visual Studio의 **파일 → 열기 → 프로젝트/솔루션**에서 엽니다. `.csproj`가 아니라 solution을 여는 것이 권장됩니다.
3. Solution Explorer에서 `MeasurementMonitor`를 시작 프로젝트로 선택하고 상단 구성을 `Debug / Any CPU`로 둡니다.
4. **빌드 → 솔루션 빌드**(`Ctrl+Shift+B`) 후 **디버그 → 디버깅 시작**(`F5`)을 선택합니다.

## 각 panel을 보는 방법

프로그램을 `F5`로 실행하면 Main Form 하나에 다음 panel이 모두 표시됩니다.

```text
┌ Serial Port: COM / Baudrate / Timeout / Open / Clear ┐
├ Wi-Fi 설정                    │ Measurement 설정      ┤
├──────────────────────────────────────────────────────┤
│ 측정값 / STATUS monitor + Auto scroll                │
└──────────────────────────────────────────────────────┘
```

Visual Studio의 디자인 화면에서는 Solution Explorer의 `MainForm.cs`를 선택한 후 **Shift+F7** 또는 우클릭 → **디자이너 보기**를 선택합니다. Main Form designer에서 네 panel의 전체 배치를 확인할 수 있습니다. 개별 panel은 `SerialPanel.cs`, `WifiPanel.cs`, `MeasurementPanel.cs`, `MonitorPanel.cs`를 선택해 같은 방법으로 designer를 열 수 있습니다. 이 프로젝트는 각 파일에 Visual Studio용 `SubType` metadata를 지정했기 때문에 Solution Explorer에서 panel component로 인식됩니다.

> 이 UI는 `.Designer.cs`의 고정 좌표 대신 각 panel 생성자에서 `TableLayoutPanel`, `FlowLayoutPanel`, `Dock`을 사용해 구성합니다. 따라서 designer 화면을 열었을 때 코드가 실행되어 panel이 렌더링됩니다. designer cache 때문에 빈 화면이 보이면 먼저 솔루션을 빌드한 뒤 designer를 닫았다 다시 열고, 그래도 보이지 않으면 `F5` 실행 화면에서 확인하십시오.

## 화면 구성

- **Serial Port**: COM port, baudrate, timeout, Open/Close, 포트 새로고침, 로그 Clear
- **Wi-Fi 설정**: AP, server, DHCP 및 static IPv4 설정의 Read/Write
- **Measurement 설정**: Reference, Offset, Resistance, Interval Time의 Read/Write
- **MCU 수신 데이터**: 왼쪽에는 숫자 CSV 측정값, 오른쪽에는 STATUS·설정 응답·기타 frame을 분리 표시하며 auto scroll 지원

Read 버튼과 Write 버튼은 서로 다른 command를 전송합니다.

```text
Wi-Fi Read       : <STX>WIFI_R_ALL<CR><LF>
Measurement Read : <STX>MEAS_R_ALL<CR><LF>
```

Write frame은 다음과 같습니다.

```text
<STX>WIFI_W_ALL,ssid,password,serverIp,serverPort,dhcp,localIp,gateway,netmask<CR><LF>
<STX>MEAS_W_ALL,referenceMv,offsetMv,resistanceMilliOhm,intervalSeconds<CR><LF>
```

수신 parser는 여러 serial read에 나뉘어 들어온 frame과 한 번에 연속 수신된 여러 frame을 모두 처리하며, 비정상 데이터는 다음 STX에서 다시 동기화합니다. MCU가 `WIFI_W_ALL,...`/`WIFI_R_ALL,...` 또는 `MEAS_W_ALL,...`/`MEAS_R_ALL,...` 설정 frame을 응답하면 해당 panel 입력값도 갱신합니다.

Wi-Fi 및 Measurement의 Read/Write 송신과 설정 응답은 **모두 첫 byte로 STX(0x02)를 사용**합니다. `DeviceProtocol.Frame()`이 모든 Read/Write command 앞에 STX를 자동으로 붙입니다. 설정 응답에 STX가 없으면 설정 panel 값으로 적용하지 않고 오른쪽 RAW 창에 표시합니다.

설정 응답의 command 뒤에는 `,`, `;`, `|`, `:`, `=`를, 필드 사이에는 `,`, `;`, `|` 구분자를 지원합니다. 위치 기반 값뿐 아니라 `SSID=my_ap`, `PORT=5000`, `DHCP=ON` 같은 `key=value` 필드와 따옴표로 감싼 값도 인식합니다. MCU가 Read command에 대한 STX 응답에서 command를 생략하고 값만 보내는 경우에도, Read timeout 안에 도착한 8개 Wi-Fi 필드 또는 4개 Measurement 필드를 요청 중인 panel에 적용합니다. DHCP 값은 `0/1`, `OFF/ON`, `FALSE/TRUE`를 지원합니다. `WIFI_R_ALL` 또는 `MEAS_R_ALL`로 시작하지만 필드 수/숫자 형식이 맞지 않으면 오른쪽 창에 `[설정 응답 형식 오류]`로 원문을 표시합니다.

측정값의 기본 형식은 `<STX>DC_-----<CR><LF>`입니다. STX가 붙은 payload가 `DC_`로 시작하면 `-----` 부분의 내용이 숫자가 아니더라도 왼쪽 **측정값** 창에 표시됩니다. 이전 호환성을 위해 STX가 붙은 숫자 CSV, `DATA,...`, `MEAS_DATA,...`도 측정값으로 인식합니다. `STATUS...`, 설정 응답, 그 밖의 문자열 frame은 오른쪽 기타 MCU 데이터 창에 표시됩니다.

측정값은 `<STX>payload<CR><LF>`로 수신하고, 일반 MCU 데이터는 STX 없이 `payload<CR><LF>`로 수신하는 형식을 지원합니다. STX 없는 line은 내용이 숫자 CSV처럼 보여도 항상 오른쪽 **기타 MCU 데이터 / 상태** 창에 `[RAW]` 표시와 함께 출력됩니다. 수신 중 STX가 나타나면 그 지점부터 새 framed packet으로 다시 동기화합니다.
