/*
 * eeprom_i2c.h
 *
 *  Created on: Feb 17, 2026
 *      Author: Adrian
 */

#ifndef EEPROM_I2C_H_
#define EEPROM_I2C_H_

#include "stdint.h"
#include "stm32c0xx_hal.h"

// ============================================================================
// Conditional chip support
// Define only chips you need before including this header to save flash.
// Example: #define EEPROM_I2C_SUPPORT_M24C02
// If nothing is defined, all chips are enabled by default.
// ============================================================================

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

typedef enum
{
    // --- ST M24Cxx series ---
#ifdef EEPROM_I2C_SUPPORT_M24C01
    EEPROM_I2C_CHIP_M24C01,
#endif
#ifdef EEPROM_I2C_SUPPORT_M24C02
    EEPROM_I2C_CHIP_M24C02,
#endif
#ifdef EEPROM_I2C_SUPPORT_M24C04
    EEPROM_I2C_CHIP_M24C04,
#endif
#ifdef EEPROM_I2C_SUPPORT_M24C08
    EEPROM_I2C_CHIP_M24C08,
#endif
#ifdef EEPROM_I2C_SUPPORT_M24C16
    EEPROM_I2C_CHIP_M24C16,
#endif
#ifdef EEPROM_I2C_SUPPORT_M24C32
    EEPROM_I2C_CHIP_M24C32,
#endif
#ifdef EEPROM_I2C_SUPPORT_M24C64
    EEPROM_I2C_CHIP_M24C64,
#endif
#ifdef EEPROM_I2C_SUPPORT_M24C128
    EEPROM_I2C_CHIP_M24C128,
#endif
#ifdef EEPROM_I2C_SUPPORT_M24C256
    EEPROM_I2C_CHIP_M24C256,
#endif
#ifdef EEPROM_I2C_SUPPORT_M24C512
    EEPROM_I2C_CHIP_M24C512,
#endif

    // --- Microchip/Atmel AT24Cxx series ---
#ifdef EEPROM_I2C_SUPPORT_AT24C01
    EEPROM_I2C_CHIP_AT24C01,
#endif
#ifdef EEPROM_I2C_SUPPORT_AT24C02
    EEPROM_I2C_CHIP_AT24C02,
#endif
#ifdef EEPROM_I2C_SUPPORT_AT24C04
    EEPROM_I2C_CHIP_AT24C04,
#endif
#ifdef EEPROM_I2C_SUPPORT_AT24C08
    EEPROM_I2C_CHIP_AT24C08,
#endif
#ifdef EEPROM_I2C_SUPPORT_AT24C16
    EEPROM_I2C_CHIP_AT24C16,
#endif
#ifdef EEPROM_I2C_SUPPORT_AT24C32
    EEPROM_I2C_CHIP_AT24C32,
#endif
#ifdef EEPROM_I2C_SUPPORT_AT24C64
    EEPROM_I2C_CHIP_AT24C64,
#endif
#ifdef EEPROM_I2C_SUPPORT_AT24C128
    EEPROM_I2C_CHIP_AT24C128,
#endif
#ifdef EEPROM_I2C_SUPPORT_AT24C256
    EEPROM_I2C_CHIP_AT24C256,
#endif
#ifdef EEPROM_I2C_SUPPORT_AT24C512
    EEPROM_I2C_CHIP_AT24C512,
#endif
} EepromI2cChipType_t;

// ============================================================================
// Configuration
// ============================================================================

// Maximum internal buffer size for verification/comparison.
// Set this to the largest dataLength you'll use in your application.
#ifndef EEPROM_I2C_MAX_BUFFER_SIZE
#define EEPROM_I2C_MAX_BUFFER_SIZE   512
#endif

// ============================================================================
// Types
// ============================================================================

typedef enum
{
    EEPROM_I2C_OK = 0,
    EEPROM_I2C_ERR,
    EEPROM_I2C_ERR_CRC,
    EEPROM_I2C_ERR_SIZE,
    EEPROM_I2C_ERR_IDENTICAL,   // Data unchanged - write skipped (wear leveling)
    EEPROM_I2C_ERR_I2C
} EepromI2cStatus_t;

typedef struct __attribute__((packed))
{
    uint16_t Crc16;
    uint16_t DataLength;
} EepromI2cHeader_t;

// Forward declaration of chip config (defined in .c)
typedef struct EepromI2cChipConfig_t EepromI2cChipConfig_t;

typedef struct
{
    I2C_HandleTypeDef        *hi2c;
    GPIO_TypeDef             *WpPort;
    uint16_t                  WpPin;
    uint8_t                   wpInverted;   // 0 = WP active LOW (standard), 1 = WP active HIGH (via transistor)
    uint8_t                   i2cAddr;      // I2C device address (e.g., 0xA0 for 0x50<<1)
    const EepromI2cChipConfig_t *chipConfig; // Pointer to chip configuration in flash
    uint16_t                  startAddr;    // Starting address where this instance stores data
    uint16_t                  maxDataSize;  // Maximum user data size for this instance
} EepromI2cHandle_t;

// ============================================================================
// Public API
// ============================================================================

/**
 * @brief Initialize EEPROM handle
 * @param handle     Handle to initialize
 * @param hi2c       Pointer to I2C peripheral handle
 * @param wpPort      GPIO port for Write Protect pin (NULL if WP not connected)
 * @param wpPin       GPIO pin number for Write Protect (ignored if wpPort is NULL)
 * @param wpInverted  0 = WP active LOW (standard), 1 = WP active HIGH (e.g. NPN transistor)
 * @param i2cAddr    I2C device address (e.g., 0xA0 for A2=A1=A0=0)
 * @param chipType   Chip type (e.g., EEPROM_I2C_CHIP_M24C02)
 * @param startAddr  Starting address in EEPROM where this instance stores data
 * @note  Multiple instances can share one physical chip using different startAddr values
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
 * @brief Write data to EEPROM with CRC, verification, and wear leveling
 * @param handle      Initialized EEPROM handle
 * @param pData       Pointer to source data (zero-copy)
 * @param dataLength  Number of bytes to write
 * @return EEPROM_I2C_OK on success, EEPROM_I2C_ERR_IDENTICAL if unchanged
 */
EepromI2cStatus_t EEPROM_I2C_Write(EepromI2cHandle_t *handle, const void *pData, uint16_t dataLength);

/**
 * @brief Read data from EEPROM and verify CRC
 * @param handle      Initialized EEPROM handle
 * @param pData       Pointer to destination buffer (zero-copy)
 * @param dataLength  Expected number of bytes
 * @return EEPROM_I2C_OK on success, EEPROM_I2C_ERR_CRC if data corrupted
 */
EepromI2cStatus_t EEPROM_I2C_Read(EepromI2cHandle_t *handle, void *pData, uint16_t dataLength);

/**
 * @brief Validate stored data without reading into user buffer
 * @param handle          Initialized EEPROM handle
 * @param pExpectedLength Optional pointer to store the stored data length (can be NULL)
 * @return EEPROM_I2C_OK if valid, EEPROM_I2C_ERR_CRC if corrupted
 */
EepromI2cStatus_t EEPROM_I2C_Validate(EepromI2cHandle_t *handle, uint16_t *pExpectedLength);

/**
 * @brief Erase stored data by zeroing the header
 * @param handle  Initialized EEPROM handle
 * @return EEPROM_I2C_OK on success
 */
EepromI2cStatus_t EEPROM_I2C_Erase(EepromI2cHandle_t *handle);


#endif /* EEPROM_I2C_H_ */
