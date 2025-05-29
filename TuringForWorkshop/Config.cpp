#include "Config.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include <cstring>

uint32_t const Config::MAGIC = 0x434F4E46;
size_t const Config::FLASH_SIZE = 2 * 1024 * 1024;
size_t const Config::BLOCK_SIZE = 4096;
size_t const Config::OFFSET = Config::FLASH_SIZE - Config::BLOCK_SIZE;

static const uint8_t *CONFIG_FLASH_PTR = reinterpret_cast<const uint8_t *>(XIP_BASE + Config::OFFSET);

void Config::load(bool forceReset)
{
    std::memcpy(&config, CONFIG_FLASH_PTR, sizeof(Data));

    if (config.magic != Config::MAGIC || forceReset)
    {
        config = Data(); // Reset to defaults
        save();
    }
}

void Config::save()
{
    uint8_t flash_copy[Config::BLOCK_SIZE];
    std::memcpy(flash_copy, CONFIG_FLASH_PTR, Config::BLOCK_SIZE);

    if (std::memcmp(&config, flash_copy, sizeof(Data)) == 0)
    {
        return; // No need to write
    }

    uint8_t temp[Config::BLOCK_SIZE] = {0};
    std::memcpy(temp, &config, sizeof(Data));

    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(Config::OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(Config::OFFSET, temp, Config::BLOCK_SIZE);
    restore_interrupts(ints);
}

Config::Data &Config::get()
{
    return config;
}
