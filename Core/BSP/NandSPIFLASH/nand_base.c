//
// Created by yangwei on 2026/6/2.
//

#include "nand_base.h"
#include "stdio.h"

#define NAND_FLASH_CMD_RESET_ENABLE          0x66U
#define NAND_FLASH_CMD_RESET_DEVICE          0x99U
#define NAND_FLASH_CMD_READ_JEDEC_ID         0x9FU
#define NAND_FLASH_CMD_READ_STATUS           0x0FU
#define NAND_FLASH_CMD_WRITE_STATUS          0x1FU
#define NAND_FLASH_CMD_WRITE_ENABLE          0x06U
#define NAND_FLASH_CMD_WRITE_DISABLE         0x04U
#define NAND_FLASH_CMD_BLOCK_ERASE           0xD8U
#define NAND_FLASH_CMD_LOAD_PROGRAM_DATA     0x02U
#define NAND_FLASH_CMD_PROGRAM_EXECUTE       0x10U
#define NAND_FLASH_CMD_PAGE_DATA_READ        0x13U
#define NAND_FLASH_CMD_READ_DATA             0x03U
#define NAND_FLASH_CMD_DEEP_POWER_DOWN       0xB9U
#define NAND_FLASH_CMD_RELEASE_POWER_DOWN    0xABU

#define NAND_FLASH_SR1_BP_TB_MASK            0x7CU

#define REG_PROTECTION      0xA0
#define REG_CONFIG          0xB0
#define REG_STATUS          0xC0

static inline uint8_t nand_is_valid(const nand_spi_base_t *nand_base)
{
    return (nand_base != NULL) &&
           (nand_base->hspi != NULL) &&
           (nand_base->cs_port != NULL);
}

static void nand_cs_low(const nand_spi_base_t *nand_base)
{
    HAL_GPIO_WritePin(nand_base->cs_port, nand_base->cs_pin, GPIO_PIN_RESET);
}

static void nand_cs_high(const nand_spi_base_t *nand_base)
{
    HAL_GPIO_WritePin(nand_base->cs_port, nand_base->cs_pin, GPIO_PIN_SET);
}

static HAL_StatusTypeDef nand_transmit(const nand_spi_base_t *nand_base, const uint8_t *data, uint16_t length)
{
    if (!nand_is_valid(nand_base) || (data == NULL && length != 0U)) {
        return HAL_ERROR;
    }

    return HAL_SPI_Transmit(nand_base->hspi, (uint8_t *)data, length, HAL_MAX_DELAY);
}

static HAL_StatusTypeDef nand_receive(const nand_spi_base_t *nand_base, uint8_t *data, uint16_t length)
{
    if (!nand_is_valid(nand_base) || (data == NULL && length != 0U)) {
        return HAL_ERROR;
    }

    return HAL_SPI_Receive(nand_base->hspi, data, length, HAL_MAX_DELAY);
}

static HAL_StatusTypeDef nand_command_write(const nand_spi_base_t *nand_base, const uint8_t *tx, uint16_t tx_len)
{
    HAL_StatusTypeDef status = HAL_ERROR;

    if (!nand_is_valid(nand_base) || (tx == NULL && tx_len != 0U)) {
        return HAL_ERROR;
    }

    nand_cs_low(nand_base);
    status = nand_transmit(nand_base, tx, tx_len);
    nand_cs_high(nand_base);

    return status;
}

static HAL_StatusTypeDef nand_command_write_then_data(const nand_spi_base_t *nand_base,
                                                      const uint8_t *header,
                                                      uint16_t header_len,
                                                      const uint8_t *data,
                                                      uint16_t data_len)
{
    HAL_StatusTypeDef status = HAL_ERROR;

    if (!nand_is_valid(nand_base) || (header == NULL && header_len != 0U) || (data == NULL && data_len != 0U)) {
        return HAL_ERROR;
    }

    nand_cs_low(nand_base);
    status = nand_transmit(nand_base, header, header_len);
    if (status == HAL_OK) {
        status = nand_transmit(nand_base, data, data_len);
    }
    nand_cs_high(nand_base);

    return status;
}

