/**
 * @file eeprom_i2c.h
 * @brief Lightweight I2C EEPROM driver for STM32 HAL.
 *
 * Supports ST M24Cxx and Microchip/Atmel AT24Cxx EEPROM families.
 * Each stored record is wrapped in a 4-byte header containing a CRC-16 checksum
 * and the data length, enabling integrity verification and wear leveling.
 *
 * Features:
 * - Multi-chip support (20 variants, conditional compilation to save flash)
 * - CRC-16-Modbus data integrity checking on every write and read
 * - Wear leveling: write is skipped when the stored data is identical
 * - Write-back verification: every write is read back and compared
 * - Write-protect GPIO support with configurable active polarity
 * - Bank addressing for chips with 8-bit word address and capacity > 256 B
 * - Multiple independent instances on a single physical chip via startAddr
 *
 * Created on: Feb 17, 2026
 *     Author: Adrian
 */

#ifndef EEPROM_I2C_H_
#define EEPROM_I2C_H_

#include "stdint.h"
#include "stm32c0xx_hal.h"

// ============================================================================
// Conditional chip support
// ============================================================================

/**
 * @defgroup EEPROM_I2C_ChipSupport Conditional chip support
 * @{
 * Define only the chips you need before including this header to save flash.
 * Example:
 * @code
 * #define EEPROM_I2C_SUPPORT_M24C02
 * #include "eeprom_i2c.h"
 * @endcode
 * If none of the EEPROM_I2C_SUPPORT_xxx macros are defined, all chips are
 * enabled by default.
 * @}
 */

#if !defined(EEPROM_I2C_SUPPORT_M24C01)  && !defined(EEPROM_I2C_SUPPORT_M24C02)  && \
    !defined(EEPROM_I2C_SUPPORT_M24C04)  && !defined(EEPROM_I2C_SUPPORT_M24C08)  && \
    !defined(EEPROM_I2C_SUPPORT_M24C16)  && !defined(EEPROM_I2C_SUPPORT_M24C32)  && \
    !defined(EEPROM_I2C_SUPPORT_M24C64)  && !defined(EEPROM_I2C_SUPPORT_M24C128) && \
    !defined(EEPROM_I2C_SUPPORT_M24C256) && !defined(EEPROM_I2C_SUPPORT_M24C512) && \
    !defined(EEPROM_I2C_SUPPORT_AT24C01) && !defined(EEPROM_I2C_SUPPORT_AT24C02) && \
    !defined(EEPROM_I2C_SUPPORT_AT24C04) && !defined(EEPROM_I2C_SUPPORT_AT24C08) && \
    !defined(EEPROM_I2C_SUPPORT_AT24C16) && !defined(EEPROM_I2C_SUPPORT_AT24C32) && \
    !defined(EEPROM_I2C_SUPPORT_AT24C64) && !defined(EEPROM_I2C_SUPPORT_AT24C128)&& \
    !defined(EEPROM_I2C_SUPPORT_AT24C256)&& !defined(EEPROM_I2C_SUPPORT_AT24C512)
    // No chips defined - enable all
    #define EEPROM_I2C_SUPPORT_M24C01
    #define EEPROM_I2C_SUPPORT_M24C02
    #define EEPROM_I2C_SUPPORT_M24C04
    #define EEPROM_I2C_SUPPORT_M24C08
    #define EEPROM_I2C_SUPPORT_M24C16
    #define EEPROM_I2C_SUPPORT_M24C32
    #define EEPROM_I2C_SUPPORT_M24C64
    #define EEPROM_I2C_SUPPORT_M24C128
    #define EEPROM_I2C_SUPPORT_M24C256
    #define EEPROM_I2C_SUPPORT_M24C512
    #define EEPROM_I2C_SUPPORT_AT24C01
    #define EEPROM_I2C_SUPPORT_AT24C02
    #define EEPROM_I2C_SUPPORT_AT24C04
    #define EEPROM_I2C_SUPPORT_AT24C08
    #define EEPROM_I2C_SUPPORT_AT24C16
    #define EEPROM_I2C_SUPPORT_AT24C32
    #define EEPROM_I2C_SUPPORT_AT24C64
    #define EEPROM_I2C_SUPPORT_AT24C128
    #define EEPROM_I2C_SUPPORT_AT24C256
    #define EEPROM_I2C_SUPPORT_AT24C512
