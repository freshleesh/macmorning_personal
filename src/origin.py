import os
import dynamixel_sdk as dxl

# 제어하려는 모터들의 ID 설정
DXL_ID_1 = 1               
DXL_ID_5 = 5               

# 사용하려는 포트 설정 (예: COM3, ttyUSB0 등)
DEVICENAME = '/dev/ttyUSB1'

# 보드레이트 설정 (AX-12A는 1000000 bps를 사용합니다)
BAUDRATE = 1000000

# 통신 프로토콜 버전 (AX-12A는 프로토콜 1.0을 사용합니다)
PROTOCOL_VERSION = 1.0

# 제어 테이블 주소
ADDR_MX_TORQUE_ENABLE = 24
ADDR_MX_GOAL_POSITION = 30
ADDR_MX_PRESENT_POSITION = 36

TORQUE_ENABLE = 1  # 토크를 켜기 위한 값
TORQUE_DISABLE = 0 # 토크를 끄기 위한 값

# 목표 위치 (0~1023 범위)
DXL_MINIMUM_POSITION_VALUE = 0  
DXL_MAXIMUM_POSITION_VALUE = 0  
DXL_MOVING_STATUS_THRESHOLD = 10  

# 패킷핸들러 및 포트핸들러 초기화
portHandler = dxl.PortHandler(DEVICENAME)
packetHandler = dxl.PacketHandler(PROTOCOL_VERSION)

# 포트를 열고 보드레이트를 설정합니다.
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

# 두 모터에 토크를 인가합니다.
for motor_id in [DXL_ID_1, DXL_ID_5]:
    dxl_comm_result, dxl_error = packetHandler.write1ByteTxRx(portHandler, motor_id, ADDR_MX_TORQUE_ENABLE, TORQUE_ENABLE)
    if dxl_comm_result != dxl.COMM_SUCCESS:
        print("ID %d: %s" % (motor_id, packetHandler.getTxRxResult(dxl_comm_result)))
    elif dxl_error != 0:
        print("ID %d: %s" % (motor_id, packetHandler.getRxPacketError(dxl_error)))
    else:
        print("ID %d 모터 토크 인가됨" % motor_id)

# 목표 위치로 두 모터를 각각 이동시키기
goal_position_1 = DXL_MINIMUM_POSITION_VALUE  # ID 1 모터의 목표 위치
goal_position_5 = DXL_MAXIMUM_POSITION_VALUE  # ID 5 모터의 목표 위치

dxl_comm_result, dxl_error = packetHandler.write2ByteTxRx(portHandler, DXL_ID_1, ADDR_MX_GOAL_POSITION, goal_position_1)
if dxl_comm_result != dxl.COMM_SUCCESS:
    print("ID 1: %s" % packetHandler.getTxRxResult(dxl_comm_result))
elif dxl_error != 0:
    print("ID 1: %s" % packetHandler.getRxPacketError(dxl_error))

dxl_comm_result, dxl_error = packetHandler.write2ByteTxRx(portHandler, DXL_ID_5, ADDR_MX_GOAL_POSITION, goal_position_5)
if dxl_comm_result != dxl.COMM_SUCCESS:
    print("ID 5: %s" % packetHandler.getTxRxResult(dxl_comm_result))
elif dxl_error != 0:
    print("ID 5: %s" % packetHandler.getRxPacketError(dxl_error))

# 현재 위치 읽기
present_position_1, dxl_comm_result, dxl_error = packetHandler.read2ByteTxRx(portHandler, DXL_ID_1, ADDR_MX_PRESENT_POSITION)
if dxl_comm_result != dxl.COMM_SUCCESS:
    print("ID 1: %s" % packetHandler.getTxRxResult(dxl_comm_result))
elif dxl_error != 0:
    print("ID 1: %s" % packetHandler.getRxPacketError(dxl_error))

present_position_5, dxl_comm_result, dxl_error = packetHandler.read2ByteTxRx(portHandler, DXL_ID_5, ADDR_MX_PRESENT_POSITION)
if dxl_comm_result != dxl.COMM_SUCCESS:
    print("ID 5: %s" % packetHandler.getTxRxResult(dxl_comm_result))
elif dxl_error != 0:
    print("ID 5: %s" % packetHandler.getRxPacketError(dxl_error))

print("[ID:%03d] 목표 위치:%03d  현재 위치:%03d" % (DXL_ID_1, goal_position_1, present_position_1))
print("[ID:%03d] 목표 위치:%03d  현재 위치:%03d" % (DXL_ID_5, goal_position_5, present_position_5))

# 포트 닫기
portHandler.closePort()
