import serial
import time

# 사용자의 환경에 맞는 시리얼 포트 이름을 입력해주세요.
# Windows: "COM3", "COM4" 등
# Linux: "/dev/ttyACM0", "/dev/ttyUSB0" 등
# macOS: "/dev/cu.usbmodemXXXXX" 등
SERIAL_PORT = "COM3"  # <--- 이 부분을 수정하세요.
BAUD_RATE = 115200

def main():
    print(f"'{SERIAL_PORT}'에 연결을 시도합니다. 속도: {BAUD_RATE}bps")

    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print("연결 성공. 1초마다 메시지를 전송합니다. (Ctrl+C로 종료)")
    except serial.SerialException as e:
        print(f"오류: 시리얼 포트를 열 수 없습니다. '{SERIAL_PORT}'가 올바른 포트인지, 다른 프로그램에서 사용 중이지 않은지 확인하세요.")
        print(f"상세 정보: {e}")
        return

    try:
        while True:
            message = b"Hello, RP2040!\n"
            ser.write(message)
            print(f"전송: {message.decode().strip()}")
            time.sleep(1)
    except KeyboardInterrupt:
        print("\n프로그램 종료.")
    finally:
        ser.close()

if __name__ == "__main__":
    main()
