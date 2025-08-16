import sys
import struct
import serial
import serial.tools.list_ports
from PyQt5.QtWidgets import QApplication, QMainWindow, QWidget, QVBoxLayout, QGridLayout, QLabel, QPushButton, QComboBox, QHBoxLayout
from PyQt5.QtCore import Qt, QThread, pyqtSignal, QTimer
from inputs import get_gamepad, UnpluggedError

# (ControllerThread class is unchanged)
class ControllerThread(QThread):
    controller_event = pyqtSignal(object)
    status_update = pyqtSignal(str)
    def __init__(self):
        super().__init__()
        self._running = True
    def run(self):
        while self._running:
            try:
                self.status_update.emit("컨트롤러를 찾는 중...")
                events = get_gamepad()
                self.status_update.emit("DS4 컨트롤러가 연결되었습니다.")
                for event in events:
                    if not self._running: break
                    self.controller_event.emit(event)
            except UnpluggedError:
                self.status_update.emit("컨트롤러 연결이 끊겼습니다. 다시 연결해주세요...")
            except Exception as e:
                self.status_update.emit(f"오류 발생: {e}. 1초 후 다시 시도합니다.")
                self.msleep(1000)
    def stop(self):
        self._running = False


class GamepadGui(QMainWindow):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("DS4-over-UART v0.2")
        self.setGeometry(100, 100, 800, 600)

        self.gamepad_state = {
            # Sticks and Triggers
            "ABS_X": 0, "ABS_Y": 0, "ABS_RX": 0, "ABS_RY": 0,
            "ABS_Z": 0, "ABS_RZ": 0,
            # Buttons
            "BTN_SOUTH": 0, "BTN_EAST": 0, "BTN_WEST": 0, "BTN_NORTH": 0,
            # D-Pad
            "ABS_HAT0X": 0, "ABS_HAT0Y": 0,
            # Motion Sensors
            "ABS_HAT2X": 0, "ABS_HAT2Y": 0, "ABS_HAT2Z": 0, # Gyro
            "ABS_HAT3X": 0, "ABS_HAT3Y": 0, "ABS_HAT3Z": 0, # Accel
        }

        # --- 시리얼 통신 설정 ---
        self.serial = serial.Serial()
        self.send_timer = QTimer(self)
        self.send_timer.timeout.connect(self.send_gamepad_data)

        # --- UI 구성 ---
        self.central_widget = QWidget()
        self.setCentralWidget(self.central_widget)
        main_layout = QVBoxLayout()
        self.central_widget.setLayout(main_layout)

        # 시리얼 포트 선택 UI
        serial_layout = QHBoxLayout()
        self.port_combobox = QComboBox()
        self.refresh_ports()
        self.connect_button = QPushButton("연결")
        self.connect_button.clicked.connect(self.toggle_serial_connection)
        serial_layout.addWidget(QLabel("시리얼 포트:"))
        serial_layout.addWidget(self.port_combobox)
        serial_layout.addWidget(self.connect_button)
        main_layout.addLayout(serial_layout)

        self.status_label = QLabel("컨트롤러와 시리얼 포트를 연결하세요.")
        self.status_label.setAlignment(Qt.AlignCenter)
        main_layout.addWidget(self.status_label)

        grid_layout = QGridLayout()
        main_layout.addLayout(grid_layout)

        self.ui_elements = {}
        # Sticks and Triggers
        self.add_label_to_grid(grid_layout, "Left Stick X:", 0, 0, "ABS_X")
        self.add_label_to_grid(grid_layout, "Left Stick Y:", 1, 0, "ABS_Y")
        self.add_label_to_grid(grid_layout, "Right Stick X:", 0, 2, "ABS_RX")
        self.add_label_to_grid(grid_layout, "Right Stick Y:", 1, 2, "ABS_RY")
        self.add_label_to_grid(grid_layout, "L2 Trigger:", 2, 0, "ABS_Z")
        self.add_label_to_grid(grid_layout, "R2 Trigger:", 2, 2, "ABS_RZ")
        self.add_label_to_grid(grid_layout, "Buttons:", 3, 0, "BUTTONS")
        # Gyro
        self.add_label_to_grid(grid_layout, "Gyro X:", 0, 4, "ABS_HAT2X")
        self.add_label_to_grid(grid_layout, "Gyro Y:", 1, 4, "ABS_HAT2Y")
        self.add_label_to_grid(grid_layout, "Gyro Z:", 2, 4, "ABS_HAT2Z")
        # Accelerometer
        self.add_label_to_grid(grid_layout, "Accel X:", 3, 4, "ABS_HAT3X")
        self.add_label_to_grid(grid_layout, "Accel Y:", 4, 4, "ABS_HAT3Y")
        self.add_label_to_grid(grid_layout, "Accel Z:", 5, 4, "ABS_HAT3Z")
        # D-Pad
        self.add_label_to_grid(grid_layout, "D-Pad X/Y:", 4, 0, "DPAD")


        self.controller_thread = ControllerThread()
        self.controller_thread.status_update.connect(self.update_status_label)
        self.controller_thread.controller_event.connect(self.handle_controller_event)
        self.controller_thread.start()

    def add_label_to_grid(self, grid, label_text, row, col, code):
        label = QLabel(label_text)
        value_label = QLabel("0")
        grid.addWidget(label, row, col)
        grid.addWidget(value_label, row, col + 1)
        self.ui_elements[code] = value_label

    def refresh_ports(self):
        self.port_combobox.clear()
        ports = serial.tools.list_ports.comports()
        for port in ports:
            self.port_combobox.addItem(port.device)

    def toggle_serial_connection(self):
        if self.serial.is_open:
            self.send_timer.stop()
            self.serial.close()
            self.connect_button.setText("연결")
            self.status_label.setText("시리얼 포트 연결 해제됨.")
        else:
            port_name = self.port_combobox.currentText()
            if not port_name:
                self.status_label.setText("사용 가능한 시리얼 포트가 없습니다.")
                return
            try:
                self.serial.port = port_name
                self.serial.baudrate = 230400
                self.serial.open()
                self.send_timer.start(1) # 1ms 간격 (1000Hz)
                self.connect_button.setText("연결 해제")
                self.status_label.setText(f"{port_name}에 연결됨. 데이터 전송 중 (1000Hz)...")
            except serial.SerialException as e:
                self.status_label.setText(f"연결 실패: {e}")

    def send_gamepad_data(self):
        if not self.serial.is_open:
            return

        # 버튼 상태를 16비트 정수로 패킹
        button_mask = 0
        # Face buttons (bits 0-3)
        if self.gamepad_state.get('BTN_SOUTH', 0): button_mask |= (1 << 0)
        if self.gamepad_state.get('BTN_EAST', 0):  button_mask |= (1 << 1)
        if self.gamepad_state.get('BTN_WEST', 0):  button_mask |= (1 << 2)
        if self.gamepad_state.get('BTN_NORTH', 0): button_mask |= (1 << 3)

        # D-Pad (bits 4-7)
        hat_y = self.gamepad_state.get('ABS_HAT0Y', 0)
        hat_x = self.gamepad_state.get('ABS_HAT0X', 0)
        if hat_y == -1: button_mask |= (1 << 4) # Up
        if hat_y == 1:  button_mask |= (1 << 5) # Down
        if hat_x == -1: button_mask |= (1 << 6) # Left
        if hat_x == 1:  button_mask |= (1 << 7) # Right

        # 페이로드의 각 부분을 개별적으로 패킹
        stick_trigger_payload = struct.pack('<hhhhBB',
            self.gamepad_state.get('ABS_X', 0), -self.gamepad_state.get('ABS_Y', 0),
            self.gamepad_state.get('ABS_RX', 0), -self.gamepad_state.get('ABS_RY', 0),
            self.gamepad_state.get('ABS_Z', 0), self.gamepad_state.get('ABS_RZ', 0)
        )
        button_payload = struct.pack('<H', button_mask) # <H for unsigned short (16-bit)
        motion_payload = struct.pack('<hhhhhh',
            self.gamepad_state.get('ABS_HAT2X', 0), self.gamepad_state.get('ABS_HAT2Y', 0), self.gamepad_state.get('ABS_HAT2Z', 0),
            self.gamepad_state.get('ABS_HAT3X', 0), self.gamepad_state.get('ABS_HAT3Y', 0), self.gamepad_state.get('ABS_HAT3Z', 0)
        )

        # 전체 페이로드 조합 (sticks/triggers + buttons + motion)
        payload = stick_trigger_payload + button_payload + motion_payload

        # 체크섬 계산
        checksum = sum(payload) & 0xFF

        # 최종 패킷 생성
        packet = b'\xAA' + struct.pack('<B', len(payload)) + payload + struct.pack('<B', checksum) + b'\x55'

        try:
            self.serial.write(packet)
        except serial.SerialException:
            self.toggle_serial_connection()
            self.status_label.setText("쓰기 오류! 포트 연결을 해제합니다.")


    def update_status_label(self, text):
        self.status_label.setText(text)

    def handle_controller_event(self, event):
        if event.ev_type == 'Sync': return

        if event.code in self.gamepad_state:
            self.gamepad_state[event.code] = event.state
            if event.code in self.ui_elements:
                 self.ui_elements[event.code].setText(str(event.state))
            # 버튼들은 하나의 라벨로 표시
            if event.code.startswith('BTN_'):
                btn_str = f"X:{self.gamepad_state['BTN_SOUTH']} O:{self.gamepad_state['BTN_EAST']} []:{self.gamepad_state['BTN_WEST']} ^:{self.gamepad_state['BTN_NORTH']}"
                self.ui_elements['BUTTONS'].setText(btn_str)

            # D-Pad 값 표시
            if event.code.startswith('ABS_HAT0'):
                dpad_str = f"X:{self.gamepad_state.get('ABS_HAT0X', 0)} Y:{self.gamepad_state.get('ABS_HAT0Y', 0)}"
                if 'DPAD' in self.ui_elements:
                    self.ui_elements['DPAD'].setText(dpad_str)

    def closeEvent(self, event):
        print("애플리케이션 종료 중...")
        self.controller_thread.stop()
        self.controller_thread.wait()
        if self.serial.is_open:
            self.send_timer.stop()
            self.serial.close()
        event.accept()

def main():
    print("GUI를 시작합니다.")
    print("필요한 라이브러리가 설치되어 있는지 확인하세요: pip install -r requirements.txt")

    app = QApplication(sys.argv)
    window = GamepadGui()
    window.show()
    sys.exit(app.exec_())

if __name__ == "__main__":
    main()