static HAL_StatusTypeDef nand_command_write_then_read(const nand_spi_base_t *nand_base,
                                                      const uint8_t *header,
                                                      uint16_t header_len,
                                                      uint8_t *data,
                                                      uint16_t data_len)
{
    HAL_StatusTypeDef status = HAL_ERROR;

    if (!nand_is_valid(nand_base) || (header == NULL && header_len != 0U) || (data == NULL && data_len != 0U)) {
        return HAL_ERROR;
    }

    nand_cs_low(nand_base);
    status = nand_transmit(nand_base, header, header_len);
    if (status == HAL_OK) {
        status = nand_receive(nand_base, data, data_len);
    }
    nand_cs_high(nand_base);

    return status;
}

static void nand_pack_page_address(uint32_t page_addr, uint8_t address[3])
{
    address[0] = (uint8_t)((page_addr >> 16U) & 0xFFU);
    address[1] = (uint8_t)((page_addr >> 8U) & 0xFFU);
    address[2] = (uint8_t)(page_addr & 0xFFU);
}

HAL_StatusTypeDef NAND_FLASH_Reset(nand_spi_base_t *nand_base)
{
    const uint8_t enable_reset = NAND_FLASH_CMD_RESET_ENABLE;
    const uint8_t reset_device = NAND_FLASH_CMD_RESET_DEVICE;
    HAL_StatusTypeDef status;

    if (!nand_is_valid(nand_base)) {
        return HAL_ERROR;
    }

    status = nand_command_write(nand_base, &enable_reset, 1U);
    if (status != HAL_OK) {
        return status;
    }

    return nand_command_write(nand_base, &reset_device, 1U);
}

HAL_StatusTypeDef NAND_FLASH_UnlockAllProtection(nand_spi_base_t *nand_base)
{
    uint8_t sr1 = 0U;
    HAL_StatusTypeDef status;

    if (!nand_is_valid(nand_base)) {
        return HAL_ERROR;
    }

    status = NAND_FLASH_ReadStatusRegister(nand_base, NAND_FLASH_STATUS_REG1_ADDR, &sr1);
    if (status != HAL_OK) {
        return status;
    }

    if ((sr1 & NAND_FLASH_SR1_BP_TB_MASK) == 0U) {
        return HAL_OK;
    }

    sr1 &= (uint8_t)~NAND_FLASH_SR1_BP_TB_MASK;

    status = NAND_FLASH_WriteEnable(nand_base);
    if (status != HAL_OK) {
        return status;
    }

    status = NAND_FLASH_WriteStatusRegister(nand_base, NAND_FLASH_STATUS_REG1_ADDR, sr1);
    if (status != HAL_OK) {
        return status;
    }

    return NAND_FLASH_WaitReady(nand_base, HAL_MAX_DELAY);
}

HAL_StatusTypeDef NAND_FLASH_Init(nand_spi_base_t *nand_base)
{
    HAL_StatusTypeDef status;

    if (!nand_is_valid(nand_base)) {
        return HAL_ERROR;
    }

    status = NAND_FLASH_Reset(nand_base);
    if (status != HAL_OK) {
        return status;
    }

    status = NAND_FLASH_WaitReady(nand_base, HAL_MAX_DELAY);
    if (status != HAL_OK) {
        return status;
    }

    return NAND_FLASH_UnlockAllProtection(nand_base);
}

HAL_StatusTypeDef NAND_FLASH_ReadJEDECID(nand_spi_base_t *nand_base, nand_flash_id_t *id)
{
    uint8_t header[2] = {NAND_FLASH_CMD_READ_JEDEC_ID, 0x00U};
    uint8_t response[3] = {0U, 0U, 0U};
    HAL_StatusTypeDef status;

    if (!nand_is_valid(nand_base)) {
        return HAL_ERROR;
    }

    status = nand_command_write_then_read(nand_base, header, sizeof(header), response, sizeof(response));
    if (status != HAL_OK) {
        return status;
    }

    nand_base->jedec_id[0] = response[0];
    nand_base->jedec_id[1] = response[1];
    nand_base->jedec_id[2] = response[2];

    if (id != NULL) {
        id->manufacturer_id = response[0];
        id->memory_type_id = response[1];
        id->capacity_id = response[2];
    }

    return HAL_OK;
}

