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
    <td>-reserved, don't use-</td>
  </tr>
  <tr>
    <td>USB-RX (reserved)</td>
    <td>3</td>
    <td>-reserved, don't use-</td>
  </tr>
  <tr>
    <td>LED_BUILTIN</td>
    <td>22</td>
    <td>-can be used for debugging-</td>
  </tr>
  <tr>
    <td>ESC 1 Output</td>
    <td>16</td>
    <td>ESC 1: S (Signal)</td>
  </tr>
  <tr>
    <td>ESC 1 Telemetry</td>
    <td>21</td>
    <td>ESC 1: T (Telemetry)</td>
  </tr>
  <tr>
    <td>ESC 2 Output</td>
    <td>25</td>
    <td>ESC 2: S (Signal)</td>
  </tr>
  <tr>
    <td>ESC 2 Telemetry</td>
    <td>27</td>
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
    <td>26 or 3V3</td>
    <td>BMI160: 3V3</td>
  </tr>
  <tr>
    <td>Transmission indicator</td>
    <td>33</td>
    <td>-can be used for debugging (set flags)-</td>
  </tr>
</table>

# Device -> Car protocol

-   format: `[key]:[value]`
-   set value
    -   key: VALUE
    -   value: [value] (0...2000, 0...2000, 0...20)
    -   example: `VALUE:7`
-   armed
    -   key: ARMED
    -   value: [YES alias [1, TRUE]|NO alias [0, FALSE]]
    -   example: `ARMED:YES`
-   ping
    -   key: PING
    -   value: none
-   change mode
    -   key: MODE
    -   value: [THROTTLE alias 0|RPS alias 1|SLIP alias 2]
    -   example: `MODE:SLIP`
-   telemetry on/off
    -   key: TELEMETRY
    -   value: [ON alias 1|OFF alias 0]
    -   example: `TELEMETRY:ON`
-   device type
    -   key: DEVICE
    -   value: [WEB alias 0|APP alias 1]
    -   example: `DEVICE:APP`
-   race mode
    -   key: RACEMODE
    -   value: [OFF alias 0|ON alias 1]
    -   example: `RACEMODE:ON`
-   start race
    -   key: STARTRACE
    -   value: none
-   set cutoff coltage
    -   key: CUTOFFVOLTAGE
    -   value: [new cutoff voltage in cV]
    -   example: `CUTOFFVOLTAGE:600` for 6.0V
-   set warning coltage
    -   key: WARNINGVOLTAGE
    -   value: [new warning voltage in cV]
    -   example: `WARNINGVOLTAGE:720` for 7.2V
-   set RPS control variables
    -   keys: RPSA, RPSB, RPSC, PIDMULTIPLIER
    -   value: [value]
-   set additional multiplier for slip control
    -   key: SLIPMULTIPLIER
    -   value: [multiplier, default: 2]
    -   example: `SLIPMULTIPLIER:2`
-   get error count
    -   key: ERRORCOUNT
    -   value: none
    -   note: currently not used/implemented
-   reconnect to WiFi
    -   key: RECONNECT
    -   value: none
-   send raw data to the ESCs
    -   key: RAWDATA
    -   value: packets of 11 bits as 3 hex characters
    -   example: `RAWDATA:100 100 100` to send 3 packets with value 0010 0000 000(1 1010) to the ESCs
    -   note: telemetry will always be requested and checksum will be added automatically
-   save settings
    -   key: SAVESETTINGS alias SAVE
    -   value: none
-   read settings
    -   key: READSETTINGS alias READ
    -   value: none
-   restore defaults
    -   key: RESTOREDEFAULTS alias RESTORE
    -   value: none
    -   note: will restart the ESP32
-   send/print settings
    -   key: SENDSETTINGS alias SETTINGS
    -   value: none
-   set maximum (target) values
    -   keys: MAXTHROTTLE alias MAXT, MAXRPS alias MAXR, MAXSLIP alias MAXS
    -   value: [value]
-   set motorpole count
    -   key: MOTORPOLE
    -   value: [number of motor poles]
-   set wheel diameter
    -   key: WHEELDIAMETER alias WHEELDIA
    -   value: [wheel diameter in mm]
-   reboot the ESP32
    -   key: REBOOT alias RESTART
    -   value: none

# Car -> Device protocol

## Special response

