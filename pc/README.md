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

### 실행 중 화면 배치 편집

Serial Port 영역의 **화면 배치** 버튼을 누르면 Label, TextBox, ComboBox, Button, CheckBox, NumericUpDown 및 각 UserControl panel을 선택할 수 있는 편집기가 열립니다. 항목을 선택하고 **자유 배치**를 누르면 기존 TableLayout/Dock에서 분리되어 PropertyGrid의 `Location`, `Size`, `Font`, `BackColor`, `ForeColor`, `Anchor` 등을 직접 변경할 수 있습니다. **저장**을 누르면 `%LOCALAPPDATA%\STM32MeasurementMonitor\control-layout.json`에 기록되어 다음 실행 때 복원됩니다. **초기화**는 사용자 배치 파일을 삭제하며 프로그램 재시작 후 기본 Designer 배치로 돌아갑니다.

컨테이너에서 분리하기 전에는 TableLayoutPanel이나 SplitContainer가 Location/Size를 다시 계산하므로 먼저 **자유 배치**를 눌러야 합니다. 기능 버튼을 직접 드래그하는 방식이 아니라 별도 PropertyGrid에서 값을 편집하므로 Read/Write/Open 같은 실제 동작이 실수로 실행되지 않습니다. 너무 작은 크기나 겹치는 위치도 설정할 수 있으므로 변경 전 저장 파일을 백업하거나 초기화 기능을 사용하십시오.

편집기 목록에서 컨트롤을 선택하면 Main Form의 해당 컨트롤 둘레에 파란 점선 선택 테두리가 표시됩니다. 테두리를 마우스로 끌면 컨트롤을 Drag & Drop으로 이동하며, 오른쪽 아래의 파란 사각형 handle을 끌면 크기를 조절합니다. 첫 Drag 시 자동으로 **자유 배치** 상태로 전환됩니다. 선택 테두리의 안쪽은 비어 있어 실제 화면을 그대로 보면서 편집하는 WYSIWYG 방식이며, 이동/크기 변경 후 편집기의 **저장**을 눌러야 다음 실행에도 유지됩니다.

### Wi-Fi label과 입력창 높이 맞추기

Wi-Fi 설정의 첫 네 행은 `RowStyle = Absolute, 34px`로 통일했습니다. Label은 `AutoSize=false`, `Height=23`, `TextAlign=MiddleLeft`를 사용하고 TextBox/NumericUpDown은 위아래 Dock 대신 `Anchor=Left|Right`를 사용합니다. TableLayoutPanel은 위/아래 Anchor가 없는 컨트롤을 행 가운데에 배치하므로 label과 입력창의 세로 중앙 및 보이는 높이가 맞습니다. Designer에서 변경하려면 `WifiPanel.cs`를 디자인 화면으로 열고 label의 `AutoSize`, `Size.Height`, `TextAlign`, `Anchor`와 해당 행의 `RowStyle`을 같은 값으로 유지하십시오. Windows 단일 행 TextBox는 폰트에 따라 Height가 자동 결정되므로 `Dock=Fill`로 세로 방향까지 늘리지 않는 것이 중요합니다.

### DHCP On에서 Static IP 입력창 테두리 유지

DHCP가 On이면 Local IP, Gateway, Net Mask를 수정하지 못하도록 `Enabled=false` 대신 `ReadOnly=true`를 사용합니다. Disabled TextBox는 Windows theme이 테두리까지 흐리게 표시하지만, ReadOnly TextBox는 입력을 차단하면서 `BorderStyle=FixedSingle`의 검정 테두리를 유지할 수 있습니다. 배경은 `SystemColors.Window`, 글자는 읽기 전용일 때 `SystemColors.GrayText`로 설정하며, DHCP가 Off가 되면 `ReadOnly=false`와 `WindowText`로 복원합니다. Designer에서 세 TextBox의 `BorderStyle`을 `FixedSingle`로 유지하십시오.

### Read/Write 버튼 크기와 폰트 변경

