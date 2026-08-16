# 통신 프로토콜

모든 PC 명령은 ASCII 한 줄이며 `\r\n`으로 끝난다. 한 줄은 최대 255 byte이다.

## PC 명령

```text
SET SSID=factory-ap
SET PASSWORD=secret
SET SERVER_IP=192.168.10.20
SET SERVER_PORT=50001
SET DHCP=0
SET MODULE_IP=192.168.10.50
SET GATEWAY=192.168.10.1
SET NETMASK=255.255.255.0
GET CONFIG
SAVE
START
```

성공은 `OK\r\n`, 실패는 `ERR <reason>\r\n`이다. `GET CONFIG`는 각 항목을 한 줄로
응답한다. PASSWORD는 기본 응답에서 `********`로 가리고, 현장 provisioning 권한이
확인된 별도 명령이 아닌 한 평문을 돌려주지 않는다. DHCP=0이면 module IP/gateway/netmask
세 항목이 모두 유효해야 `SAVE`가 성공한다.

설정 변경은 RAM의 staging copy에 적용하고 `SAVE` 때 validation 후 flash에 commit한다.
USART3와 USB가 동시에 설정하더라도 `configMutex`로 transaction을 직렬화한다.

## FPGA 입력

MCU는 boot와 설정 load 완료 후 USART2로 정확히 한 번 `START\r\n`을 보낸다. FPGA는
trigger falling edge 후 다음 binary frame을 보낸다(두 장치에서 반드시 동일하게 구현).

| offset | size | 내용 |
|---:|---:|---|
| 0 | 2 | sync `0xA5 0x5A` |
| 2 | 2 | payload length, little endian (최대 480) |
| 4 | 4 | FPGA sequence, little endian |
| 8 | N | 6 channel samples/interleaved peak data |
| 8+N | 4 | CRC32(IEEE), header+payload |

EXTI timestamp와 frame sequence를 결합한다. trigger 후 100 ms 안에 완전한 frame이 없으면
parser를 sync 탐색 상태로 되돌리고 timeout counter를 올린다. CRC/length 오류 frame은 PC와
서버로 전달하지 않는다.

## 서버 출력

TCP는 record 경계가 없으므로 각 record 앞에 4-byte big-endian record length를 붙인다.
record body에는 protocol version, device ID, MCU sequence, trigger timestamp(us), FPGA 원본
frame을 넣고 CRC32를 끝에 둔다. 서버는 sequence로 누락/중복을 판정해야 한다.

ESP AT state machine은 `RESET -> WAIT_READY -> CONFIG_IP -> JOIN_AP -> OPEN_TCP -> ONLINE`
순서다. `WIFI DISCONNECT`, `CLOSED`, send timeout 시 socket을 닫고 1, 2, 4, 8, 16, 30초
bounded exponential backoff로 재접속한다. 연결이 끊긴 동안 frame은 제한 queue에만
보관하고 queue가 가득 차면 오래된 frame부터 drop한다. 무한 저장은 하지 않는다.
