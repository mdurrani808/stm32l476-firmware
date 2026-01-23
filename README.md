# stm32l476-firmware — Motherboard Firmware (Round‑Robin)

## Fix for “cannot find entry symbol Reset_Handler”
If you see:

> `ld.exe: warning: cannot find entry symbol Reset_Handler; defaulting to 08000000`

it means the **startup file in `Startup/` is not being compiled/linked** (e.g., `startup_stm32l476rgtx.s` is missing from the build).

When reorganizing folders, make sure the **Startup** folder remains a compiled source folder in the CubeIDE/CDT project configuration.

This repo organization expects these compiled source folders:
- `Core/`
- `Drivers/`
- `Startup/`  ✅ required (Reset_Handler lives here)
- `Platform/`
- `App/`

and these include paths:
- `../Core/Inc`
- `../Drivers/...` (Cube defaults)
- `../Platform/Inc`
- `../App/Inc`