#endif

// ============================================================================
// Supported chip types
// ============================================================================

/**
 * @brief Supported EEPROM chip variants.
 *
 * Pass one of these values to EEPROM_I2C_Init() as the @p chipType argument.
 * Only the variants enabled by the corresponding EEPROM_I2C_SUPPORT_xxx macro
 * are compiled in; the others are absent from the enum.
 */
typedef enum
{
    // --- ST M24Cxx series ---
#ifdef EEPROM_I2C_SUPPORT_M24C01
    EEPROM_I2C_CHIP_M24C01,   /**< ST M24C01 - 128 B, 8 B page, 8-bit addr */
#endif
#ifdef EEPROM_I2C_SUPPORT_M24C02
    EEPROM_I2C_CHIP_M24C02,   /**< ST M24C02 - 256 B, 8 B page, 8-bit addr */
#endif
#ifdef EEPROM_I2C_SUPPORT_M24C04
    EEPROM_I2C_CHIP_M24C04,   /**< ST M24C04 - 512 B, 16 B page, 8-bit addr (banked) */
#endif
#ifdef EEPROM_I2C_SUPPORT_M24C08
    EEPROM_I2C_CHIP_M24C08,   /**< ST M24C08 - 1 KB, 16 B page, 8-bit addr (banked) */
#endif
#ifdef EEPROM_I2C_SUPPORT_M24C16
    EEPROM_I2C_CHIP_M24C16,   /**< ST M24C16 - 2 KB, 16 B page, 8-bit addr (banked) */
#endif
#ifdef EEPROM_I2C_SUPPORT_M24C32
    EEPROM_I2C_CHIP_M24C32,   /**< ST M24C32 - 4 KB, 32 B page, 16-bit addr */
#endif
#ifdef EEPROM_I2C_SUPPORT_M24C64
    EEPROM_I2C_CHIP_M24C64,   /**< ST M24C64 - 8 KB, 32 B page, 16-bit addr */
#endif
#ifdef EEPROM_I2C_SUPPORT_M24C128
    EEPROM_I2C_CHIP_M24C128,  /**< ST M24C128 - 16 KB, 64 B page, 16-bit addr */
#endif
#ifdef EEPROM_I2C_SUPPORT_M24C256
    EEPROM_I2C_CHIP_M24C256,  /**< ST M24C256 - 32 KB, 64 B page, 16-bit addr */
#endif
#ifdef EEPROM_I2C_SUPPORT_M24C512
    EEPROM_I2C_CHIP_M24C512,  /**< ST M24C512 - 64 KB, 128 B page, 16-bit addr */
#endif

    // --- Microchip/Atmel AT24Cxx series ---
#ifdef EEPROM_I2C_SUPPORT_AT24C01
    EEPROM_I2C_CHIP_AT24C01,  /**< Microchip AT24C01 - 128 B, 8 B page, 8-bit addr */
#endif
#ifdef EEPROM_I2C_SUPPORT_AT24C02
    EEPROM_I2C_CHIP_AT24C02,  /**< Microchip AT24C02 - 256 B, 8 B page, 8-bit addr */
#endif
#ifdef EEPROM_I2C_SUPPORT_AT24C04
    EEPROM_I2C_CHIP_AT24C04,  /**< Microchip AT24C04 - 512 B, 16 B page, 8-bit addr (banked) */
#endif
#ifdef EEPROM_I2C_SUPPORT_AT24C08
    EEPROM_I2C_CHIP_AT24C08,  /**< Microchip AT24C08 - 1 KB, 16 B page, 8-bit addr (banked) */
#endif
#ifdef EEPROM_I2C_SUPPORT_AT24C16
    EEPROM_I2C_CHIP_AT24C16,  /**< Microchip AT24C16 - 2 KB, 16 B page, 8-bit addr (banked) */
#endif
#ifdef EEPROM_I2C_SUPPORT_AT24C32
    EEPROM_I2C_CHIP_AT24C32,  /**< Microchip AT24C32 - 4 KB, 32 B page, 16-bit addr */
#endif
#ifdef EEPROM_I2C_SUPPORT_AT24C64
    EEPROM_I2C_CHIP_AT24C64,  /**< Microchip AT24C64 - 8 KB, 32 B page, 16-bit addr */
#endif
#ifdef EEPROM_I2C_SUPPORT_AT24C128
    EEPROM_I2C_CHIP_AT24C128, /**< Microchip AT24C128 - 16 KB, 64 B page, 16-bit addr */
#endif
#ifdef EEPROM_I2C_SUPPORT_AT24C256
    EEPROM_I2C_CHIP_AT24C256, /**< Microchip AT24C256 - 32 KB, 64 B page, 16-bit addr */
#endif
#ifdef EEPROM_I2C_SUPPORT_AT24C512
    EEPROM_I2C_CHIP_AT24C512, /**< Microchip AT24C512 - 64 KB, 128 B page, 16-bit addr */
#endif
} EepromI2cChipType_t;