HAL_StatusTypeDef NAND_FLASH_ReadStatusRegister(nand_spi_base_t *nand_base, uint8_t reg_addr, uint8_t *value)
{
    uint8_t header[2] = {NAND_FLASH_CMD_READ_STATUS, reg_addr};
    uint8_t status_value = 0U;
    HAL_StatusTypeDef status;

    if (!nand_is_valid(nand_base) || value == NULL) {
        return HAL_ERROR;
    }

    status = nand_command_write_then_read(nand_base, header, sizeof(header), &status_value, 1U);
    if (status == HAL_OK) {
        *value = status_value;
    }

    return status;
}

HAL_StatusTypeDef NAND_FLASH_WriteStatusRegister(nand_spi_base_t *nand_base, uint8_t reg_addr, uint8_t value)
{
    uint8_t tx[3] = {NAND_FLASH_CMD_WRITE_STATUS, reg_addr, value};

    if (!nand_is_valid(nand_base)) {
        return HAL_ERROR;
    }

    return nand_command_write(nand_base, tx, sizeof(tx));
}

HAL_StatusTypeDef NAND_FLASH_WriteEnable(nand_spi_base_t *nand_base)
{
    const uint8_t command = NAND_FLASH_CMD_WRITE_ENABLE;

    if (!nand_is_valid(nand_base)) {
        return HAL_ERROR;
    }

    return nand_command_write(nand_base, &command, 1U);
}

HAL_StatusTypeDef NAND_FLASH_WriteDisable(nand_spi_base_t *nand_base)
{
    const uint8_t command = NAND_FLASH_CMD_WRITE_DISABLE;

    if (!nand_is_valid(nand_base)) {
        return HAL_ERROR;
    }

    return nand_command_write(nand_base, &command, 1U);
}

HAL_StatusTypeDef NAND_FLASH_DeepPowerDown(nand_spi_base_t *nand_base)
{
    const uint8_t command = NAND_FLASH_CMD_DEEP_POWER_DOWN;

    if (!nand_is_valid(nand_base)) {
        return HAL_ERROR;
    }

    return nand_command_write(nand_base, &command, 1U);
}

HAL_StatusTypeDef NAND_FLASH_ReleasePowerDown(nand_spi_base_t *nand_base)
{
    const uint8_t command = NAND_FLASH_CMD_RELEASE_POWER_DOWN;

    if (!nand_is_valid(nand_base)) {
        return HAL_ERROR;
    }

    return nand_command_write(nand_base, &command, 1U);
}

HAL_StatusTypeDef NAND_FLASH_WaitReady(nand_spi_base_t *nand_base, uint32_t timeout_ms)
{
    uint32_t start_tick;
    uint8_t status = 0U;
    HAL_StatusTypeDef result;

    if (!nand_is_valid(nand_base)) {
        return HAL_ERROR;
    }

    start_tick = HAL_GetTick();

    for (;;) {
        result = NAND_FLASH_ReadStatusRegister(nand_base, NAND_FLASH_STATUS_REG3_ADDR, &status);
        if (result != HAL_OK) {
            return result;
        }

        if ((status & NAND_FLASH_STATUS3_BUSY) == 0U) {
            return HAL_OK;
        }

        if (timeout_ms != HAL_MAX_DELAY) {
            if ((HAL_GetTick() - start_tick) >= timeout_ms) {
                return HAL_TIMEOUT;
            }
        }

        HAL_Delay(1U);
    }
}

