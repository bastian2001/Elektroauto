# Pin assignments

<table>
  <tr>
    <td><b>Function</b></td>
    <td><b>ESP32 Pin</b></td>
    <td><b>Outside Connection</b></td>
  </tr>
  <tr>
    <td>Supply Voltage</td>
    <td>5V/3V3 depending on converter</td>
    <td>OUT+ of  buck converter</td>
  </tr>
  <tr>
    <td>Ground</td>
    <td>GND</td>
    <td>OUT- of buck converter</td>
  </tr>
  <tr>
    <td>USB-TX (reserved)</td>
    <td>1</td>
    <td>-none-</td>
  </tr>
  <tr>
    <td>USB-RX (reserved)</td>
    <td>3</td>
    <td>-none-</td>
  </tr>
  <tr>
    <td>LED_BUILTIN</td>
    <td>22</td>
    <td>-none-</td>
  </tr>
  <tr>
    <td>ESC 1 Output</td>
    <td>25</td>
    <td>ESC 1: S (Signal)</td>
  </tr>
  <tr>
    <td>ESC 1 Telemetry</td>
    <td>21 (RX1 remapped)</td>
    <td>ESC 1: T (Telemetry)</td>
  </tr>
  <tr>
    <td>ESC 2 Output</td>
    <td>27</td>
    <td>ESC 2: S (Signal)</td>
  </tr>
  <tr>
    <td>ESC 2 Telemetry</td>
    <td>16 (RX2)</td>
    <td>ESC 2: T (Telemetry)</td>
  </tr>
  <tr>
    <td>BMI160 - MISO</td>
    <td>19</td>
    <td>BMI160: SA0</td>
  </tr>
  <tr>
    <td>BMI160 - MOSI</td>
    <td>23</td>
    <td>BMI160: SDA</td>
  </tr>
  <tr>
    <td>BMI160 - CS</td>
    <td>5</td>
    <td>BMI160: CS</td>
  </tr>
  <tr>
    <td>BMI160 - SCL</td>
    <td>18</td>
    <td>BMI160: SCL</td>
  </tr>
  <tr>
    <td>BMI160 - GND</td>
    <td>GND</td>
    <td>BMI160: GND</td>
  </tr>
  <tr>
    <td>BMI160 - VCC</td>
    <td>3V3</td>
    <td>BMI160: 3V3</td>
  </tr>
  <tr>
    <td>Transmission indicator</td>
    <td>33</td>
    <td>-none-</td>
  </tr>
</table>

# Device -> Car protocol

- format: `[key]:[value]`
- set value
  - key: VALUE
  - value: [value] (0...2000, 0...2000, 0...20)
  - example: `VALUE:7`
- armed
  - key: ARMED
  - value: [YES alias [1, TRUE]|NO alias [0, FALSE]]
  - example: `ARMED:YES`
- ping
  - key: PING
  - value: none
- change mode
  - key: MODE
  - value: [THROTTLE alias 0|RPS alias 1|SLIP alias 2]
  - example: `MODE:SLIP`
- telemetry on/off
  - key: TELEMETRY
  - value: [ON alias 1|OFF alias 0]
  - example: `TELEMETRY:ON`
- device type
  - key: DEVICE
  - value: [WEB alias 0|APP alias 1]
  - example: `DEVICE:APP`
- race mode
  - key: RACEMODE
  - value: [OFF alias 0|ON alias 1]
  - example: `RACEMODE:ON`
- start race
  - key: STARTRACE
  - value: none
- set cutoff coltage
  - key: CUTOFFVOLTAGE
  - value: [new cutoff voltage in cV]
  - example: `CUTOFFVOLTAGE:600` for 6.0V
- set warning coltage
  - key: WARNINGVOLTAGE
  - value: [new warning voltage in cV]
  - example: `WARNINGVOLTAGE:720` for 7.2V
- set RPS control variables
  - keys: RPSA, RPSB, RPSC, PIDMULTIPLIER
  - value: [value]
- error count
  - key: ERRORCOUNT
  - value: none