// ============================================================================
// Configuration
// ============================================================================

/**
 * @brief Maximum size (in bytes) of the internal verification/comparison buffer.
 *
 * This buffer is allocated on the stack during write (verification pass) and
 * during validate. Set this to the largest @p dataLength you will ever pass
 * to EEPROM_I2C_Write() or EEPROM_I2C_Validate(). Requests exceeding this
 * limit return EEPROM_I2C_ERR_SIZE.
 *
 * Override before including this header:
 * @code
 * #define EEPROM_I2C_MAX_BUFFER_SIZE 128
 * #include "eeprom_i2c.h"
 * @endcode
 */
#ifndef EEPROM_I2C_MAX_BUFFER_SIZE
#define EEPROM_I2C_MAX_BUFFER_SIZE   512
#endif

// ============================================================================
// Types
// ============================================================================

/**
 * @brief Return status codes for all public API functions.
 */
typedef enum
{
    EEPROM_I2C_OK           = 0, /**< Operation completed successfully. */
    EEPROM_I2C_ERR,              /**< Generic error (NULL pointer, write-back mismatch). */
    EEPROM_I2C_ERR_CRC,          /**< CRC-16 mismatch - stored data is corrupted. */
    EEPROM_I2C_ERR_SIZE,         /**< Requested data length exceeds available space
                                      or EEPROM_I2C_MAX_BUFFER_SIZE. */
    EEPROM_I2C_ERR_IDENTICAL,    /**< Write skipped - stored data is identical to new data
                                      (wear leveling). Not an error; treat as EEPROM_I2C_OK
                                      if unchanged data is acceptable. */
    EEPROM_I2C_ERR_I2C,          /**< Underlying HAL I2C communication failure. */
} EepromI2cStatus_t;

/**
 * @brief Internal record header written before user data in the EEPROM.
 *
 * The header occupies the first 4 bytes of the region starting at
 * EepromI2cHandle_t::startAddr. User data follows immediately after.
 * The struct is packed to ensure no padding bytes affect the layout.
 */
typedef struct __attribute__((packed))
{
    uint16_t Crc16;      /**< CRC-16-Modbus checksum of the user data. */
    uint16_t DataLength; /**< Length of the user data in bytes. */
} EepromI2cHeader_t;

/** @brief Forward declaration of the private chip configuration structure (defined in .c). */
typedef struct EepromI2cChipConfig_t EepromI2cChipConfig_t;

/**
 * @brief EEPROM instance handle.
 *
 * Initialize with EEPROM_I2C_Init() before calling any other function.
 * Multiple handles can be created for the same physical chip by using
 * different @p startAddr values, allowing independent storage regions.
 */