Wi-Fi와 Measurement의 Read/Write 버튼은 기본적으로 `Size=90,32`, `Font=Segoe UI 10pt`, `AutoSize=false`, `Anchor=None`으로 통일했습니다. Designer에서 변경하려면 `WifiPanel.cs` 또는 `MeasurementPanel.cs`를 디자인 화면으로 열고 Read/Write 버튼을 Ctrl 키로 함께 선택한 뒤 Properties 창의 `Size`, `Font`, `AutoSize`를 수정하십시오. 지정한 크기를 유지하려면 `AutoSize=False`여야 하며, TableLayoutPanel 셀 가운데에 유지하려면 `Anchor=None`을 사용합니다. 굵은 글자를 원하면 `Font` 대화상자의 Style을 `Bold`로 선택합니다. 두 panel에서 동일하게 보이게 하려면 네 버튼에 같은 값을 적용하십시오.

> 이 UI는 `.Designer.cs`의 고정 좌표 대신 각 panel 생성자에서 `TableLayoutPanel`, `FlowLayoutPanel`, `Dock`을 사용해 구성합니다. 따라서 designer 화면을 열었을 때 코드가 실행되어 panel이 렌더링됩니다. designer cache 때문에 빈 화면이 보이면 먼저 솔루션을 빌드한 뒤 designer를 닫았다 다시 열고, 그래도 보이지 않으면 `F5` 실행 화면에서 확인하십시오.

## 화면 구성

- **Serial Port**: COM port, baudrate, timeout, Open/Close, 포트 새로고침, 로그 Clear
- **Wi-Fi 설정**: AP, server, DHCP 및 static IPv4 설정의 Read/Write
- **Measurement 설정**: Reference, Offset, Resistance, Interval Time, RS485_ONLY의 Read/Write와 RTC software reset 주기 설정
- **MCU 수신 데이터**: 왼쪽에는 숫자 CSV 측정값, 오른쪽에는 STATUS·설정 응답·기타 frame을 분리 표시하며 auto scroll 지원

하단의 **측정값** 창과 **기타 MCU 데이터 / 상태** 창은 `SplitContainer(dataSplit)`로 나뉩니다. 두 창 사이의 회색 세로 분리선 위에 마우스를 올리면 좌우 크기 조절 커서가 나타나며, 분리선을 좌우로 끌어 각 창 너비를 조절할 수 있습니다. 양쪽 창은 최소 `150px` 너비를 유지합니다. 분리선을 놓으면 현재 위치가 즉시 `settings.json`에 저장되고 다음 실행 때 복원됩니다.

수신 창의 맨 윗줄이 잘리던 원인은 `Dock=Fill`인 RichTextBox와 `Dock=Top`인 제목 Label이 같은 Panel 안에서 겹칠 수 있었기 때문입니다. 현재는 Monitor header와 데이터 영역, 그리고 각 제목과 RichTextBox를 각각 2행 `TableLayoutPanel`로 분리했습니다. 제목은 AutoSize 행에, 수신 데이터는 나머지 100% 행에 들어가므로 첫 번째 수신 줄이 제목 아래에서 시작하며 가려지지 않습니다.

화면 크기는 다음 세 가지 방법으로 조절할 수 있습니다. Main 화면의 가로 분리선을 위아래로 움직이면 전체 MCU 수신 Panel 높이가 바뀌고, 측정값/기타 데이터 사이의 세로 분리선을 좌우로 움직이면 두 로그의 너비가 바뀝니다. 더 세밀한 조정은 **화면 배치** 편집기에서 `dataSplit`, `measurementLog`, `otherLog`, `monitorLayout` 등을 선택한 뒤 PropertyGrid 또는 파란 선택 테두리로 수행할 수 있습니다. 구조화된 자동 배치를 유지하려면 TableLayoutPanel 자체는 `자유 배치`하지 말고 `SplitterDistance`, Margin, Font 등의 속성만 변경하는 것을 권장합니다.

Monitor 상단에는 STATUS 오른쪽에 간격을 두고 MAC Address가 표시됩니다. MCU가 `<STX>MAC_mac address<CR><LF>` 형식으로 전송하면 수신 frame 원문은 **기타 MCU 데이터 / 상태** 창에 그대로 남기고, `MAC_` 뒤의 값만 MAC Address 값 영역에도 별도로 표시합니다. 예를 들어 `<STX>MAC_AA:BB:CC:DD:EE:FF<CR><LF>`는 수신 창에 `MAC_AA:BB:CC:DD:EE:FF`로 기록되는 동시에 MAC 값 영역에 `AA:BB:CC:DD:EE:FF`로 표시됩니다. 호환성을 위해 `MAC,값`, `MAC:값`, `MAC=값`, `MAC_ADDRESS_값` 형식도 인식합니다. Auto scroll, `STATUS`, `MAC Address` 제목은 Windows 기본 Control 배경색을 사용하고, MCU에서 받은 STATUS/MAC **값 영역만** 흰색 배경과 검정색 `FixedSingle` 테두리를 사용합니다. Clear 버튼은 두 값 영역을 `-`로 초기화합니다.

