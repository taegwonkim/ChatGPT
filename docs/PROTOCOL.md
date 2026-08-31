# 통신 프로토콜

## PC 명령 (USART3 RS-485 / USB CDC 공통)

UTF-8이 아닌 ASCII line protocol이며 각 명령은 `\r\n`으로 끝납니다. 비밀번호는 응답에서 마스킹합니다.

```text
READ CONFIG
WRITE WIFI ssid=<percent-encoded> password=<percent-encoded>
WRITE SERVER ip=192.168.1.10 port=50001
WRITE NET dhcp=1
WRITE NET dhcp=0 ip=192.168.1.50 gateway=192.168.1.1 netmask=255.255.255.0
SAVE
STATUS
```

응답은 `OK ...\r\n` 또는 `ERR <code> <message>\r\n`입니다. SSID 최대 32 byte, password 최대 63 byte, IPv4만 허용하며 server port는 1..65535입니다. 설정 변경은 `SAVE` 전까지 RAM staging 영역에만 반영합니다. RS-485와 USB가 동시에 write할 수 있으므로 한 client가 write를 시작하면 30초 transaction lock을 두는 것을 권장합니다.

## FPGA 이진 프레임

FPGA RTL과 함께 확정할 권장 wire format입니다. 모든 다중 바이트 값은 little-endian입니다.

| 필드 | 크기 | 값 |
|---|---:|---|
| SOF | 2 | `0xA5 0x5A` |
| version | 1 | `1` |
| channel count | 1 | `6` |
| sample count/channel | 2 | N (최대 256 예시) |
| FPGA sequence | 4 | 증가값 |
| samples | `6*N*2` | channel-major signed 16-bit ADC samples |
| peak | 12 | 채널별 signed 16-bit peak |
| CRC32 | 4 | version부터 peak까지 IEEE CRC-32 |

EXTI falling edge는 “프레임이 곧 시작됨” 알림이며 데이터 경계 자체는 SOF/길이/CRC로 판단합니다. trigger 후 timeout(예: 200 ms), 비정상 길이, CRC 오류 시 현재 프레임을 폐기하고 다음 SOF로 resync합니다.

## 서버 payload

TCP 연결 후 각 레코드를 `SDV1` magic, device id, MCU sequence, timestamp(ms), FPGA payload length, FPGA frame, CRC32로 감쌉니다. TCP는 message boundary를 보존하지 않으므로 length가 필수입니다. 서버 ACK(`ACK <sequence>\r\n`)이 필요한 경우 제한된 재전송 queue와 중복 제거 sequence를 구현하십시오. 보안이 필요한 제품은 ESP AT 펌웨어의 TLS command와 인증서 검증을 사용해야 하며 평문 TCP에 자격 증명을 싣지 않습니다.
