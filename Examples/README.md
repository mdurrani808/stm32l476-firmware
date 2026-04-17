# Examples

This folder contains small, copy-paste-friendly snippets for bring-up.

## Suggested order
1. `led_gpio_example.c` – verify a pin can be driven.
2. `pwm_example.c` – verify a timer pin is producing servo-style PWM.
3. `analog_read_example.c` – verify a raw ADC read path.
4. `can_api_example.c` – transmit and request pages from the shipped DBC.
5. Enable the optional `ex_system_controller()` or
   `dbc_examples_system_controller()` lines in `Core/Src/main.c` for scheduler-
   driven demos.

## CAN notes
- The examples use names from `dbc_latest_4.13.2026.dbc` / `App/dbc/file.dbc`.
- The checked-in Debug makefile regenerates `App/dbc/can_dbc_text.c` from that
  DBC before compiling the generated C blob.
- The default hardware RX filter is set to the POWER, SCIENCE_SERVO, and
  SCIENCE_DC_MOTOR message families used by the examples.
