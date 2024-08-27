import dynamixel_sdk as dxl
import time

# 모터 ID 설정
DXL_ID_1 = 1               
DXL_ID_5 = 5               

# 사용 포트 설정
DEVICENAME = '/dev/ttyUSB1'
BAUDRATE = 1000000
PROTOCOL_VERSION = 1.0

# 제어 테이블 주소
ADDR_MX_TORQUE_ENABLE = 24
ADDR_MX_CW_ANGLE_LIMIT = 6
ADDR_MX_CCW_ANGLE_LIMIT = 8
ADDR_MX_MOVING_SPEED = 32
ADDR_MX_PRESENT_POSITION = 36

TORQUE_ENABLE = 1  
TORQUE_DISABLE = 0 
CW_ANGLE_LIMIT = 0
CCW_ANGLE_LIMIT = 0

# 패킷핸들러 및 포트핸들러 초기화
portHandler = dxl.PortHandler(DEVICENAME)
packetHandler = dxl.PacketHandler(PROTOCOL_VERSION)

# 포트 열기
if portHandler.openPort():
    print("포트 열기 성공")
else:
    print("포트 열기 실패")
    quit()

if portHandler.setBaudRate(BAUDRATE):
    print("보드레이트 설정 성공")
else:
    print("보드레이트 설정 실패")
    quit()

# 모터에 토크 인가
for motor_id in [DXL_ID_1, DXL_ID_5]:
    packetHandler.write1ByteTxRx(portHandler, motor_id, ADDR_MX_TORQUE_ENABLE, TORQUE_ENABLE)

# 연속 회전 모드 설정
for motor_id in [DXL_ID_1, DXL_ID_5]:
    packetHandler.write2ByteTxRx(portHandler, motor_id, ADDR_MX_CW_ANGLE_LIMIT, CW_ANGLE_LIMIT)
    packetHandler.write2ByteTxRx(portHandler, motor_id, ADDR_MX_CCW_ANGLE_LIMIT, CCW_ANGLE_LIMIT)

# 모터 속도 설정 (양수: 시계 방향, 음수: 반시계 방향)
moving_speed =  + 200  # 시계 방향으로 회전
for motor_id in [DXL_ID_1, DXL_ID_5]:
    packetHandler.write2ByteTxRx(portHandler, motor_id, ADDR_MX_MOVING_SPEED, moving_speed)

# 원하는 회전수만큼 돌리기
desired_revolutions = 2  # 2바퀴 회전

# 회전수 추적 변수 초기화
revolutions_1 = 0
revolutions_5 = 0

last_position_1, _, _ = packetHandler.read2ByteTxRx(portHandler, DXL_ID_1, ADDR_MX_PRESENT_POSITION)
last_position_5, _, _ = packetHandler.read2ByteTxRx(portHandler, DXL_ID_5, ADDR_MX_PRESENT_POSITION)

while True:
    current_position_1, _, _ = packetHandler.read2ByteTxRx(portHandler, DXL_ID_1, ADDR_MX_PRESENT_POSITION)
    current_position_5, _, _ = packetHandler.read2ByteTxRx(portHandler, DXL_ID_5, ADDR_MX_PRESENT_POSITION)

    # 1023에서 0으로 넘어가는 경우
    if current_position_1 < last_position_1 and last_position_1 - current_position_1 > 512:
        revolutions_1 += 1
        print("jump")
    # 0에서 1023으로 넘어가는 경우
    elif current_position_1 > last_position_1 and current_position_1 - last_position_1 > 512:
        revolutions_1 -= 1

    # 1023에서 0으로 넘어가는 경우
    if current_position_5 < last_position_5 and last_position_5 - current_position_5 > 512:
        revolutions_5 += 1
    # 0에서 1023으로 넘어가는 경우
    elif current_position_5 > last_position_5 and current_position_5 - last_position_5 > 512:
        revolutions_5 -= 1

    # 업데이트된 위치로 총 회전수 계산
    total_revolutions_1 = revolutions_1 + current_position_1 / 1023.0
    total_revolutions_5 = revolutions_5 + current_position_5 / 1023.0

    if total_revolutions_1 >= desired_revolutions:
        packetHandler.write2ByteTxRx(portHandler, DXL_ID_1, ADDR_MX_MOVING_SPEED, 0)

    if total_revolutions_5 >= desired_revolutions:
        packetHandler.write2ByteTxRx(portHandler, DXL_ID_5, ADDR_MX_MOVING_SPEED, 0)

    if total_revolutions_1 >= desired_revolutions and total_revolutions_5 >= desired_revolutions:
        break

    print(total_revolutions_1, total_revolutions_5)
    last_position_1 = current_position_1
    last_position_5 = current_position_5

    

    time.sleep(0.1)  # 모니터링 주기

# 모터 멈추기
for motor_id in [DXL_ID_1, DXL_ID_5]:
    packetHandler.write2ByteTxRx(portHandler, motor_id, ADDR_MX_MOVING_SPEED, 0)

# 포트 닫기
portHandler.closePort()
