/**
 * @file eeprom_i2c.c
 * @brief Lightweight I2C EEPROM driver implementation for STM32 HAL.
 *
 * Created on: Feb 17, 2026
 *     Author: Adrian
 */

#include "eeprom_i2c.h"
#include <string.h>

// ============================================================================
// Chip configuration structure (private)
// ============================================================================

/**
 * @brief Internal, read-only descriptor for a supported EEPROM chip variant.
 *
 * One entry per EepromI2cChipType_t value is stored in the chipConfigs[] table
 * in flash. The public API accesses this structure only through the pointer
 * cached in EepromI2cHandle_t::chipConfig.
 */
struct EepromI2cChipConfig_t
{
    uint32_t totalSize;    /**< Total chip capacity in bytes. */
    uint8_t  pageSize;     /**< Page buffer size in bytes; writes must not cross page boundaries. */
    uint8_t  writeCycleMs; /**< Internal write cycle time in milliseconds (tWR from datasheet). */
    uint16_t addrSize;     /**< Word address width: I2C_MEMADD_SIZE_8BIT or I2C_MEMADD_SIZE_16BIT. */
};

/** @brief Read-only chip configuration table stored in flash.
 *         Only entries for enabled EEPROM_I2C_SUPPORT_xxx chips are compiled in. */
static const EepromI2cChipConfig_t chipConfigs[] =
{
    // -------------------------------------------------------------------------
    // ST M24Cxx series
    // -------------------------------------------------------------------------
#ifdef EEPROM_I2C_SUPPORT_M24C01
    [EEPROM_I2C_CHIP_M24C01]  = {128,    8,  5, I2C_MEMADD_SIZE_8BIT},
#endif
#ifdef EEPROM_I2C_SUPPORT_M24C02
    [EEPROM_I2C_CHIP_M24C02]  = {256,    8,  5, I2C_MEMADD_SIZE_8BIT},
#endif
#ifdef EEPROM_I2C_SUPPORT_M24C04
    [EEPROM_I2C_CHIP_M24C04]  = {512,   16,  5, I2C_MEMADD_SIZE_8BIT},
#endif
#ifdef EEPROM_I2C_SUPPORT_M24C08
    [EEPROM_I2C_CHIP_M24C08]  = {1024,  16,  5, I2C_MEMADD_SIZE_8BIT},
#endif
#ifdef EEPROM_I2C_SUPPORT_M24C16
    [EEPROM_I2C_CHIP_M24C16]  = {2048,  16,  5, I2C_MEMADD_SIZE_8BIT},
#endif
#ifdef EEPROM_I2C_SUPPORT_M24C32
    [EEPROM_I2C_CHIP_M24C32]  = {4096,  32,  5, I2C_MEMADD_SIZE_16BIT},
#endif
#ifdef EEPROM_I2C_SUPPORT_M24C64
    [EEPROM_I2C_CHIP_M24C64]  = {8192,  32,  5, I2C_MEMADD_SIZE_16BIT},
#endif
#ifdef EEPROM_I2C_SUPPORT_M24C128
    [EEPROM_I2C_CHIP_M24C128] = {16384, 64,  5, I2C_MEMADD_SIZE_16BIT},
#endif
#ifdef EEPROM_I2C_SUPPORT_M24C256
    [EEPROM_I2C_CHIP_M24C256] = {32768, 64,  5, I2C_MEMADD_SIZE_16BIT},
#endif
#ifdef EEPROM_I2C_SUPPORT_M24C512
    [EEPROM_I2C_CHIP_M24C512] = {65536, 128, 5, I2C_MEMADD_SIZE_16BIT},
#endif

    // -------------------------------------------------------------------------
    // Microchip/Atmel AT24Cxx series
    // -------------------------------------------------------------------------
#ifdef EEPROM_I2C_SUPPORT_AT24C01
    [EEPROM_I2C_CHIP_AT24C01]  = {128,    8,  5, I2C_MEMADD_SIZE_8BIT},
#endif
#ifdef EEPROM_I2C_SUPPORT_AT24C02
    [EEPROM_I2C_CHIP_AT24C02]  = {256,    8,  5, I2C_MEMADD_SIZE_8BIT},
#endif
#ifdef EEPROM_I2C_SUPPORT_AT24C04
    [EEPROM_I2C_CHIP_AT24C04]  = {512,   16,  5, I2C_MEMADD_SIZE_8BIT},
#endif
#ifdef EEPROM_I2C_SUPPORT_AT24C08
    [EEPROM_I2C_CHIP_AT24C08]  = {1024,  16,  5, I2C_MEMADD_SIZE_8BIT},
#endif
#ifdef EEPROM_I2C_SUPPORT_AT24C16
    [EEPROM_I2C_CHIP_AT24C16]  = {2048,  16,  5, I2C_MEMADD_SIZE_8BIT},
#endif
#ifdef EEPROM_I2C_SUPPORT_AT24C32
    [EEPROM_I2C_CHIP_AT24C32]  = {4096,  32,  5, I2C_MEMADD_SIZE_16BIT},
#endif
#ifdef EEPROM_I2C_SUPPORT_AT24C64
    [EEPROM_I2C_CHIP_AT24C64]  = {8192,  32,  5, I2C_MEMADD_SIZE_16BIT},
#endif
#ifdef EEPROM_I2C_SUPPORT_AT24C128
    [EEPROM_I2C_CHIP_AT24C128] = {16384, 64,  5, I2C_MEMADD_SIZE_16BIT},
#endif
#ifdef EEPROM_I2C_SUPPORT_AT24C256
    [EEPROM_I2C_CHIP_AT24C256] = {32768, 64,  5, I2C_MEMADD_SIZE_16BIT},
#endif
#ifdef EEPROM_I2C_SUPPORT_AT24C512
    [EEPROM_I2C_CHIP_AT24C512] = {65536, 128, 5, I2C_MEMADD_SIZE_16BIT},
#endif
};

