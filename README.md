# STM32L476 Firmware – CAN‑DBC Driven Modular Control System

## Overview

This firmware is a **modular, round‑robin–scheduled STM32L476 system** designed around a **CAN + DBC–driven architecture**.

Key goals of this design:

- **Write the CAN system once** and never touch it again
- Define all CAN behavior via a **DBC file**
- Automatically create **parameters for every CAN signal**
- Update parameters **asynchronously via CAN RX interrupts**
- Allow all other systems to **read parameters in a safe, decoupled way**
- Keep `main.c` **minimal and declarative**
- Enable/disable systems dynamically via **round‑robin controller registration**
- Clean separation between **Platform**, **App**, and **Systems**

This firmware is intended for **scalable embedded robotics / control systems**, where CAN messages act as a distributed command/state bus.

---

## High‑Level Architecture

```
Core/
 ├── main.c                  → Minimal entrypoint
 ├── error_handler.c         → Global Error_Handler()
 └── main.h

Platform/                    → Hardware & HAL glue only
 ├── Src/
 │   ├── system_clock.c
 │   ├── gpio.c
 │   ├── can.c
 │   ├── usart.c
 │   ├── stm32l4xx_hal_msp.c
 │   └── stm32l4xx_it.c
 └── Inc/

App/
 ├── rr/                     → Round‑robin scheduler
 ├── systems/                → Application systems
 │   ├── can_system.c
 │   └── pcb_led_system.c
 ├── dbc/                    → CAN DBC + generated C
 │   ├── file.dbc
 │   └── can_dbc_text.c
 ├── Src/
 │   └── can_params.c
 └── Inc/
     ├── can_params.h
     ├── can_system.h
     ├── pcb_led_system.h
     └── rr_scheduler.h

tools/
 └── dbc_to_c.py              → DBC → C generator
```

---

## Core Concepts

### 1. Round‑Robin Scheduler

The firmware uses a **cooperative round‑robin scheduler**.  
There is **no RTOS**.

- Systems register controller functions at runtime
- Each controller is called once per loop
- Controllers must be **non‑blocking**

```c
typedef void (*rr_controller_t)(void);
```

Controllers are added/removed dynamically:

```c
RR_AddController(can_system_controller);
RR_AddController(pcb_led_system_controller);
RR_RemoveController(pcb_led_system_controller);
```

Only functions ending in `_controller` are intended to be scheduled.

---

### 2. CAN System (Write Once)

The CAN system is **generic and DBC‑driven**:

- Parses the DBC at startup
- Creates parameters for **every CAN signal**
- Decodes CAN RX frames using DBC metadata
- Updates parameters **inside the CAN RX interrupt**
- Does **no application logic**
- CAN TX is handled on the CAN system's turn in the round-robin when application code requests parameter updates (see TX section below)

> The CAN system **never knows what the signals mean**.

---

### 3. CAN Parameters

Each CAN signal becomes a **named parameter**:

```
<MESSAGE_NAME>.<SIGNAL_NAME>
```

Example:
```
STEPPER_COMMAND.Set_LED
```

Parameters are:

- Created automatically from the DBC
- Updated asynchronously from CAN RX interrupts
- Thread‑safe via `volatile`
- Accessible by any system

Access API:

```c
bool CanParams_GetBool("STEPPER_COMMAND.Set_LED", &value);
bool CanParams_GetInt32(...);
bool CanParams_GetFloat(...);
```

All parameters are created in memory at CAN startup (from the DBC), and a parameter is considered **valid only after at least one CAN frame updates it**.

#### Global CAN State Flags

Two global boolean parameters are always created at CAN startup:

- `pending_inbox` – set to `true` if *any* decoded CAN RX frame updated at least one parameter since the last CAN system tick.  
  On the next CAN system tick, if no new updates occurred, it is cleared to `false`.
- `pending_outbox` – set to `true` when *any system* requests a parameter change that should be transmitted onto the bus.

These flags allow other systems to cheaply detect bus activity without scanning every parameter.

#### Transmitting Parameter Changes (DBC-driven TX)

Any system may request that a DBC-defined signal be transmitted by calling:

```c
#include "can_system.h"

CanSystem_SetBool("STEPPER_COMMAND.Set_LED", true);
CanSystem_SetInt32("SOME_MESSAGE.SomeCounter", 123);
CanSystem_SetFloat("SOME_MESSAGE.SomeValue", 12.34f);
```

Rules:

- Calling `CanSystem_Set*()` updates the stored parameter value and marks the owning DBC message as **dirty**.
- On the CAN system's next round-robin tick, all dirty messages are encoded and transmitted, and `pending_outbox` is cleared.
- Messages are **not** transmitted due to values learned from CAN RX (RX updates do not mark TX dirty), preventing echo/loopback.

#### Optional CAN RX Allowlist Filter

You can optionally restrict which CAN arbitration IDs are decoded into parameters.

Configure the allowlist in:

- `App/Inc/can_config.h`

Provide a list of 11-bit standard IDs (hex) that should be processed. If the list is empty, all standard IDs are accepted (default behavior).


---

### 4. CAN RX – Fully Interrupt‑Driven

CAN RX is handled **entirely via interrupts**:

1. CAN FIFO0 receives a frame
2. CAN peripheral triggers `CAN1_RX0_IRQn`
3. `CAN1_RX0_IRQHandler()` calls `HAL_CAN_IRQHandler(&hcan1)`
4. HAL invokes:
   ```c
   HAL_CAN_RxFifo0MsgPendingCallback()
   ```
5. Frame is decoded using DBC metadata
6. Corresponding parameters are updated

No polling is used.

---

### 5. DBC Workflow (Single Source of Truth)

The **DBC file defines everything**.

#### Source DBC
```
App/dbc/file.dbc
```

#### Generator
```
tools/dbc_to_c.py
```

#### Generated Output
```
App/dbc/can_dbc_text.c
```

To update CAN behavior:

```bash
python tools/dbc_to_c.py App/dbc/file.dbc App/dbc/can_dbc_text.c
```

Then rebuild and flash.

The CAN system code **does not change** when the DBC changes.

---

### 6. PCB LED System (Example Consumer)

`pcb_led_system` demonstrates how application logic consumes CAN parameters.

Behavior:
- Reads `STEPPER_COMMAND.Set_LED`
- If `true` → LED ON
- If `false` or invalid → LED OFF

This system:
- Knows nothing about CAN frames
- Knows nothing about bit positions
- Only depends on the parameter API

---

## main.c Philosophy

`main.c` is intentionally minimal:

- HAL init
- Clock config
- Peripheral init
- Round‑robin registration

```c
int main(void)
{
  HAL_Init();
  Platform_SystemClock_Config();

  MX_GPIO_Init();
  MX_CAN1_Init();
  MX_UART4_Init();

  RR_Scheduler_Init();
  RR_AddController(can_system_controller);
  RR_AddController(pcb_led_system_controller);

  while (1)
  {
    RR_Scheduler_Tick();
  }
}
```

No logic. No conditionals. No protocols.

---

## Error Handling

A single global error handler exists:

```
Core/Src/error_handler.c
```

All fatal errors funnel into:

```c
void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}
```

This guarantees **link safety** and consistent failure behavior across Platform and App layers.

---

## CAN Test Example

### Message
- Standard ID: `0x080` (128)
- DLC: `8`
- Byte 0: `0x90` (Command_Byte = 144)
- Byte 1 bit 0: `Set_LED`

### Payloads
```
90 01 00 00 00 00 00 00  → LED ON
90 00 00 00 00 00 00 00  → LED OFF
```

---

## Design Guarantees

- No system depends on another system’s internals
- CAN decode is **data‑driven**, not hardcoded
- Adding new CAN signals requires **no firmware logic changes**
- Systems can be added/removed at runtime
- No hidden coupling between layers

---

## Intended Future Extensions

- CAN TX signals driven from parameters
- Parameter ownership rules (RX‑only vs TX‑owned)
- Big‑endian (`@0`) signal support
- DBC validation tooling
- Optional RTOS wrapper (keeping same architecture)

---

## Summary

This firmware establishes a **clean, scalable embedded architecture**:

- CAN is a **data bus**, not logic
- DBC is the **contract**
- Systems are **loosely coupled**
- Interrupts handle real‑time work
- Round‑robin handles orchestration

This structure is meant to scale from a single LED to a full rover control stack without architectural rewrites.
