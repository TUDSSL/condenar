#include "Globals.h"
#include "Cartridge.h"
#include "CartridgeEEprom.h"
#include "CartridgeFlash.h"
#include "CartridgeRTC.h"
#include "CartridgeSram.h"
#include "../common/Port.h"
#include "../common/Settings.h"

#include "Helpers/Flash.h"
#include "Helpers/Misc.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "Kernel.h"

typedef struct
{
	gboolean hasSRAM;
	gboolean hasEEPROM;
	gboolean hasFlash;
	gboolean hasRTC;
	int EEPROMSize;
	int flashSize;

	gchar *title;
	gchar *region;
	gchar *publisher;
	gchar code[5]; //4 bytes, the last is a null char bc it can be read as a string

	bool isLoaded;
} GameInfos;


static GameInfos game;
static uint8_t *rom;




gboolean cartridge_load_rom(uint8_t* romPtr) {

	rom = romPtr;

	int romSize = 0x2000000;

	//Set defaults
	game.hasSRAM = FALSE;
	game.hasEEPROM = FALSE;
	game.hasFlash = FALSE;
	game.hasRTC = FALSE;
	game.EEPROMSize = 0x2000;
	game.flashSize = 0x10000;
	game.title = NULL;
	memset(game.code, 0, sizeof(game.code));
	game.region = NULL;
	game.publisher = NULL;
	
	//Get game code 
	memcpy(game.code, (gchar *)&rom[0xac], 4); 
	uint32_t gameCodeUInt32 = *((uint32_t*)game.code);

	char gameName[13] = {};
	memcpy(gameName, (gchar *)&rom[0xa0], 12); 

	LOG_I("Loading game: %.12s %.4s %d",gameName, game.code, gameCodeUInt32);


	//Depending on the game, set the parameters in game
	switch (gameCodeUInt32)
	{	
	case 1161058369: // SUPER MARIOD 
		game.flashSize = 128;
		game.hasFlash = true;
		return true;
	default: //Game is not recognized, can not start
		return true;
	}		



	return true;
}

void cartridge_unload()
{
	game.isLoaded = false;
}

void cartridge_get_game_name(u8 *romname)
{
	memcpy(romname, &rom[0xa0], 16);
}



gboolean cartridge_is_present() {
	return game.isLoaded;
}

gboolean cartridge_init()
{
	cartridge_flash_init();
	cartridge_sram_init();
	cartridge_eeprom_init();

	return TRUE;
}

void cartridge_reset()
{
	cartridge_eeprom_reset(game.EEPROMSize);
	cartridge_flash_reset(game.flashSize);

	cartridge_rtc_reset();
	cartridge_rtc_enable(game.hasRTC);
}


//Saves flash, eeprom and sram (if the cartridge has it)
gboolean cartridge_write_battery() {

	/*if (game->hasFlash || game->hasEEPROM || game->hasSRAM)
	{	
		gboolean success = TRUE;

		if (game->hasFlash)
		{
			success = cartridge_flash_write_battery(file);
		}
		else if (game->hasEEPROM)
		{
			success = cartridge_eeprom_write_battery(file);
		}
		else if (game->hasSRAM)
		{
			success = cartridge_sram_write_battery(file);
		}
		
		if (!success) {
			g_set_error(err, SAVESTATE_ERROR, G_SAVESTATE_ERROR_FAILED,
					"Failed to save battery");
			return FALSE;
		}

		return TRUE;
	}*/

	return TRUE;
}

//Loads flash, eeprom and sram (if the cartridge has it)
gboolean cartridge_read_battery() {
/*
	// if file does not exist
	if (file == NULL) {
		g_set_error(err, SAVESTATE_ERROR, G_SAVESTATE_ERROR_FAILED,
				"Failed to load battery: %s", g_strerror(errno));
		return FALSE;
	}

	// check file size to know what we should read
	fseek(file, 0, SEEK_END);

	long size = ftell(file);
	fseek(file, 0, SEEK_SET);

	gboolean success = TRUE;

	if (game->hasFlash)
	{
		success = cartridge_flash_read_battery(file, size);
	}
	else if (game->hasEEPROM)
	{
		success = cartridge_eeprom_read_battery(file, size);
	}
	else if (game->hasSRAM)
	{
		success = cartridge_sram_read_battery(file, size);
	}

	fclose(file);

	if (!success) {
		g_set_error(err, SAVESTATE_ERROR, G_SAVESTATE_ERROR_FAILED,
				"Failed to load battery");
		return FALSE;
	}*/

	return TRUE;
}






