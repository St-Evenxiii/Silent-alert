from skidl import *

# Define the components for the Main Board (Inside Unit)
# Because SKiDL relies on local KiCad libraries that may be missing in headless envs,
# we'll use generic Part definitions to create a clean schematic netlist.
esp32 = Part(lib="Device", name="MCU", dest="U1")
esp32.add_pins(
    Pin(num='1', name='GND', func=Pin.PWRIN),
    Pin(num='2', name='3V3', func=Pin.PWRIN),
    Pin(num='38', name='GND', func=Pin.PWRIN),
    Pin(num='27', name='IO16_RX2', func=Pin.BIDIR),
    Pin(num='28', name='IO17_TX2', func=Pin.BIDIR),
    Pin(num='26', name='IO4_LED', func=Pin.BIDIR)
)

rs485 = Part(lib="Device", name="MAX485E", dest="U2")
rs485.add_pins(
    Pin(num='1', name='RO', func=Pin.OUTPUT),
    Pin(num='2', name='RE_N', func=Pin.INPUT),
    Pin(num='3', name='DE', func=Pin.INPUT),
    Pin(num='4', name='DI', func=Pin.INPUT),
    Pin(num='5', name='GND', func=Pin.PWRIN),
    Pin(num='6', name='A', func=Pin.BIDIR),
    Pin(num='7', name='B', func=Pin.BIDIR),
    Pin(num='8', name='VCC', func=Pin.PWRIN)
)

buck_converter = Part(lib="Connector", name="Conn_01x04_Male", dest="J1")
buck_converter.add_pins(
    Pin(num='1', name='IN+', func=Pin.PASSIVE),
    Pin(num='2', name='IN-', func=Pin.PASSIVE),
    Pin(num='3', name='OUT+', func=Pin.PASSIVE),
    Pin(num='4', name='OUT-', func=Pin.PASSIVE)
)

charger = Part(lib="Connector", name="Conn_01x04_Male", dest="J2")
charger.add_pins(
    Pin(num='1', name='IN+', func=Pin.PASSIVE),
    Pin(num='2', name='IN-', func=Pin.PASSIVE),
    Pin(num='3', name='BAT+', func=Pin.PASSIVE),
    Pin(num='4', name='BAT-', func=Pin.PASSIVE)
)

bms = Part(lib="Connector", name="Conn_01x06_Male", dest="J3")
bms.add_pins(
    Pin(num='1', name='B+', func=Pin.PASSIVE),
    Pin(num='2', name='BM', func=Pin.PASSIVE),
    Pin(num='3', name='B-', func=Pin.PASSIVE),
    Pin(num='4', name='P+', func=Pin.PASSIVE),
    Pin(num='5', name='P-', func=Pin.PASSIVE),
    Pin(num='6', name='CHG', func=Pin.PASSIVE)
)

# Connectors (Screw Terminals)
term_battery = Part(lib="Connector", name="Screw_Terminal_01x02", dest="J4")
term_battery.add_pins(Pin(num='1', name='Pin_1', func=Pin.PASSIVE), Pin(num='2', name='Pin_2', func=Pin.PASSIVE))

term_sensor = Part(lib="Connector", name="Screw_Terminal_01x04", dest="J5")
term_sensor.add_pins(Pin(num='1', name='Pin_1', func=Pin.PASSIVE), Pin(num='2', name='Pin_2', func=Pin.PASSIVE), Pin(num='3', name='Pin_3', func=Pin.PASSIVE), Pin(num='4', name='Pin_4', func=Pin.PASSIVE))

term_leds = Part(lib="Connector", name="Screw_Terminal_01x03", dest="J6")
term_leds.add_pins(Pin(num='1', name='Pin_1', func=Pin.PASSIVE), Pin(num='2', name='Pin_2', func=Pin.PASSIVE), Pin(num='3', name='Pin_3', func=Pin.PASSIVE))

# Power Nets
vbat = Net("VBAT") # Raw battery voltage (approx 7.4V - 8.4V)
vcc_5v = Net("5V") # Regulated 5V
vcc_3v3 = Net("3.3V") # Regulated 3.3V (from ESP32)
gnd = Net("GND")

# Signal Nets
rs485_a = Net("RS485_A")
rs485_b = Net("RS485_b")
uart_rx = Net("UART_RX")
uart_tx = Net("UART_TX")
led_data = Net("LED_DATA")

# --- Connections ---

# Battery Terminal -> BMS P+/P-
term_battery[1] += vbat
term_battery[2] += gnd
bms[4] += vbat # P+
bms[5] += gnd  # P-

# Battery Power -> Buck Converter IN
buck_converter[1] += vbat # IN+
buck_converter[2] += gnd  # IN-
buck_converter[3] += vcc_5v # OUT+
buck_converter[4] += gnd  # OUT-

# 5V Power -> ESP32 Vin
esp32[2] += vcc_3v3 # VDD33

# ESP32 Pins
esp32[1] += gnd # GND
esp32[38] += gnd # GND
esp32[27] += uart_rx # IO16 (RX2)
esp32[28] += uart_tx # IO17 (TX2)
esp32[26] += led_data # IO4 (LED)

# RS485 Connections
rs485[8] += vcc_3v3 # VCC
rs485[5] += gnd # GND
rs485[1] += uart_rx # RO
rs485[4] += uart_tx # DI
# Receiver Only Mode
rs485[2] += gnd # /RE = Low
rs485[3] += gnd # DE = Low

rs485[6] += rs485_a # A
rs485[7] += rs485_b # B

# Sensor Terminal Connections
term_sensor[1] += vcc_5v # 5V Power for outside unit
term_sensor[2] += gnd
term_sensor[3] += rs485_a
term_sensor[4] += rs485_b

# LED Terminal Connections
term_leds[1] += vcc_5v # 5V for NeoPixels
term_leds[2] += led_data # Data signal
term_leds[3] += gnd # GND

# Generate Netlist
generate_netlist(file_="hardware/main_board/main_board.net")
print("Main Board Netlist Generated.")