typedef struct
{
    I2C_HandleTypeDef        *hi2c;       /**< Pointer to the STM32 HAL I2C peripheral handle. */
    GPIO_TypeDef             *WpPort;     /**< GPIO port of the Write Protect pin.
                                               Set to NULL if WP is not connected. */
    uint16_t                  WpPin;      /**< GPIO pin number of the Write Protect pin
                                               (e.g. GPIO_PIN_5). Ignored when WpPort is NULL. */
    uint8_t                   wpInverted; /**< Write protect polarity.
                                               0 = WP active LOW (standard - pin HIGH disables write).
                                               1 = WP active HIGH (e.g. NPN transistor - pin LOW disables write). */
    uint8_t                   i2cAddr;    /**< I2C device address including the R/W bit shifted out
                                               (e.g. 0xA0 when A2=A1=A0=GND). */
    const EepromI2cChipConfig_t *chipConfig; /**< Pointer to the read-only chip configuration
                                                  entry in flash. Set by EEPROM_I2C_Init(). */
    uint16_t                  startAddr;  /**< Starting byte address in the EEPROM where this
                                               instance stores its header and data. */
    uint16_t                  maxDataSize;/**< Maximum user data size in bytes for this instance.
                                               Computed by EEPROM_I2C_Init() as:
                                               totalSize - startAddr - sizeof(EepromI2cHeader_t). */
} EepromI2cHandle_t;

// ============================================================================
// Public API
// ============================================================================

/**
 * @brief Initialize an EEPROM handle.
 *
 * Populates all fields of the handle, selects the chip configuration from the
 * internal table, computes the maximum available data size, and asserts the
 * Write Protect pin (if connected) to prevent accidental writes.
 *
 * @param handle      Pointer to the handle structure to initialize.
 * @param hi2c        Pointer to the STM32 HAL I2C peripheral handle.
 * @param wpPort      GPIO port for the Write Protect pin. Pass NULL if WP is
 *                    not connected or not needed.
 * @param wpPin       GPIO pin number for the Write Protect signal (e.g. GPIO_PIN_5).
 *                    Ignored when wpPort is NULL.
 * @param wpInverted  Write protect polarity: 0 = WP active LOW (standard),
 *                    1 = WP active HIGH (inverted, e.g. driven through NPN transistor).
 * @param i2cAddr     I2C device address with the R/W bit position (e.g. 0xA0 for
 *                    a chip with A2=A1=A0 tied to GND).
 * @param chipType    Chip variant selector from EepromI2cChipType_t.
 * @param startAddr   Byte offset within the EEPROM where this instance begins.
 *                    Use different offsets for multiple independent instances on
 *                    one physical chip.
 *
 * @note This function does not perform any I2C communication.
 * @note The handle must remain valid for the lifetime of all subsequent API calls.
 */
void EEPROM_I2C_Init(EepromI2cHandle_t *handle,
                     I2C_HandleTypeDef *hi2c,
                     GPIO_TypeDef *wpPort,
                     uint16_t wpPin,
                     uint8_t wpInverted,
                     uint8_t i2cAddr,
                     EepromI2cChipType_t chipType,
                     uint16_t startAddr);

/**
 * @brief Write data to the EEPROM with CRC protection, wear leveling, and write-back verification.
 *
 * The function first computes a CRC-16-Modbus checksum of the new data and reads
 * the existing header from the EEPROM. If the stored checksum and length match,
 * the actual stored bytes are compared with the new data; if they are identical
 * the write is skipped and EEPROM_I2C_ERR_IDENTICAL is returned, avoiding
 * unnecessary wear. Otherwise, the header is written first, followed by the data.
 * After writing, the header and data are read back and compared to verify the
 * write succeeded.
 *
 * @param handle      Pointer to an initialized EEPROM handle.
 * @param pData       Pointer to the source data buffer. The data is written directly
 *                    without an intermediate copy (zero-copy).
 * @param dataLength  Number of bytes to write. Must not exceed handle->maxDataSize
 *                    or EEPROM_I2C_MAX_BUFFER_SIZE.
 *
 * @return EEPROM_I2C_OK           - Data written and verified successfully.
 *         EEPROM_I2C_ERR_IDENTICAL - Write skipped; stored data is unchanged (not a failure).
 *         EEPROM_I2C_ERR_SIZE      - dataLength exceeds the available region or buffer limit.
 *         EEPROM_I2C_ERR_I2C       - I2C communication error during write or readback.
 *         EEPROM_I2C_ERR           - handle or pData is NULL, or write-back comparison failed.
 */
