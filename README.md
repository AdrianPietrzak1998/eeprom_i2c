# eeprom_i2c

Lightweight I2C EEPROM driver for STM32 HAL.
Supports ST M24Cxx and Microchip/Atmel AT24Cxx families with CRC-16 data integrity, wear leveling, and write-back verification.

---

## Features

- **Multi-chip support** — 20 EEPROM variants (ST M24C01…M24C512, Microchip AT24C01…AT24C512)
- **Conditional compilation** — enable only the chips you need to minimize flash usage
- **CRC-16-Modbus integrity** — every stored record is protected by a checksum verified on every read
- **Wear leveling** — write is skipped when the data in EEPROM is already identical to the new data
- **Write-back verification** — after every write the data is read back and compared byte-by-byte
- **Write Protect GPIO** — optional WP pin control with configurable active polarity (standard or inverted via transistor)
- **Bank addressing** — transparent handling of 8-bit addressed chips with capacity > 256 B (M24C04/08/16, AT24C04/08/16)
- **Multiple instances** — independent storage regions on a single physical chip via configurable `startAddr`
- **Zero-copy I/O** — writes read directly from the user pointer; reads go directly into the user buffer

---

## Requirements

- C99 or later
- STM32 HAL (I2C and GPIO)
- No external dependencies

---

## Installation

Copy `eeprom_i2c.h` and `eeprom_i2c.c` into your project and add them to your build system.
The header includes `stm32c0xx_hal.h` — adjust the include path for your MCU family if needed.

---

## Quick Start

```c
#include "eeprom_i2c.h"

EepromI2cHandle_t eeprom;

void App_Init(void)
{
    EEPROM_I2C_Init(
        &eeprom,
        &hi2c1,          // STM32 HAL I2C handle
        WP_GPIO_Port,    // GPIO port for Write Protect pin
        WP_Pin,          // GPIO pin number
        0,               // wpInverted: 0 = WP active LOW (standard)
        0xA0,            // I2C address (A2=A1=A0=GND)
        EEPROM_I2C_CHIP_M24C02,
        0                // startAddr: store data from address 0
    );
}

typedef struct {
    uint32_t counter;
    float    setpoint;
} AppConfig_t;

void App_SaveConfig(const AppConfig_t *cfg)
{
    EepromI2cStatus_t status = EEPROM_I2C_Write(&eeprom, cfg, sizeof(AppConfig_t));

    if (status == EEPROM_I2C_OK || status == EEPROM_I2C_ERR_IDENTICAL)
    {
        // Data saved (or unchanged — no write needed)
    }
}

void App_LoadConfig(AppConfig_t *cfg)
{
    if (EEPROM_I2C_Read(&eeprom, cfg, sizeof(AppConfig_t)) != EEPROM_I2C_OK)
    {
        // CRC error or I2C failure — use defaults
    }
}
```

---

## Usage

### 1. Limit chip support (optional, saves flash)

Define only the chips you use **before** including the header:

```c
#define EEPROM_I2C_SUPPORT_M24C02
#include "eeprom_i2c.h"
```

If no `EEPROM_I2C_SUPPORT_xxx` macro is defined, all 20 chips are compiled in.

### 2. Adjust the internal buffer size (optional)

The internal verification buffer defaults to 512 bytes. Override it to match your largest record:

```c
#define EEPROM_I2C_MAX_BUFFER_SIZE 64
#include "eeprom_i2c.h"
```

Requests whose `dataLength` exceeds this limit return `EEPROM_I2C_ERR_SIZE`.

### 3. Initialize the handle

```c
EepromI2cHandle_t eeprom;

EEPROM_I2C_Init(
    &eeprom,
    &hi2c1,
    WP_GPIO_Port,          // NULL if WP pin is not connected
    WP_Pin,
    0,                     // 0 = WP active LOW (standard), 1 = WP active HIGH
    0xA0,                  // I2C device address
    EEPROM_I2C_CHIP_AT24C32,
    0                      // startAddr
);
```

`EEPROM_I2C_Init()` performs no I2C communication. It asserts the WP pin immediately after initialization.

### 4. Write data

```c
EepromI2cStatus_t status = EEPROM_I2C_Write(&eeprom, &myData, sizeof(myData));
```

The function:
1. Computes a CRC-16 of the new data.
2. Reads the existing header and compares CRC + raw bytes — skips the write if identical (`EEPROM_I2C_ERR_IDENTICAL`).
3. Writes the header (CRC + length), then the data.
4. Reads back header and data and compares byte-by-byte.

### 5. Read data

```c
EepromI2cStatus_t status = EEPROM_I2C_Read(&eeprom, &myData, sizeof(myData));
if (status == EEPROM_I2C_ERR_CRC)
{
    // Data corrupted — do not use myData
}
```

### 6. Validate without reading

Useful at startup to check whether valid data exists before allocating a buffer:

```c
uint16_t storedLen = 0;
EepromI2cStatus_t status = EEPROM_I2C_Validate(&eeprom, &storedLen);

if (status == EEPROM_I2C_OK)
{
    // storedLen contains the number of bytes currently stored
}
```

### 7. Erase stored data

Zeros the 4-byte header, invalidating the stored record without erasing the data payload:

```c
EEPROM_I2C_Erase(&eeprom);
```

### 8. Multiple instances on one chip

Use different `startAddr` values to create independent storage regions:

```c
EepromI2cHandle_t eepromA, eepromB;

// Region A: starts at byte 0, holds up to (totalSize/2 - 4) bytes of user data
EEPROM_I2C_Init(&eepromA, &hi2c1, WP_GPIO_Port, WP_Pin, 0, 0xA0,
                EEPROM_I2C_CHIP_M24C64, 0);

// Region B: starts at byte 4096 (half of 8 KB chip)
EEPROM_I2C_Init(&eepromB, &hi2c1, WP_GPIO_Port, WP_Pin, 0, 0xA0,
                EEPROM_I2C_CHIP_M24C64, 4096);
```

---

## Write Protect pin wiring

| `wpInverted` | WP wiring | Pin HIGH | Pin LOW |
|:---:|---|---|---|
| `0` (default) | WP connected directly to MCU GPIO | Write **disabled** | Write **enabled** |
| `1` | WP driven through NPN transistor (inverted) | Write **enabled** | Write **disabled** |

---

## Memory layout

Each instance occupies a contiguous region starting at `startAddr`:

```
┌─────────────────────────┬───────────────────────────────────────┐
│  Header (4 bytes)       │  User data (dataLength bytes)         │
│  CRC16 | DataLength     │                                       │
└─────────────────────────┴───────────────────────────────────────┘
^startAddr
```

The maximum user data size is computed automatically:
```
maxDataSize = totalSize - startAddr - 4
```

---

## Status codes

| Code | Value | Meaning |
|---|:---:|---|
| `EEPROM_I2C_OK` | 0 | Operation completed successfully. |
| `EEPROM_I2C_ERR` | 1 | Generic error: NULL pointer or write-back mismatch. |
| `EEPROM_I2C_ERR_CRC` | 2 | CRC-16 mismatch — stored data is corrupted. |
| `EEPROM_I2C_ERR_SIZE` | 3 | Data length exceeds available space or `EEPROM_I2C_MAX_BUFFER_SIZE`. |
| `EEPROM_I2C_ERR_IDENTICAL` | 4 | Write skipped — stored data is unchanged (wear leveling). Not a failure. |
| `EEPROM_I2C_ERR_I2C` | 5 | HAL I2C communication error. |

---

## Supported chips

| Chip | Capacity | Page size | Address |
|---|---:|---:|---|
| M24C01 / AT24C01 | 128 B | 8 B | 8-bit |
| M24C02 / AT24C02 | 256 B | 8 B | 8-bit |
| M24C04 / AT24C04 | 512 B | 16 B | 8-bit (banked) |
| M24C08 / AT24C08 | 1 KB | 16 B | 8-bit (banked) |
| M24C16 / AT24C16 | 2 KB | 16 B | 8-bit (banked) |
| M24C32 / AT24C32 | 4 KB | 32 B | 16-bit |
| M24C64 / AT24C64 | 8 KB | 32 B | 16-bit |
| M24C128 / AT24C128 | 16 KB | 64 B | 16-bit |
| M24C256 / AT24C256 | 32 KB | 64 B | 16-bit |
| M24C512 / AT24C512 | 64 KB | 128 B | 16-bit |

*Banked* — chips that multiplex upper address bits into the I2C device address byte. Handled transparently by the driver.

---

## Best practices

- Always check the return value — even `EEPROM_I2C_ERR_IDENTICAL` means the write was skipped, not failed.
- Use `EEPROM_I2C_Validate()` at startup before the first `EEPROM_I2C_Read()` to detect uninitialized or corrupted EEPROM.
- Keep `EEPROM_I2C_MAX_BUFFER_SIZE` as small as your largest record to avoid unnecessary stack usage.
- I2C calls use `HAL_MAX_DELAY` — do not call the API from an ISR.

## Common pitfalls

- **Wrong `startAddr` on a banked chip** — if `startAddr` is not aligned to 256-byte banks, the first write may cross a bank boundary and corrupt data in adjacent regions. Use 256-byte-aligned offsets for banked chips.
- **`dataLength` mismatch between write and read** — `EEPROM_I2C_Read()` compares the requested `dataLength` against the stored length in the header and returns `EEPROM_I2C_ERR_SIZE` if they differ.
- **WP pin polarity** — verify `wpInverted` matches your hardware schematic. An incorrect polarity will keep the chip permanently write-protected or permanently unlocked.

---

## License

Mozilla Public License 2.0