HAL_StatusTypeDef NAND_FLASH_ReadPageToBuffer(nand_spi_base_t *nand_base, uint32_t page_addr)
{
    uint8_t tx[4] = {NAND_FLASH_CMD_PAGE_DATA_READ, 0U, 0U, 0U};

    if (!nand_is_valid(nand_base) || (page_addr >= NAND_FLASH_PAGE_COUNT)) {
        return HAL_ERROR;
    }

    nand_pack_page_address(page_addr, &tx[1]);
    return nand_command_write(nand_base, tx, sizeof(tx));
}

HAL_StatusTypeDef NAND_FLASH_ReadBuffer(nand_spi_base_t *nand_base, uint16_t column_addr, uint8_t *buffer, uint16_t length)
{
    uint8_t tx[4] = {
        NAND_FLASH_CMD_READ_DATA,
        (uint8_t)((column_addr >> 8U) & 0xFFU),
        (uint8_t)(column_addr & 0xFFU),
        0x00U
    };

    if (!nand_is_valid(nand_base) || (buffer == NULL && length != 0U)) {
        return HAL_ERROR;
    }

    if (((uint32_t)column_addr + (uint32_t)length) > NAND_FLASH_BUFFER_SIZE) {
        return HAL_ERROR;
    }

    return nand_command_write_then_read(nand_base, tx, sizeof(tx), buffer, length);
}

HAL_StatusTypeDef NAND_FLASH_ReadPage(nand_spi_base_t *nand_base, uint32_t page_addr, uint8_t *buffer, uint16_t length)
{
    HAL_StatusTypeDef status;

    if (!nand_is_valid(nand_base) || (buffer == NULL && length != 0U)) {
        return HAL_ERROR;
    }

    status = NAND_FLASH_ReadPageToBuffer(nand_base, page_addr);
    if (status != HAL_OK) {
        return status;
    }

    status = NAND_FLASH_WaitReady(nand_base, HAL_MAX_DELAY);
    if (status != HAL_OK) {
        return status;
    }

    return NAND_FLASH_ReadBuffer(nand_base, 0U, buffer, length);
}

HAL_StatusTypeDef NAND_FLASH_LoadProgramData(nand_spi_base_t *nand_base, uint16_t column_addr, const uint8_t *buffer, uint16_t length)
{
    uint8_t header[3] = {
        NAND_FLASH_CMD_LOAD_PROGRAM_DATA,
        (uint8_t)((column_addr >> 8U) & 0xFFU),
        (uint8_t)(column_addr & 0xFFU)
    };

    if (!nand_is_valid(nand_base) || (buffer == NULL && length != 0U)) {
        return HAL_ERROR;
    }

    if (((uint32_t)column_addr + (uint32_t)length) > NAND_FLASH_BUFFER_SIZE) {
        return HAL_ERROR;
    }

    return nand_command_write_then_data(nand_base, header, sizeof(header), buffer, length);
}

HAL_StatusTypeDef NAND_FLASH_ProgramExecute(nand_spi_base_t *nand_base, uint32_t page_addr)
{
    uint8_t tx[4] = {NAND_FLASH_CMD_PROGRAM_EXECUTE, 0U, 0U, 0U};

    if (!nand_is_valid(nand_base) || (page_addr >= NAND_FLASH_PAGE_COUNT)) {
        return HAL_ERROR;
    }

    nand_pack_page_address(page_addr, &tx[1]);
    return nand_command_write(nand_base, tx, sizeof(tx));
}

HAL_StatusTypeDef NAND_FLASH_ProgramPage(nand_spi_base_t *nand_base, uint32_t page_addr, const uint8_t *buffer, uint16_t length)
{
    HAL_StatusTypeDef status;

    if (!nand_is_valid(nand_base) || (buffer == NULL && length != 0U)) {
        return HAL_ERROR;
    }

    status = NAND_FLASH_WriteEnable(nand_base);
    if (status != HAL_OK) {
        return status;
    }

    status = NAND_FLASH_LoadProgramData(nand_base, 0U, buffer, length);
    if (status != HAL_OK) {
        return status;
    }

    status = NAND_FLASH_ProgramExecute(nand_base, page_addr);
    if (status != HAL_OK) {
        return status;
    }

    return NAND_FLASH_WaitReady(nand_base, HAL_MAX_DELAY);
}

