/* mbed Microcontroller Library
 *******************************************************************************
 * Copyright (c) 2017, STMicroelectronics
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of STMicroelectronics nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************
 */

#include "flash_api.h"
#include "flash_data.h"
#include "mbed_critical.h"

#if DEVICE_FLASH

#if defined (STM32F427xx) || defined (STM32F437xx)
#define FLASH_SIZE      (uint32_t) 0x100000
#endif
 
 static uint32_t GetSector(uint32_t Address);
static uint32_t GetSectorSize(uint32_t Sector);

int32_t flash_init(flash_t *obj)
{
    /* Allow Access to Flash control registers and user Falsh */
    if (HAL_FLASH_Unlock()) {
        return -1;
    } else {
        return 0;
    }
}
int32_t flash_free(flash_t *obj)
{
    /* Disable the Flash option control register access (recommended to protect
    the option Bytes against possible unwanted operations) */
    if (HAL_FLASH_Lock()) {
        return -1;
    } else {
        return 0;
    }
}
int32_t flash_erase_sector(flash_t *obj, uint32_t address)
{
    /*Variable used for Erase procedure*/
    static FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t FirstSector;
    uint32_t SectorError = 0;

    if ((address >= (FLASH_BASE + FLASH_SIZE)) || (address < FLASH_BASE)) {

        return -1;
    }

    /* Get the 1st sector to erase */
    FirstSector = GetSector(address);

    /* Fill EraseInit structure*/
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    EraseInitStruct.Sector = FirstSector;
    EraseInitStruct.NbSectors = 1;
    if(HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK){
        return -1;
    } else {
        return 0;
    }
}

int32_t flash_program_page(flash_t *obj, uint32_t address, const uint8_t *data, uint32_t size)
{

    if ((address >= (FLASH_BASE + FLASH_SIZE)) || (address < FLASH_BASE)) {
        return -1;
    }

  /* Note: If an erase operation in Flash memory also concerns data in the data or instruction cache,
     you have to make sure that these data are rewritten before they are accessed during code
     execution. If this cannot be done safely, it is recommended to flush the caches by setting the
     DCRST and ICRST bits in the FLASH_CR register. */
    __HAL_FLASH_DATA_CACHE_DISABLE();
    __HAL_FLASH_INSTRUCTION_CACHE_DISABLE();

    __HAL_FLASH_DATA_CACHE_RESET();
    __HAL_FLASH_INSTRUCTION_CACHE_RESET();

    __HAL_FLASH_INSTRUCTION_CACHE_ENABLE();
    __HAL_FLASH_DATA_CACHE_ENABLE();

    while (size > 0) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, address, (uint64_t)*data) != HAL_OK) {
            return -1;
        } else {
            size--;
            address++;
            data++;
        }
    }
    return 0;
}

uint32_t flash_get_sector_size(const flash_t *obj, uint32_t address)
{

    if ((address >= (FLASH_BASE + FLASH_SIZE)) || (address < FLASH_BASE)) {
        return MBED_FLASH_INVALID_SIZE;
    }

    return (GetSectorSize(GetSector(address)));
}

uint32_t flash_get_page_size(const flash_t *obj)
{
    // not applicable for STM32F4
    return (0x4000); // minimum sector size
}
uint32_t flash_get_start_address(const flash_t *obj)
{
    return FLASH_BASE;
}
uint32_t flash_get_size(const flash_t *obj)
{
    return FLASH_SIZE;
}

/**
  * @brief  Gets the sector of a given address
  * @param  None
  * @retval The sector of a given address
  */
static uint32_t GetSector(uint32_t address)
{
    uint32_t sector = 0;
    uint32_t tmp = address - ADDR_FLASH_SECTOR_0;
    if (address < ADDR_FLASH_SECTOR_4) { // 16k sectorsize
        sector += tmp >>14;
    } else if (address < ADDR_FLASH_SECTOR_5) { //64k sector size
        sector += FLASH_SECTOR_4;
    } else {
        sector += 4 + (tmp >>17);
    }
  return sector;
}

/**
  * @brief  Gets sector Size
  * @param  None
  * @retval The size of a given sector
  */
static uint32_t GetSectorSize(uint32_t Sector)
{
    uint32_t sectorsize = 0x00;
    if((Sector == FLASH_SECTOR_0) || (Sector == FLASH_SECTOR_1) || (Sector == FLASH_SECTOR_2) ||\
       (Sector == FLASH_SECTOR_3)) {
        sectorsize = 16 * 1024;
    } else if((Sector == FLASH_SECTOR_4)){
        sectorsize = 64 * 1024;
    } else {
        sectorsize = 128 * 1024;
    }
    return sectorsize;
}

#endif