// ============================================================================
// Private helper functions
// ============================================================================

/**
 * @brief Calculate a CRC-16-Modbus checksum (CRC-16-ANSI, polynomial 0x8005 reflected).
 *
 * The initial value is 0xFFFF. Each byte is XORed into the low byte of the CRC
 * register, then processed bit by bit using the reflected polynomial 0xA001.
 *
 * @param pData   Pointer to the data buffer to checksum.
 * @param length  Number of bytes to process.
 *
 * @return Computed 16-bit CRC value.
 */
static uint16_t CRC16_Modbus(const uint8_t *pData, uint16_t length)
{
    uint16_t crc = 0xFFFF;

    for (uint16_t i = 0; i < length; i++)
    {
        crc ^= (uint16_t)pData[i];

        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }

    return crc;
}

/**
 * @brief Return the number of upper address bits embedded in the I2C device address byte.
 *
 * Chips with 8-bit word addressing and a total capacity greater than 256 bytes
 * (e.g. M24C04/08/16, AT24C04/08/16) cannot address all memory with a single
 * 8-bit word address field. Instead, the upper address bits are multiplexed into
 * the A0/A1/A2 positions (bits 3:1) of the I2C device address byte, effectively
 * creating 256-byte "banks".
 *
 * @param config  Pointer to the chip configuration entry.
 *
 * @return Number of upper address bits that must be injected into the device
 *         address byte. Returns 0 for chips with 16-bit word addressing or
 *         8-bit word addressed chips that fit entirely within 256 bytes.
 */
static uint8_t GetDevAddrBits(const EepromI2cChipConfig_t *config)
{
    if (config->addrSize != I2C_MEMADD_SIZE_8BIT || config->totalSize <= 256)
        return 0;

    uint8_t bits = 0;
    uint16_t n = (config->totalSize - 1) >> 8;
    while (n) { n >>= 1; bits++; }
    return bits;
}

/**
 * @brief Compute the effective I2C device address for a given absolute memory address.
 *
 * For bank-addressed chips (GetDevAddrBits() > 0), the upper bits of the full
 * memory address are extracted and written into bits 3:1 (A0/A1/A2 positions)
 * of the base device address. For all other chips the base address is returned
 * unchanged.
 *
 * @param handle    Pointer to the EEPROM handle (provides base i2cAddr and chipConfig).
 * @param fullAddr  Absolute byte address within the EEPROM (0 … totalSize-1).
 *
 * @return Effective I2C device address to use in HAL_I2C_Mem_Read/Write for
 *         the given @p fullAddr.
 */
static uint8_t ComputeDevAddr(EepromI2cHandle_t *handle, uint16_t fullAddr)
{
    uint8_t devAddrBits = GetDevAddrBits(handle->chipConfig);
    if (devAddrBits == 0)
        return handle->i2cAddr;

    uint8_t upperBits   = (fullAddr >> 8) & ((1 << devAddrBits) - 1);
    uint8_t devAddrMask = ((1 << devAddrBits) - 1) << 1;
    return (handle->i2cAddr & ~devAddrMask) | (upperBits << 1);
}