Wi-Fi **Read** 버튼은 COM port가 닫혀 있을 때도 화면에서 활성 상태를 유지합니다. 이때 누르면 먼저 port를 열라는 안내가 표시되며, port가 열려 있으면 `<STX>WIFI_R_ALL<CR><LF>`를 전송합니다. 이전 WYSIWYG 배치 파일에 비활성 상태나 화면 밖 위치가 남아 발생하는 문제를 막기 위해 앱 시작 시 Read 버튼 활성 상태와 STATUS/MAC header 배치를 다시 확인합니다.

### RTC software reset 주기

Measurement 설정 창의 **RTC Reset Period (sec)**에 초 단위 주기를 입력합니다. `0`은 자동 reset OFF이고 최대값은 `31536000`초(365일)입니다.

- **Reset Read**: `<STX>RESET_R_ALL<CR><LF>` 전송
- **Reset Write**: `<STX>RESET_W_ALL,seconds<CR><LF>` 전송
- MCU 응답: `<STX>RESET_R_ALL,seconds<CR><LF>`

응답값은 MCU 수신 데이터 창으로 보내지 않고 RTC Reset Period 입력란에 표시되며 PC 설정 파일에도 저장됩니다. 실제 reset은 STM32 펌웨어의 RTC wake-up timer가 수행합니다.

STATUS/MAC 값 영역의 `BackColor=White`, `ForeColor=Black`, `BorderStyle=FixedSingle`은 생성 시와 사용자 배치 복원 직후 다시 강제 적용됩니다. 따라서 이전 `control-layout.json`에 다른 색상이 저장되어 있어도 흰색 배경과 검정색 글자/테두리가 유지됩니다. 화면 배치 편집기 역시 이 두 값 영역의 저장 색상은 시작 시 적용하지 않습니다.

MAC Address의 `Location`을 직접 바꾸면 원위치로 돌아가는 이유는 layout container가 자식 위치를 자동 계산하기 때문입니다. `TableLayoutPanel(headerLayout)`에서 STATUS 값과 MAC Address 제목 사이에 폭 `80px`의 빈 열을 두었습니다. 간격을 바꾸려면 MonitorPanel Designer에서 `headerLayout`을 선택하고 **Edit Rows and Columns → 네 번째 열(Absolute)의 Width**를 변경하십시오. MAC label의 `Location`을 직접 수정할 필요가 없습니다.

Designer에서 `if (value < control.Minimum)` 또는 `Math.Min(control.Maximum, Math.Max(control.Minimum, value))`가 표시되는 것은 Measurement/Reset 저장값을 `NumericUpDown`의 허용 범위로 제한하는 코드가 호출 스택에 잡힌 것입니다. 소스 편집기가 비교문을 강조하더라도 실제 원인은 바로 뒤의 `NumericUpDown.Value` 대입이나 Designer의 실행용 설정 복원일 수 있습니다. 범위 제한 계산은 값이 Minimum보다 작으면 Minimum, Maximum보다 크면 Maximum을 선택한다는 뜻입니다.

`RESET_R_ALL`/`RESET_W_ALL` 문자열을 수정한 것 자체는 `NumericUpDown` 범위 예외의 직접 원인이 아닙니다. 이 예외는 이전 `settings.json`에 남은 값, MCU가 보낸 범위 밖의 값, 또는 Visual Studio Designer가 실행용 설정 복원 코드를 호출할 때 주로 발생합니다. 현재 코드는 VS 2022의 `devenv` 및 `DesignToolsServer` process를 디자인 모드로 판별하여 Designer에서는 설정 파일을 읽지 않으며, 실행 중에는 Minimum/Maximum을 먼저 읽은 뒤 유효 범위로 제한한 값만 `NumericUpDown.Value`에 적용합니다. 문제가 지속되면 앱을 종료하고 `%LOCALAPPDATA%\STM32MeasurementMonitor\settings.json`을 삭제한 뒤 다시 실행하면 저장값이 기본값으로 초기화됩니다.

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
