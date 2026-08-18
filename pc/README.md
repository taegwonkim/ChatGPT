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

`SerialPanel`, `WifiPanel`, `MeasurementPanel`, `MonitorPanel`은 모두 Visual Studio WinForms designer가 편집할 수 있는 표준 `UserControl` + `.Designer.cs` 구조입니다. Solution Explorer에서 각 `*Panel.cs` 왼쪽 화살표를 펼치면 같은 이름의 `*Panel.Designer.cs`가 종속 파일로 보입니다. **반드시 상위 `*Panel.cs`를 선택한 상태에서 Shift+F7**을 누르십시오. `.Designer.cs` 자체를 열거나 코드 보기 상태에서는 도구 상자 안내 문구만 보일 수 있습니다.

기존에는 `WifiPanel`만 실제 `UserControl`/`.Designer.cs` 구조이고 나머지 panel은 `GroupBox`를 상속해 생성자에서 컨트롤을 동적으로 만들었습니다. `.csproj`의 `SubType`만 `UserControl`로 지정해도 WinForms 디자인 루트가 만들어지는 것은 아니므로 나머지는 Component Designer 안내 화면이 표시되었습니다. 현재는 네 panel 모두 실제 `UserControl` partial class와 `InitializeComponent()`를 사용하도록 변경했습니다.

Main Form도 `MainForm.cs` + `MainForm.Designer.cs` 구조로 변경했으므로 `MainForm.cs`에서 **디자이너 보기**를 선택하면 Serial/Wi-Fi/Measurement/Monitor 배치가 표시됩니다. Designer 파일에는 Visual Studio의 CodeDOM parser가 안정적으로 읽을 수 있도록 `new ColumnStyle(...)`, `new RowStyle(...)`, `new object[] { ... }`처럼 타입이 명확한 기존 문법을 사용합니다. `new(...)` target-typed 표현이나 `[ ... ]` collection expression은 프로그램 빌드에는 유효해도 일부 Visual Studio 2022 WinForms Designer 버전에서 파싱 오류를 일으킬 수 있습니다.

## Panel 크기 변경

Main Form에서 Wi-Fi/Measurement panel은 좌우 `SplitContainer`, 설정 영역/MCU 수신 영역은 상하 `SplitContainer`로 배치됩니다. 프로그램 실행 중 가운데 splitter를 마우스로 드래그하면 각 영역 크기를 조정할 수 있으며, 종료할 때 splitter 위치를 `settings.json`에 저장해 다음 실행 시 복원합니다.

Designer에서 초기 크기를 바꾸려면 `MainForm.cs`의 Designer를 열고 splitter를 드래그하거나, Properties 창에서 `settingsSplit.SplitterDistance`(Wi-Fi 영역 너비)와 `contentSplit.SplitterDistance`(설정 영역 높이)를 변경하십시오. Serial 영역 높이는 Main Form Designer에서 `serialPanel`을 선택하고 `Size → Height`를 변경합니다. 개별 `WifiPanel.cs`나 `MeasurementPanel.cs`에서 `Size`를 바꿔도 Main Form에서는 `Dock=Fill`과 SplitContainer가 실제 크기를 결정하므로 원래처럼 보일 수 있습니다. 즉, 실행 화면의 panel 비율은 개별 panel의 `Size`가 아니라 Main Form의 두 `SplitterDistance`로 조정해야 합니다.

### Wi-Fi label과 입력창 높이 맞추기