EepromI2cStatus_t EEPROM_I2C_Write(EepromI2cHandle_t *handle, const void *pData, uint16_t dataLength);

/**
 * @brief Read data from the EEPROM and verify the CRC checksum.
 *
 * Reads the stored header to obtain the expected data length and CRC, then reads
 * the data directly into the caller's buffer. After reading, the CRC-16-Modbus
 * checksum is computed over the received bytes and compared against the stored
 * value. If they do not match, EEPROM_I2C_ERR_CRC is returned and the buffer
 * contents should not be used.
 *
 * @param handle      Pointer to an initialized EEPROM handle.
 * @param pData       Pointer to the destination buffer. Data is read directly into
 *                    this buffer without an intermediate copy (zero-copy).
 * @param dataLength  Number of bytes to read. Must match the length that was used
 *                    during the last successful write.
 *
 * @return EEPROM_I2C_OK       - Data read and CRC verified successfully.
 *         EEPROM_I2C_ERR_CRC  - CRC mismatch; stored data is corrupted.
 *         EEPROM_I2C_ERR_SIZE - dataLength does not match the stored length,
 *                               or exceeds handle->maxDataSize.
 *         EEPROM_I2C_ERR_I2C  - I2C communication error.
 *         EEPROM_I2C_ERR      - handle or pData is NULL.
 */
EepromI2cStatus_t EEPROM_I2C_Read(EepromI2cHandle_t *handle, void *pData, uint16_t dataLength);

/**
 * @brief Validate stored data integrity without copying it to a user buffer.
 *
 * Reads the header and the stored data into an internal temporary buffer, then
 * verifies the CRC-16-Modbus checksum. Useful for checking whether valid data
 * exists before allocating a receive buffer, or for a power-on integrity check.
 * Optionally returns the stored data length so the caller can allocate an
 * appropriately sized buffer before calling EEPROM_I2C_Read().
 *
 * @param handle          Pointer to an initialized EEPROM handle.
 * @param pExpectedLength Optional pointer to a variable that will receive the
 *                        stored data length in bytes. May be NULL if not needed.
 *
 * @return EEPROM_I2C_OK       - Stored data is present and CRC is valid.
 *         EEPROM_I2C_ERR_CRC  - CRC mismatch; stored data is corrupted.
 *         EEPROM_I2C_ERR_SIZE - Stored length exceeds handle->maxDataSize or
 *                               EEPROM_I2C_MAX_BUFFER_SIZE.
 *         EEPROM_I2C_ERR_I2C  - I2C communication error.
 *         EEPROM_I2C_ERR      - handle is NULL.
 */
EepromI2cStatus_t EEPROM_I2C_Validate(EepromI2cHandle_t *handle, uint16_t *pExpectedLength);

/**
 * @brief Erase stored data by zeroing the header region.
 *
 * Writes a zeroed EepromI2cHeader_t at the instance's start address. After this
 * call, EEPROM_I2C_Read() and EEPROM_I2C_Validate() will return EEPROM_I2C_ERR_SIZE
 * (length 0 does not match any valid request) or EEPROM_I2C_ERR_CRC, effectively
 * invalidating the stored record without erasing the data bytes themselves.
 *
 * @param handle  Pointer to an initialized EEPROM handle.
 *
 * @return EEPROM_I2C_OK      - Header zeroed successfully.
 *         EEPROM_I2C_ERR_I2C - I2C communication error during the header write.
 *         EEPROM_I2C_ERR     - handle is NULL.
 */
EepromI2cStatus_t EEPROM_I2C_Erase(EepromI2cHandle_t *handle);


#endif /* EEPROM_I2C_H_ */