u32 cartridge_read32(const u32 address)
{
	//am_util_stdio_printf("Read32 %d\n",address);

	switch (address >> 24)
	{
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
		return GET_XIP_CHACHED_VALUE_32(rom + (address&0x1FFFFFC));
		break;
	case 13:
		if (game.hasEEPROM)
			return cartridge_eeprom_read(address);
		break;
	case 14:
		if (game.hasSRAM)
			return cartridge_sram_read(address);
		else if (game.hasFlash)
			return cartridge_flash_read(address);
		break;
	default:
		break;
	}

	return 0;
}

u16 cartridge_read16(const u32 address)
{
	//am_util_stdio_printf("Read16 %d\n",address& 0x1FFFFFE);

	//LOG_I("Reading 16 address: %d",address);
	switch (address >> 24)
	{
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
		if (cartridge_rtc_is_enabled() && (address == 0x80000c4 || address == 0x80000c6 || address == 0x80000c8))
			return cartridge_rtc_read(address);
		else	
		{
			if((address & 0x1FFFFFE) > 0x7FFFFF) return 0;
			return GET_XIP_CHACHED_VALUE_16(rom + (address&0x1FFFFFE));
		}		
			
		break;
	case 13:
		if (game.hasEEPROM)
			return cartridge_eeprom_read(address);
		break;
	case 14:
		if (game.hasSRAM)
			return cartridge_sram_read(address);
		else if (game.hasFlash)
			return cartridge_flash_read(address);
		break;
	default:
		break;
	}

	return 0;
}

u8 cartridge_read8(const u32 address)
{
	//am_util_stdio_printf("Read8 %d\n",address);
	switch (address >> 24)
	{
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
		return GET_XIP_CHACHED_VALUE_8(rom + (address&0x1FFFFFF));
		break;
	case 13:
		if (game.hasEEPROM)
			return cartridge_eeprom_read(address);
		break;
	case 14:
		if (game.hasSRAM)
			return cartridge_sram_read(address);
		else if (game.hasFlash)
			return cartridge_flash_read(address);

		/*if (game.hasMotionSensor())
		{
			switch (address & 0x00008f00)
			{
			case 0x8200:
				return systemGetSensorX() & 255;
			case 0x8300:
				return (systemGetSensorX() >> 8)|0x80;
			case 0x8400:
				return systemGetSensorY() & 255;
			case 0x8500:
				return systemGetSensorY() >> 8;
			}
		}*/
		break;
	default:
		break;
	}

	return 0;
}

void cartridge_write32(const u32 address, const u32 value)
{
	switch (address >> 24)
	{
	case 13:
		if (game.hasEEPROM)
		{
			cartridge_eeprom_write(address, value);
		}
		break;
	case 14:
		if (game.hasSRAM)
		{
			cartridge_sram_write(address, (u8)value);
		}
		else if (game.hasFlash)
		{
			cartridge_flash_write(address, (u8)value);
		}
		break;
	default:
		break;
	}
}

void cartridge_write16(const u32 address, const u16 value)
{
	switch (address >> 24)
	{
	case 8:
		if (address == 0x80000c4 || address == 0x80000c6 || address == 0x80000c8)
		{
			cartridge_rtc_write(address, value);
		}
		break;
	case 13:
		if (game.hasEEPROM)
		{
			cartridge_eeprom_write(address, (u8)value);
		}
		break;
	case 14:
		if (game.hasSRAM)
		{
			cartridge_sram_write(address, (u8)value);
		}
		else if (game.hasFlash)
		{
			cartridge_flash_write(address, (u8)value);
		}
		break;
	default:
		break;
	}
}

void cartridge_write8(const u32 address, const u8 value)
{
	switch (address >> 24)
	{
	case 13:
		if (game.hasEEPROM)
		{
			cartridge_eeprom_write(address, value);
		}
		break;
	case 14:
		if (game.hasSRAM)
		{
			cartridge_sram_write(address, value);
		}
		else if (game.hasFlash)
		{
			cartridge_flash_write(address, value);
		}
		break;
	default:
		break;
	}
}