Wi-Fi 설정의 첫 네 행은 `RowStyle = Absolute, 34px`로 통일했습니다. Label은 `AutoSize=false`, `Height=23`, `TextAlign=MiddleLeft`를 사용하고 TextBox/NumericUpDown은 위아래 Dock 대신 `Anchor=Left|Right`를 사용합니다. TableLayoutPanel은 위/아래 Anchor가 없는 컨트롤을 행 가운데에 배치하므로 label과 입력창의 세로 중앙 및 보이는 높이가 맞습니다. Designer에서 변경하려면 `WifiPanel.cs`를 디자인 화면으로 열고 label의 `AutoSize`, `Size.Height`, `TextAlign`, `Anchor`와 해당 행의 `RowStyle`을 같은 값으로 유지하십시오. Windows 단일 행 TextBox는 폰트에 따라 Height가 자동 결정되므로 `Dock=Fill`로 세로 방향까지 늘리지 않는 것이 중요합니다.

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
<STX>MEAS_W_ALL,referenceMv,offsetMv,resistanceMilliOhm,intervalSeconds,rs485Only<CR><LF>
```

수신 parser는 여러 serial read에 나뉘어 들어온 frame과 한 번에 연속 수신된 여러 frame을 모두 처리하며, 비정상 데이터는 다음 STX에서 다시 동기화합니다. MCU가 `WIFI_W_ALL,...`/`WIFI_R_ALL,...` 또는 `MEAS_W_ALL,...`/`MEAS_R_ALL,...` 설정 frame을 응답하면 해당 panel 입력값도 갱신합니다.

Wi-Fi 및 Measurement의 Read/Write 송신과 설정 응답은 **모두 첫 byte로 STX(0x02)를 사용**합니다. `DeviceProtocol.Frame()`이 모든 Read/Write command 앞에 STX를 자동으로 붙입니다. 설정 응답에 STX가 없으면 설정 panel 값으로 적용하지 않고 오른쪽 RAW 창에 표시합니다.

설정 응답의 command 뒤에는 `,`, `;`, `|`, `:`, `=`를, 필드 사이에는 `,`, `;`, `|` 구분자를 지원합니다. 위치 기반 값뿐 아니라 `SSID=my_ap`, `PORT=5000`, `DHCP=ON` 같은 `key=value` 필드와 따옴표로 감싼 값도 인식합니다. MCU가 Read command에 대한 STX 응답에서 command를 생략하고 값만 보내는 경우에도, Read timeout 안에 도착한 8개 Wi-Fi 필드 또는 4개 Measurement 필드를 요청 중인 panel에 적용합니다. DHCP 값은 `0/1`, `OFF/ON`, `FALSE/TRUE`를 지원합니다. `WIFI_R_ALL` 또는 `MEAS_R_ALL`로 시작하지만 필드 수/숫자 형식이 맞지 않으면 오른쪽 창에 `[설정 응답 형식 오류]`로 원문을 표시합니다.

현재 MCU Read 응답의 기본 형식은 command 이름 없이 다음 순서로 들어오는 STX frame입니다. 마지막 값 뒤의 선택적인 쉼표도 허용합니다.

```text
<STX>ssid,password,serverIp,serverPort,dhcp,localIp,gateway,netmask<CR><LF>
<STX>reference,offset,resistance,intervalTime,rs485Only<CR><LF>
```

Wi-Fi Read를 누른 뒤 첫 번째 8-field STX frame은 Wi-Fi 설정 panel에, Measurement Read를 누른 뒤 첫 번째 5-field STX frame은 Measurement 설정 panel에 표시합니다. DHCP와 RS485_ONLY는 `0=Off`, `1=On`으로 적용합니다. command 없는 응답을 놓치지 않도록 최소 5초 동안 해당 Read 응답을 기다리며, 정상 설정 응답은 아래 MCU 수신 데이터 창에 출력하지 않습니다.

Measurement 설정의 마지막 항목에 **RS485_ONLY**를 추가했습니다. ComboBox 기본값은 OFF이며 Write 시 OFF는 `0`, ON은 `1`을 마지막 field로 전송합니다. Read 응답의 마지막 `0/1`도 ComboBox의 OFF/ON으로 표시합니다. 설정 응답은 command 없이 `reference,offset,resistance,intervalTime,rs485Only` 다섯 값으로 올라오므로 Read 대기 시간과 관계없이 **5-field 숫자 STX frame을 먼저 Measurement 설정 응답으로 판정**합니다. 따라서 이 frame은 하단 측정값 창으로 전달되지 않습니다. 이전 4-field 응답은 RS485_ONLY=OFF로 호환 처리하며, 실제 측정 데이터는 `<STX>DC_...<CR><LF>` 식별자를 사용해야 합니다.

Read 응답이 정상적으로 파싱되면 값은 Wi-Fi 또는 Measurement 설정 영역에만 표시되며 **기타 MCU 데이터 / 상태** 창에는 중복 출력하지 않습니다. Read로 받은 값, Write 버튼으로 보낸 값, 프로그램 종료 시 화면에 있던 값은 `%LOCALAPPDATA%\STM32MeasurementMonitor\settings.json`에 저장되고 다음 실행 시 자동 복원됩니다. Serial Port의 COM port, baudrate, timeout도 Open 성공 시와 프로그램 종료 시 같은 파일에 저장되며 다음 실행 시 자동 선택됩니다. 저장했던 COM port가 현재 연결되어 있지 않아도 ComboBox에 마지막 선택값을 유지하므로 장치를 다시 연결한 뒤 사용할 수 있습니다. 저장 파일에는 Wi-Fi password도 포함되므로 해당 Windows 사용자 계정의 파일 접근 권한을 적절히 관리하십시오.

측정값의 기본 형식은 `<STX>DC_-----<CR><LF>`입니다. STX가 붙은 payload가 `DC_`로 시작하면 `-----` 부분의 내용이 숫자가 아니더라도 왼쪽 **측정값** 창에 표시됩니다. 이전 호환성을 위해 STX가 붙은 숫자 CSV, `DATA,...`, `MEAS_DATA,...`도 측정값으로 인식합니다. `STATUS...`, 설정 응답, 그 밖의 문자열 frame은 오른쪽 기타 MCU 데이터 창에 표시됩니다.

측정값은 `<STX>payload<CR><LF>`로 수신하고, 일반 MCU 데이터는 STX 없이 `payload<CR><LF>`로 수신하는 형식을 지원합니다. STX 없는 line은 내용이 숫자 CSV처럼 보여도 항상 오른쪽 **기타 MCU 데이터 / 상태** 창에 `[RAW]` 표시와 함께 출력됩니다. 수신 중 STX가 나타나면 그 지점부터 새 framed packet으로 다시 동기화합니다.