-   format: `[command] [...args]`
-   set interface element
    -   command: SET
    -   args[0]: [RACEMODETOGGLE|MODESPINNER|VALUE]
    -   args[1]: [OFF alias [FALSE, THROTTLE, 0]|ON alias [TRUE, RPS, 1]|2 alias SLIP|3...2000]
    -   example: `SET MODESPINNER THROTTLE`
    -   unblocks this interface element
-   block interface element
    -   command: BLOCK
    -   args[0]: [VALUE|RACEMODETOGGLE]
    -   args[1]: 0...2000|ON|off
    -   example: `BLOCK VALUE 0`
-   unblock interface element
    -   command: UNBLOCK
    -   args[0]: [VALUE]
    -   example: `UNBLOCK VALUE`
-   create a Toast message
    -   command: MESSAGE
    -   ...args: message
    -   example: `MESSAGE bereits disarmed`
-   set maximum slider value
    -   command: MAXVALUE
    -   ...args: new maximum value
    -   example: `MAXVALUE 2000`

## Telemetry response

Telemetry is sent as 28 bytes of binary data:

-   Byte 1: armed: 1 (armed), 0 (disarmed)
-   Bytes 2-3: throttle of left ESC
-   Bytes 4-5: throttle of right ESC
-   Bytes 6-7: speed of left ESC in mm/s
-   Bytes 8-9: speed of right ESC in mm/s
-   Bytes 10-11: speed as measured by the accelerometer in mm/s
-   Bytes 12-13: RPS of left ESC
-   Bytes 14-15: RPS of right ESC
-   Byte 16: temperature of left ESC
-   Byte 17: temperature of right ESC
-   Byte 18: temperature of the accelerometer
-   Byte 19: temperature of the ESP32
-   Bytes 20-21: voltage measured by left ESC
-   Bytes 22-23: voltage measured by right ESC
-   Bytes 24-25: raw accelerometer value (-32768 = -2g ... 32767 = +2g)
-   Byte 26: 1 if race mode is enabled and race is not running, 0 otherwise. This is neccessary for the UI, so it will actually show the correct value in the slider AND the input box
-   Bytes 27-28: the last requested value (will usually be used to show in the input box)

# CPU core assignments

-   core 1
    -   loop
        -   ESC loop
            -   telemetry acquisition and processing
            -   handling errors and status change
        -   throttle calculation
    -   interrupts
        -   ESC
            -   sending ESC data packet
            -   receiving BMI sensor data
-   core 0
    -   loop
        -   WiFi
            -   receiving commands and sending telemetry and race log
        -   Serial
        -   voltage protection
        -   runs "Actions", e.g. timed disarming or sending queued messages
    -   interrupts: none

# Logging in RAM

-   every esc cycle (currently 800Hz) for 3000 frames:
-   uint16_t[] left throttle (0...2000) - 6KB
-   uint16_t[] right throttle (0...2000) - 6KB
-   uint16_t[] raw left wheel heRPM - 6KB
-   uint16_t[] raw right wheel heRPM - 6KB
-   uint16_t[] voltage (from left ESC) in cV - 6KB
-   uint16_t[] voltage (from right ESC) in cV - 6KB
-   uint8_t[] temperature (from left ESC) in °C - 3KB
-   uint8_t[] temperature (from right ESC) in °C - 3KB
-   int16_t[] acceleration (from BMI, raw value) - 6KB
-   int16_t[] temperature (from BMI, raw value) - 6KB
-   total: 54KB

# Sending the logs

54KB of binary data, sent like they are in the RAM. Beware that the ESP32 is little endian, meaning 16 bit values will be stored and sent as follows:

7 ... 0(LSB) | 15(MSB) ... 8

# Other notes

## Slip calculation

slip = (vWheel - vCar) / vWheel

<=> slip \* vWheel = vWheel - vCar

<=> vWheel \* slip - vWheel = - vCar

<=> vWheel = vCar / (1 - slip)

## Hold RPS

d_thr: throttle difference for next cycle

1. d_rps = rps_soll - rps_pred
2. d_thr = d_rps³ \* .00000015+d_rps² \* .000006+d_rps \* .006 &nbsp;&nbsp;&nbsp;&nbsp;(each ESC cycle)

## ESC telemetry protocol

-   0-7: temp in °C
-   8-23: voltage in cV
-   24-39: current in cA
-   40-55: charge in mAh
-   56-71: angular velocity in heRPM --> readout of 98 means 9800eRPM: 1400RPM (≈ readout \* 14)
-   72-79: CRC
