# STM32L476 Firmware Repository

## Overview

This repository contains firmware for the STM32L476RG using STM32CubeIDE and STM32 HAL. It is organized around a cooperative scheduler, a DBC-driven CAN parameter database, and modular controller systems.

## Features

* Round-robin cooperative scheduler
* Runtime CAN DBC parsing / generated DBC text blob
* Named parameter database for signals
* CAN RX decode / TX encode
* PWM / GPIO / ADC platform helpers
* Example systems for testing hardware and CAN flows
* Centralized project configuration

## Repository Layout

```text
Core/            CubeMX startup, HAL init, main.c
App/Inc/         Application headers
App/Src/         Application sources
App/systems/     Runtime controller systems
App/dbc/         DBC files and generated C blob
Platform/        Hardware helper drivers
Drivers/         STM32 HAL / CMSIS
Examples/        Example usage snippets
```

## Build

Use STM32CubeIDE.

1. Import project.
2. Select `Debug` configuration.
3. Clean project.
4. Build.
5. Flash to target.

## Runtime Architecture

```text
main()
 ├── HAL init
 ├── clock/peripheral init
 ├── register controllers
 └── while(1)
      RR_Scheduler_Tick()
```

Each registered controller runs repeatedly in sequence.

## Scheduler API

### `RR_Scheduler_Init()`

Initializes scheduler state.

### `RR_AddController(fn)`

Registers a controller callback.

Example:

```c
RR_AddController(can_system_controller);
RR_AddController(test_pwm_system_controller);
```

### `RR_Scheduler_Tick()`

Runs all controllers once.

## CAN System

## Files

* `can_system.h`
* `can_system.c`
* `can_params.h`
* `can_params.c`
* `can_config.h`
* `can_config.c`

## Concepts

* DBC messages become named parameters.
* RX frames decode into parameters.
* TX frames are scheduled from parameter values.
* Hardware filters optionally restrict accepted IDs.

## Common CAN Functions

### `CanSystem_Send(const char* name)`

Schedules transmit for a message or signal.

Examples:

```c
CanSystem_Send("POWER_PCB_R");
CanSystem_Send("SCIENCE_SERVO_PCB_R.heartbeat_success");
```

### `CanSystem_SendRaw(const char* frame_str)`

Sends a raw standard-ID CAN data frame immediately without using the DBC or parameter database.

Accepted format:

```text
XXX#
XXX#112233
7FF#0011223344556677
```

Rules:

- `XXX` is a 1 to 3 digit hex standard ID (`0x000..0x7FF`)
- payload is optional
- payload must contain an even number of hex digits
- maximum payload is 8 bytes (`16` hex digits)
- only standard 11-bit IDs are supported

Examples:

```c
CanSystem_SendRaw("70#300000");
CanSystem_SendRaw("70#300040");
CanSystem_SendRaw("70#30FF7F");
CanSystem_SendRaw("123#1122334455667788");
```

### `CanSystem_SetBool(name, value)` *(legacy)*

Sets parameter then schedules send.

### `CanSystem_SetInt32(name, value)` *(legacy)*

Sets parameter then schedules send.

### `CanSystem_SetFloat(name, value)` *(legacy)*

Sets parameter then schedules send.

### `CanSystem_DebugGetLastRxTick(name, &tick)`

Returns last RX tick for message/page.

### `CanSystem_DebugGetLastTxTick(name, &tick)`

Returns last TX tick for message/page.

### `CanSystem_DebugIsStdIdAllowed(id)`

Returns whether ID passes allowlist.

## Parameter Database Functions

### `CanParams_SetBool(name, value)`

### `CanParams_SetInt32(name, value)`

### `CanParams_SetFloat(name, value)`

Update stored parameter values.

### `CanParams_GetBool(name, &out)`

### `CanParams_GetInt32(name, &out)`

### `CanParams_GetFloat(name, &out)`

Read parameter values.

### `CanParams_GetEvent(name, &flag)`

Read event flag.

### `CanParams_ProcEvent(name)`

Consumes/clears event.

## Hardware Filter Configuration

Edit:

* `App/Inc/project_config.h`
* `App/Src/can_config.c`

Set:

```c
#define PROJECT_CAN_USE_DEFAULT_RX_FILTER 1
```

or `0` to accept all IDs.

## Platform Drivers

## PWM

Files:

* `pwm.h`
* `pwm.c`

### `Platform_PWM_Init(&cfg)`

Initializes timer PWM output.

### `Platform_PWM_WriteMicroseconds(&cfg, pulse_us)`

Writes pulse width.

## GPIO

Use STM32 HAL:

```c
HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);
HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_5);
```

## ADC

Use HAL ADC APIs or project helper if included.

## Included Systems

## `can_system_controller()`

Owns CAN RX/TX processing.

## `pcb_led_system_controller()`

Drives board LED from CAN parameter.

## `heartbeat_system_controller()`

Sends heartbeat responses periodically.

## `servo_system_controller()`

Servo PWM / IO logic.

## `test_pwm_system_controller()`

Reads:

```text
SCIENCE_DC_MOTOR_PCB_C.dc_motor_velocity_target_0
```

Maps `0..32767` to PWM duty cycle.

## Enabling Systems

In `main.c` uncomment desired controllers:

```c
RR_AddController(can_system_controller);
RR_AddController(test_pwm_system_controller);
```

## Example CAN Commands

Raw SocketCAN style examples:

```text
70#300000   0%
70#300040   50%
70#30FF7F   100%
```

## Configuration

Primary config file:

```text
App/Inc/project_config.h
```

Contains tunables such as:

* CAN capacities
  n- CAN IDs
* filter enable
* demo timing
* servo settings
* LED pin settings

## Recommended Bring-Up Order

1. Build project.
2. Flash board.
3. Verify LED system.
4. Verify CAN heartbeat.
5. Verify PWM output.
6. Verify actuator systems.

## Notes

* If adding new messages, update DBC and regenerate blob.
* If using many CAN IDs, software fallback still validates IDs beyond hardware banks.
* Clean + rebuild after config changes.
