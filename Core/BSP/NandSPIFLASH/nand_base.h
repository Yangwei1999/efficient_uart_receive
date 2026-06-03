//
// Created by yangwei on 2026/6/2.
//

#ifndef RING_UART_NAND_BASE_H
#define RING_UART_NAND_BASE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>

#define NAND_FLASH_JEDEC_MANUFACTURER_ID      0xEFU
#define NAND_FLASH_JEDEC_MEMORY_TYPE_ID       0xAAU
#define NAND_FLASH_JEDEC_CAPACITY_ID          0x22U

#define NAND_FLASH_PAGE_SIZE                  2048U
#define NAND_FLASH_SPARE_SIZE                 128U
#define NAND_FLASH_BUFFER_SIZE                (NAND_FLASH_PAGE_SIZE + NAND_FLASH_SPARE_SIZE)
#define NAND_FLASH_PAGES_PER_BLOCK            64U
#define NAND_FLASH_BLOCK_SIZE                 (NAND_FLASH_PAGE_SIZE * NAND_FLASH_PAGES_PER_BLOCK)
#define NAND_FLASH_PAGE_COUNT                 131072U
#define NAND_FLASH_BLOCK_COUNT                2048U

#define NAND_FLASH_STATUS_REG1_ADDR           0xA0U
#define NAND_FLASH_STATUS_REG2_ADDR           0xB0U
#define NAND_FLASH_STATUS_REG3_ADDR           0xC0U

#define NAND_FLASH_STATUS3_BUSY               0x01U
#define NAND_FLASH_STATUS3_WEL                0x02U
#define NAND_FLASH_STATUS3_EFAIL              0x04U
#define NAND_FLASH_STATUS3_PFAIL              0x08U

typedef struct
{
    char name[16];
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
    uint8_t jedec_id[3];
} nand_spi_base_t;

typedef struct
{
    uint8_t manufacturer_id;
    uint8_t memory_type_id;
    uint8_t capacity_id;
} nand_flash_id_t;

HAL_StatusTypeDef NAND_FLASH_Reset(nand_spi_base_t *nand_base);
HAL_StatusTypeDef NAND_FLASH_Init(nand_spi_base_t *nand_base);
HAL_StatusTypeDef NAND_FLASH_UnlockAllProtection(nand_spi_base_t *nand_base);
HAL_StatusTypeDef NAND_FLASH_ReadJEDECID(nand_spi_base_t *nand_base, nand_flash_id_t *id);
HAL_StatusTypeDef NAND_FLASH_ReadStatusRegister(nand_spi_base_t *nand_base, uint8_t reg_addr, uint8_t *value);
HAL_StatusTypeDef NAND_FLASH_WriteStatusRegister(nand_spi_base_t *nand_base, uint8_t reg_addr, uint8_t value);
HAL_StatusTypeDef NAND_FLASH_WriteEnable(nand_spi_base_t *nand_base);
HAL_StatusTypeDef NAND_FLASH_WriteDisable(nand_spi_base_t *nand_base);
HAL_StatusTypeDef NAND_FLASH_WaitReady(nand_spi_base_t *nand_base, uint32_t timeout_ms);
HAL_StatusTypeDef NAND_FLASH_DeepPowerDown(nand_spi_base_t *nand_base);
HAL_StatusTypeDef NAND_FLASH_ReleasePowerDown(nand_spi_base_t *nand_base);
HAL_StatusTypeDef NAND_FLASH_ReadPageToBuffer(nand_spi_base_t *nand_base, uint32_t page_addr);
HAL_StatusTypeDef NAND_FLASH_ReadBuffer(nand_spi_base_t *nand_base, uint16_t column_addr, uint8_t *buffer, uint16_t length);
HAL_StatusTypeDef NAND_FLASH_ReadPage(nand_spi_base_t *nand_base, uint32_t page_addr, uint8_t *buffer, uint16_t length);
HAL_StatusTypeDef NAND_FLASH_IsBadBlockByIndex(nand_spi_base_t *nand_base, uint32_t block_index, uint8_t *is_bad);
HAL_StatusTypeDef NAND_FLASH_FindGoodBlock(nand_spi_base_t *nand_base, uint32_t start_block, uint32_t *good_block);
HAL_StatusTypeDef NAND_FLASH_LoadProgramData(nand_spi_base_t *nand_base, uint16_t column_addr, const uint8_t *buffer, uint16_t length);
HAL_StatusTypeDef NAND_FLASH_ProgramExecute(nand_spi_base_t *nand_base, uint32_t page_addr);
HAL_StatusTypeDef NAND_FLASH_ProgramPage(nand_spi_base_t *nand_base, uint32_t page_addr, const uint8_t *buffer, uint16_t length);
HAL_StatusTypeDef NAND_FLASH_BeginEraseBlockByPage(nand_spi_base_t *nand_base, uint32_t page_addr);
HAL_StatusTypeDef NAND_FLASH_BeginEraseBlockByIndex(nand_spi_base_t *nand_base, uint32_t block_index);
HAL_StatusTypeDef NAND_FLASH_EraseBlockByPage(nand_spi_base_t *nand_base, uint32_t page_addr);
HAL_StatusTypeDef NAND_FLASH_EraseBlockByIndex(nand_spi_base_t *nand_base, uint32_t block_index);

void NAND_FLASH_ReadID(nand_spi_base_t *nand_base);
void NADN_FLASH_READ_STATUS(nand_spi_base_t *nand_base);
void NADN_FLASH_Erase_block(nand_spi_base_t *nand_base);

#ifdef __cplusplus
}
#endif

#endif //RING_UART_NAND_BASE_H
