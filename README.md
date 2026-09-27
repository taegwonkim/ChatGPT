# WinForms TextBox를 마우스로 이동하고 크기 조절하기

이 예제는 두 가지 상황을 구분합니다.

## 1. Visual Studio 디자이너에서만 조절할 때

폼 디자이너에서 `TextBox`를 선택한 뒤 **Properties** 창에서 다음 값을 설정합니다.

| 속성 | 예제 값 | 의미 |
| --- | --- | --- |
| `Location` | `120, 90` | 폼 안의 초기 위치 |
| `Size` | `260, 100` | 초기 너비와 높이 |
| `Multiline` | `True` | 한 줄 TextBox의 높이 제한을 없앰 |
| `Anchor` | `None` | 폼 크기에 따라 위치/크기가 자동 변경되지 않게 함 |
| `MinimumSize` | `80, 40` | 너무 작게 줄어드는 것을 방지 |

`Multiline = true`가 중요합니다. WinForms의 한 줄 `TextBox`는 글꼴과 테두리에 맞춰 높이가 제한되므로 디자이너에서 위아래 크기 조절이 자유롭지 않습니다. 위 속성은 `Form1.Designer.cs`의 `editableTextBox` 설정에도 들어 있습니다.

> `Form1.Designer.cs`는 디자이너가 다시 생성할 수 있으므로 일반적으로 직접 수정하지 말고 Properties 창을 사용합니다.

## 2. 실행 중 사용자가 마우스로 조절할 때

디자이너 속성만으로는 실행 중 드래그 이동/크기 변경 기능이 생기지 않습니다. 이 샘플은 다음 코드를 추가했습니다.

- TextBox 본문을 왼쪽 버튼으로 드래그: 위치 이동
- 오른쪽 아래 파란 손잡이를 드래그: 크기 변경
- 폼 바깥으로 나가지 않도록 `Math.Clamp`로 범위 제한
- `MinimumSize`보다 작아지지 않도록 제한

텍스트를 선택하거나 편집하는 기본 마우스 동작도 유지해야 하는 실제 편집기라면, 본문 전체를 이동 영역으로 쓰는 대신 별도의 제목 표시줄 또는 이동 손잡이를 두는 방식을 권장합니다.

## 실행

Windows에서 .NET 8 SDK를 설치한 뒤 실행합니다.

```powershell
dotnet run --project .\MouseEditableTextBoxDemo\MouseEditableTextBoxDemo.csproj
```
