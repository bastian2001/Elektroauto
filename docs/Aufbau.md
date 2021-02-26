# Pin assignments

```
| 0  | 2  | 25         | 16        | 22  | 23           | 33     |
|----|----|------------|-----------|-----|--------------|--------|
| TX | RX | ESC output | ESC telem | LED | transm. ind. | ESC IR |
```

# Request

- format: `[key]:[value]`
- device type
  - key: DEVICE
  - value: [WEB alias 0|APP alias 1]
  - example: `DEVICE:APP`
- telemetry on/off
  - key: TELEMETRY
  - value: [ON alias 1|OFF alias 0]
  - example: `TELEMETRY:ON`
- armed
  - key: ARMED
  - value: [YES alias [1, TRUE]|NO alias [0, FALSE]]
  - example: `ARMED:YES`
- change mode
  - key: MODE
  - value: [THROTTLE alias 0|RPS alias 1|SLIP alias 2]
  - example: `MODE:SLIP`
- set value
  - key: VALUE
  - value: [value] (0...2000, 0...2000, 0...20)
  - example: `VALUE:7`
- race mode
  - key: RACEMODE
  - value: [OFF alias 0|ON alias 1]
  - example: `RACEMODE:ON`
- start race
  - key: STARTRACE
  - value: none
- ping
  - key: PING
  - value: none

# Responses

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
  - command: MESSSAGE
  - ...args: message
  - example: `MESSAGE bereits disarmed`

## Telemetry response

- prefix: "TELEMETRY"
- a: armed
- m: mode
- t: throttle
- r: rps
- s: slip
- v: velocity (mpu)
- w: velocity (wheels)
- c: acceleration (mpu)
- u: voltage (cV)
- p: temperature (°C)
- o: override slider and text input - or - 
- q: override just text input
- example: `TELEMETRY a0!m0!t0!r0!s0!v0!w0!c0!u740!p30`

# CPU core assignments

- core 1
  - loop
    - MPU
  - interrupts
    - (nothing)
- core 0
  - loop
    - WiFi
    - Serial
    - process telemetry
  - interrupts
    - ESCir --> calc

# Logging in RAM

- every esc cycle (1000Hz) for 5000 frames:
- uint16_t[] throttle - 10KB (before +47)
- int16_t[] acceleration(MPU) - 10KB
- uint16_t[] wheel rps - 10KB
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

slip = (vRad - vAuto) / vRad = 1 - vAuto / vRad

 <=> slip * vRad = vRad - vAuto

 <=> vRad * slip - vRad = - vAuto

 <=> vRad = (- vAuto) / (slip - 1)

## Hold RPS

1. d_rps = rps_soll - rps_pred
2. d_thr = d_rps³ * .00000015+d_rps² * .000006+d_rps \* .006 &nbsp;&nbsp;&nbsp;&nbsp;(each ms, for PID_DIV = 0)

## ESC telemetry protocol

- 0-7: temp in °C
- 8-23: voltage in cV
- 24-39: current in cA
- 40-55: charge in mAh
- 56-71: angular velocity in heRPM --> readout of 98 means 9800eRPM: 1400RPM (≈ readout \* 14)
- 72-79: CRC