/**
 * @brief Read a block of bytes from the EEPROM via I2C.
 *
 * For chips with 16-bit word addressing or chips whose capacity fits within
 * 256 bytes, the read is issued as a single HAL call. For bank-addressed chips
 * (8-bit word addressing, totalSize > 256 B) the transfer is split at each
 * 256-byte bank boundary and the device address is recomputed for each segment.
 *
 * @param handle   Pointer to the EEPROM handle.
 * @param memAddr  Absolute start address within the EEPROM to read from.
 * @param pData    Destination buffer; must be at least @p size bytes long.
 * @param size     Number of bytes to read.
 *
 * @return HAL_OK if all bytes were read successfully, or a HAL error code on failure.
 */
static HAL_StatusTypeDef I2C_ReadBytes(EepromI2cHandle_t *handle, uint16_t memAddr, uint8_t *pData, uint16_t size)
{
    if (GetDevAddrBits(handle->chipConfig) == 0)
    {
        return HAL_I2C_Mem_Read(handle->hi2c, handle->i2cAddr, memAddr,
                                handle->chipConfig->addrSize, pData, size, HAL_MAX_DELAY);
    }

    // Bank-addressed chip: split at 256-byte boundaries
    HAL_StatusTypeDef status = HAL_OK;
    uint16_t bytesRead  = 0;
    uint16_t currentAddr = memAddr;

    while (bytesRead < size && status == HAL_OK)
    {
        uint8_t  devAddr         = ComputeDevAddr(handle, currentAddr);
        uint16_t bankOffset      = currentAddr & 0xFF;
        uint16_t bytesLeftInBank = 256u - bankOffset;
        uint16_t bytesToRead     = (size - bytesRead < bytesLeftInBank) ?
                                   (size - bytesRead) : bytesLeftInBank;

        status = HAL_I2C_Mem_Read(handle->hi2c, devAddr, bankOffset,
                                  I2C_MEMADD_SIZE_8BIT, &pData[bytesRead], bytesToRead, HAL_MAX_DELAY);

        bytesRead   += bytesToRead;
        currentAddr += bytesToRead;
    }

    return status;
}

/**
 * @brief Return the GPIO state that asserts (locks) the Write Protect signal.
 *
 * When WP is active LOW (standard), driving the pin HIGH locks the EEPROM.
 * When WP is active HIGH (wpInverted = 1, e.g. NPN transistor), driving the
 * pin LOW locks the EEPROM.
 *
 * @param handle  Pointer to the EEPROM handle.
 * @return GPIO_PIN_SET or GPIO_PIN_RESET that must be written to WpPin to lock WP.
 */
static GPIO_PinState WP_LockState(EepromI2cHandle_t *handle)
{
    return handle->wpInverted ? GPIO_PIN_RESET : GPIO_PIN_SET;
}

/**
 * @brief Return the GPIO state that de-asserts (unlocks) the Write Protect signal.
 *
 * When WP is active LOW (standard), driving the pin LOW unlocks the EEPROM.
 * When WP is active HIGH (wpInverted = 1), driving the pin HIGH unlocks it.
 *
 * @param handle  Pointer to the EEPROM handle.
 * @return GPIO_PIN_SET or GPIO_PIN_RESET that must be written to WpPin to unlock WP.
 */