HAL_StatusTypeDef NAND_FLASH_BeginEraseBlockByPage(nand_spi_base_t *nand_base, uint32_t page_addr)
{
    uint8_t tx[4] = {NAND_FLASH_CMD_BLOCK_ERASE, 0U, 0U, 0U};
    HAL_StatusTypeDef status;

    if (!nand_is_valid(nand_base) || (page_addr >= NAND_FLASH_PAGE_COUNT)) {
        return HAL_ERROR;
    }

    status = NAND_FLASH_WriteEnable(nand_base);
    if (status != HAL_OK) {
        return status;
    }

    nand_pack_page_address(page_addr, &tx[1]);
    return nand_command_write(nand_base, tx, sizeof(tx));
}

HAL_StatusTypeDef NAND_FLASH_BeginEraseBlockByIndex(nand_spi_base_t *nand_base, uint32_t block_index)
{
    if (!nand_is_valid(nand_base) || (block_index >= NAND_FLASH_BLOCK_COUNT)) {
        return HAL_ERROR;
    }

    return NAND_FLASH_BeginEraseBlockByPage(nand_base, block_index * NAND_FLASH_PAGES_PER_BLOCK);
}

HAL_StatusTypeDef NAND_FLASH_EraseBlockByPage(nand_spi_base_t *nand_base, uint32_t page_addr)
{
    HAL_StatusTypeDef status;

    status = NAND_FLASH_BeginEraseBlockByPage(nand_base, page_addr);
    if (status != HAL_OK) {
        return status;
    }

    return NAND_FLASH_WaitReady(nand_base, HAL_MAX_DELAY);
}

HAL_StatusTypeDef NAND_FLASH_EraseBlockByIndex(nand_spi_base_t *nand_base, uint32_t block_index)
{
    if (!nand_is_valid(nand_base) || (block_index >= NAND_FLASH_BLOCK_COUNT)) {
        return HAL_ERROR;
    }

    return NAND_FLASH_EraseBlockByPage(nand_base, block_index * NAND_FLASH_PAGES_PER_BLOCK);
}

void NAND_FLASH_ReadID(nand_spi_base_t *nand_base)
{
    nand_flash_id_t id = {0U, 0U, 0U};

    if (NAND_FLASH_ReadJEDECID(nand_base, &id) == HAL_OK) {
        printf("NAND %s JEDEC ID: %02X %02X %02X\r\n",
               (nand_base != NULL) ? nand_base->name : "unknown",
               id.manufacturer_id,
               id.memory_type_id,
               id.capacity_id);
    } else {
        printf("NAND read JEDEC ID failed\r\n");
    }
}

void NADN_FLASH_READ_STATUS(nand_spi_base_t *nand_base)
{
    uint8_t status = 0U;

    if (NAND_FLASH_ReadStatusRegister(nand_base, NAND_FLASH_STATUS_REG3_ADDR, &status) == HAL_OK) {
        printf("NAND status3: 0x%02X (BUSY=%u WEL=%u EFAIL=%u PFAIL=%u)\r\n",
               status,
               (status & NAND_FLASH_STATUS3_BUSY) ? 1U : 0U,
               (status & NAND_FLASH_STATUS3_WEL) ? 1U : 0U,
               (status & NAND_FLASH_STATUS3_EFAIL) ? 1U : 0U,
               (status & NAND_FLASH_STATUS3_PFAIL) ? 1U : 0U);
    } else {
        printf("NAND read status failed\r\n");
    }
}

void NADN_FLASH_Erase_block(nand_spi_base_t *nand_base)
{
    if (NAND_FLASH_EraseBlockByIndex(nand_base, 0U) == HAL_OK) {
        printf("NAND erase block 0 done\r\n");
    } else {
        printf("NAND erase block 0 failed\r\n");
    }
}