- reconnect to WiFi
  - key: RECONNECT
  - value: none
- save settings
  - key: SAVESETTINGS alias SAVE
  - value: none
- read settings
  - key: READSETTINGS alias READ
  - value: none
- restore defaults
  - key: RESTOREDEFAULTS alias RESTORE
  - value: none
- send/print settings
  - key: SENDSETTINGS alias SETTINGS
  - value: none
- set maximum (target) values
  - keys: MAXTHROTTLE alias MAXT, MAXRPS alias MAXR, MAXSLIP alias MAXS
  - value: [value]
- set motorpole count
  - key: MOTORPOLE
  - value: [number of motor poles]
- set wheel diameter
  - key: WHEELDIAMETER alias WHEELDIA
  - value: [wheel diameter in mm]

# Car -> Device protocol

## Special response

- format: `[command] [...args]`
- set interface element
  - command: SET
  - args[0]: [RACEMODETOGGLE|MODESPINNER|VALUE]
  - args[1]: [OFF alias [FALSE, THROTTLE, 0]|ON alias [TRUE, RPS, 1]|2 alias SLIP|3...2000]
  - example: `SET MODESPINNER THROTTLE`
  - unblocks this interface element
- block interface element
  - command: BLOCK
  - args[0]: [VALUE|RACEMODETOGGLE]
  - args[1]: 0...2000|ON|off
  - example: `BLOCK VALUE 0`
- unblock interface element
  - command: UNBLOCK
  - args[0]: [VALUE]
  - example: `UNBLOCK VALUE`
- create a Toast message
  - command: MESSAGE
  - ...args: message
  - example: `MESSAGE bereits disarmed`
- set maximum slider value
  - command: MAXVALUE
  - ...args: new maximum value
  - example: `MAXVALUE 2000`

## Telemetry response

- prefix: "TELEMETRY "
- a: armed
- m: mode
- t: throttle
- r: rps
- s: slip
- v: velocity (BMI)
- w: velocity (wheels)
- c: acceleration (BMI)
- u: voltage (cV)
- p: temperature (°C)
- o: override slider and text input - or -
- q: override just text input
- example: TELEMETRY a0!m0!t0!r0!s0!v0!w0!c0!u740!p30

# CPU core assignments

- core 1
  - loop
    - telemetry acquisition and processing
    - throttle calculation
  - interrupts
    - ESCir
      - sending ESC data packet
      - receiving BMI sensor data
- core 0
  - loop
    - WiFi
      - receiving commands and sending telemetry
    - Serial
    - voltage protection
  - interrupts: none

# Logging in RAM

- every esc cycle (1000Hz) for 5000 frames:
- uint16_t[] throttle (0...2000) - 10KB
- int16_t[] acceleration (BMI raw value) - 10KB
- uint16_t[] raw wheel heRPM - 10KB
- uint16_t[] voltage (from ESC) in cV - 10KB
- uint8_t[] temperature (from ESC) in °C - 5KB
- total: 45KB

# Sending the logs

45KB of binary data:

1. 0-9999: throttle
2. 10000-19999: acceleration
3. 20000-29999: erpm
4. 30000-39999: voltage
5. 40000-44999: temperature

# Other notes

## Slip calculation

slip = (vRad - vAuto) / vRad

<=> slip \* vRad = vRad - vAuto

<=> vRad \* slip - vRad = - vAuto

<=> vRad = (- vAuto) / (slip - 1)

## Hold RPS

1. d_rps = rps_soll - rps_pred
2. d*thr = d_rps³ * .00000015+d*rps² * .000006+d_rps \* .006 &nbsp;&nbsp;&nbsp;&nbsp;(each ms, for PID_DIV = 0)

## ESC telemetry protocol

- 0-7: temp in °C
- 8-23: voltage in cV
- 24-39: current in cA
- 40-55: charge in mAh
- 56-71: angular velocity in heRPM --> readout of 98 means 9800eRPM: 1400RPM (≈ readout \* 14)
- 72-79: CRC