static GPIO_PinState WP_UnlockState(EepromI2cHandle_t *handle)
{
    return handle->wpInverted ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

/**
 * @brief Write a block of bytes to the EEPROM via I2C with page-write support.
 *
 * The transfer is split at page boundaries to comply with the chip's page buffer
 * limit. After each page write, the function waits for the internal write cycle
 * (tWR) using HAL_Delay() before issuing the next write. The Write Protect GPIO
 * is de-asserted at the start and re-asserted after the last page.
 *
 * For bank-addressed chips, the upper address bits are inserted into the device
 * address and only the lower 8 bits are sent as the word address field.
 *
 * @param handle   Pointer to the EEPROM handle.
 * @param memAddr  Absolute start address within the EEPROM to write to.
 * @param pData    Source buffer; must be at least @p size bytes long.
 * @param size     Number of bytes to write.
 *
 * @return HAL_OK if all bytes were written successfully, or a HAL error code
 *         on the first failing page write. Write Protect is re-asserted even
 *         on failure.
 */
static HAL_StatusTypeDef I2C_WriteBytes(EepromI2cHandle_t *handle, uint16_t memAddr, const uint8_t *pData, uint16_t size)
{
    HAL_StatusTypeDef status = HAL_OK;
    uint16_t bytesWritten = 0;
    uint16_t currentAddr  = memAddr;

    // Unlock write protect
    if (handle->WpPort != NULL)
        HAL_GPIO_WritePin(handle->WpPort, handle->WpPin, WP_UnlockState(handle));

    while (bytesWritten < size && status == HAL_OK)
    {
        // Partial first page if write starts mid-page
        uint16_t pageOffset      = currentAddr % handle->chipConfig->pageSize;
        uint16_t bytesLeftInPage = handle->chipConfig->pageSize - pageOffset;
        uint16_t bytesToWrite    = (size - bytesWritten < bytesLeftInPage) ?
                                   (size - bytesWritten) : bytesLeftInPage;

        // For bank-addressed chips: upper address bits go into device address,
        // only the lower 8 bits are sent as the word address.
        uint8_t  devAddr     = ComputeDevAddr(handle, currentAddr);
        uint16_t wordAddr    = (GetDevAddrBits(handle->chipConfig) > 0) ?
                               (currentAddr & 0xFF) : currentAddr;

        status = HAL_I2C_Mem_Write(handle->hi2c, devAddr, wordAddr,
                                   handle->chipConfig->addrSize, (uint8_t*)&pData[bytesWritten],
                                   bytesToWrite, HAL_MAX_DELAY);

        if (status != HAL_OK)
            break;

        // Wait for internal write cycle
        HAL_Delay(handle->chipConfig->writeCycleMs);

        bytesWritten += bytesToWrite;
        currentAddr  += bytesToWrite;
    }

    // Lock write protect
    if (handle->WpPort != NULL)
        HAL_GPIO_WritePin(handle->WpPort, handle->WpPin, WP_LockState(handle));

    return status;
}

// ============================================================================
// Public API
// ============================================================================

void EEPROM_I2C_Init(EepromI2cHandle_t *handle,
                     I2C_HandleTypeDef *hi2c,
                     GPIO_TypeDef *wpPort,
                     uint16_t wpPin,
                     uint8_t wpInverted,
                     uint8_t i2cAddr,
                     EepromI2cChipType_t chipType,
                     uint16_t startAddr)
{
    handle->hi2c        = hi2c;
    handle->WpPort      = wpPort;
    handle->WpPin       = wpPin;
    handle->wpInverted  = wpInverted;
    handle->i2cAddr     = i2cAddr;
    handle->startAddr   = startAddr;
    handle->chipConfig  = &chipConfigs[chipType];
    handle->maxDataSize = handle->chipConfig->totalSize - startAddr - sizeof(EepromI2cHeader_t);

    // Lock write protect by default
    if (handle->WpPort != NULL)
        HAL_GPIO_WritePin(handle->WpPort, handle->WpPin, WP_LockState(handle));
}

EepromI2cStatus_t EEPROM_I2C_Write(EepromI2cHandle_t *handle, const void *pData, uint16_t dataLength)
{
    if (handle == NULL || pData == NULL)
        return EEPROM_I2C_ERR;

    if (dataLength > handle->maxDataSize)
        return EEPROM_I2C_ERR_SIZE;

    const uint8_t *pByteData = (const uint8_t*)pData;
    uint16_t newCrc = CRC16_Modbus(pByteData, dataLength);

    // ========================================================================
    // WEAR LEVELING: skip write if data is identical
    // ========================================================================
    EepromI2cHeader_t existingHeader;

    if (HAL_OK == I2C_ReadBytes(handle, handle->startAddr,
                                (uint8_t*)&existingHeader, sizeof(EepromI2cHeader_t)))
    {
        if (existingHeader.DataLength == dataLength && existingHeader.Crc16 == newCrc)
        {
            if (dataLength > EEPROM_I2C_MAX_BUFFER_SIZE)
                return EEPROM_I2C_ERR_SIZE;

            uint8_t tempBuffer[EEPROM_I2C_MAX_BUFFER_SIZE];

            if (HAL_OK == I2C_ReadBytes(handle, handle->startAddr + sizeof(EepromI2cHeader_t),
                                        tempBuffer, dataLength))
            {
                if (memcmp(tempBuffer, pByteData, dataLength) == 0)
                    return EEPROM_I2C_ERR_IDENTICAL;
            }
        }
    }

    // ========================================================================
    // Write header
    // ========================================================================
    EepromI2cHeader_t header = { .Crc16 = newCrc, .DataLength = dataLength };

    if (HAL_OK != I2C_WriteBytes(handle, handle->startAddr,
                                 (uint8_t*)&header, sizeof(EepromI2cHeader_t)))
        return EEPROM_I2C_ERR_I2C;

    // ========================================================================
    // Write data (zero-copy: write directly from user pointer)
    // ========================================================================
    if (HAL_OK != I2C_WriteBytes(handle, handle->startAddr + sizeof(EepromI2cHeader_t),
                                 pByteData, dataLength))
        return EEPROM_I2C_ERR_I2C;

    // ========================================================================
    // Verify write
    // ========================================================================
    EepromI2cHeader_t verifyHeader;

    if (HAL_OK != I2C_ReadBytes(handle, handle->startAddr,
                                (uint8_t*)&verifyHeader, sizeof(EepromI2cHeader_t)))
        return EEPROM_I2C_ERR_I2C;

    if (verifyHeader.Crc16 != newCrc || verifyHeader.DataLength != dataLength)
        return EEPROM_I2C_ERR;

    if (dataLength > EEPROM_I2C_MAX_BUFFER_SIZE)
        return EEPROM_I2C_ERR_SIZE;

    uint8_t verifyBuffer[EEPROM_I2C_MAX_BUFFER_SIZE];

    if (HAL_OK != I2C_ReadBytes(handle, handle->startAddr + sizeof(EepromI2cHeader_t),
                                verifyBuffer, dataLength))
        return EEPROM_I2C_ERR_I2C;

    if (memcmp(verifyBuffer, pByteData, dataLength) != 0)
        return EEPROM_I2C_ERR;

    return EEPROM_I2C_OK;
}

EepromI2cStatus_t EEPROM_I2C_Read(EepromI2cHandle_t *handle, void *pData, uint16_t dataLength)
{
    if (handle == NULL || pData == NULL)
        return EEPROM_I2C_ERR;

    if (dataLength > handle->maxDataSize)
        return EEPROM_I2C_ERR_SIZE;

    uint8_t *pByteData = (uint8_t*)pData;

    // Read header
    EepromI2cHeader_t header;

    if (HAL_OK != I2C_ReadBytes(handle, handle->startAddr,
                                (uint8_t*)&header, sizeof(EepromI2cHeader_t)))
        return EEPROM_I2C_ERR_I2C;

    if (header.DataLength != dataLength)
        return EEPROM_I2C_ERR_SIZE;

    // Read data (zero-copy: read directly into user buffer)
    if (HAL_OK != I2C_ReadBytes(handle, handle->startAddr + sizeof(EepromI2cHeader_t),
                                pByteData, dataLength))
        return EEPROM_I2C_ERR_I2C;

    // Verify CRC
    if (CRC16_Modbus(pByteData, dataLength) != header.Crc16)
        return EEPROM_I2C_ERR_CRC;

    return EEPROM_I2C_OK;
}

EepromI2cStatus_t EEPROM_I2C_Validate(EepromI2cHandle_t *handle, uint16_t *pExpectedLength)
{
    if (handle == NULL)
        return EEPROM_I2C_ERR;

    EepromI2cHeader_t header;

    if (HAL_OK != I2C_ReadBytes(handle, handle->startAddr,
                                (uint8_t*)&header, sizeof(EepromI2cHeader_t)))
        return EEPROM_I2C_ERR_I2C;

    if (header.DataLength > handle->maxDataSize)
        return EEPROM_I2C_ERR_SIZE;

    if (pExpectedLength != NULL)
        *pExpectedLength = header.DataLength;

    if (header.DataLength > EEPROM_I2C_MAX_BUFFER_SIZE)
        return EEPROM_I2C_ERR_SIZE;

    uint8_t tempBuffer[EEPROM_I2C_MAX_BUFFER_SIZE];

    if (HAL_OK != I2C_ReadBytes(handle, handle->startAddr + sizeof(EepromI2cHeader_t),
                                tempBuffer, header.DataLength))
        return EEPROM_I2C_ERR_I2C;

    if (CRC16_Modbus(tempBuffer, header.DataLength) != header.Crc16)
        return EEPROM_I2C_ERR_CRC;

    return EEPROM_I2C_OK;
}

EepromI2cStatus_t EEPROM_I2C_Erase(EepromI2cHandle_t *handle)
{
    if (handle == NULL)
        return EEPROM_I2C_ERR;

    EepromI2cHeader_t emptyHeader = {0};

    if (HAL_OK != I2C_WriteBytes(handle, handle->startAddr,
                                 (uint8_t*)&emptyHeader, sizeof(EepromI2cHeader_t)))
        return EEPROM_I2C_ERR_I2C;

    return EEPROM_I2C_OK;
}
