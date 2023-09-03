#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include "gb.h"
#include "Flash.h"

/* 16KB ROM banks */
#define GB_ROM_BANK_SIZE (16 * 1024)
/* 8KB RAM banks */
#define GB_RAM_BANK_SIZE (8 * 1024)

/* GB ROMs are at least 32KB (2 banks) */
#define GB_CART_MIN_SIZE (GB_ROM_BANK_SIZE * 2)

/* I think the biggest licensed GB cartridge is 8MB but let's add a margin in
 * case there are homebrews with even bigger carts. */
#define GB_CART_MAX_SIZE (32U * 1024 * 1024)

#define GB_CART_OFF_TITLE     0x134
#define GB_CART_OFF_GBC       0x143
#define GB_CART_OFF_TYPE      0x147
#define GB_CART_OFF_ROM_BANKS 0x148
#define GB_CART_OFF_RAM_BANKS 0x149

static void gb_cart_get_rom_title(struct gb *gb, char title[17]) {
     struct gb_cart *cart = &gb->cart;
     unsigned i;

     for (i = 0; i < 16; i++) {
          char c = cart->rom[GB_CART_OFF_TITLE + i];

          if (c == 0) {
               /* End-of-title */
               break;
          }

          if (!isprint(c)) {
               c = '?';
          }

          title[i] = c;
     }

     /* End of string */
     title[i] = '\0';
}

void gb_cart_load(struct gb *gb,uint8_t* romPtr,uint8_t* storageRAMPtr, int maxStorageRAMSize) {
     struct gb_cart *cart = &gb->cart;
     long l;
     size_t nread;
     char rom_title[17];
     bool has_battery_backup;

     cart->rom = NULL;
     cart->cur_rom_bank = 1;
     cart->ram = NULL;
     cart->cur_ram_bank = 0;
     cart->ram_write_protected = true;
     cart->mbc1_bank_ram = false;
     cart->save_file = NULL;
     cart->dirty_ram = false;
     cart->has_rtc = false;
     has_battery_backup = false;

     cart->rom = romPtr;

     /* Figure out the number of ROM banks for this cartridge */
     switch (cart->rom[GB_CART_OFF_ROM_BANKS]) {
     case 0:
          cart->rom_banks = 2;
          break;
     case 1:
          cart->rom_banks = 4;
          break;
     case 2:
          cart->rom_banks = 8;
          break;
     case 3:
          cart->rom_banks = 16;
          break;
     case 4:
          cart->rom_banks = 32;
          break;
     case 5:
          cart->rom_banks = 64;
          break;
     case 6:
          cart->rom_banks = 128;
          break;
     case 7:
          cart->rom_banks = 256;
          break;
     case 8:
          cart->rom_banks = 512;
          break;
     case 0x52:
          cart->rom_banks = 72;
          break;
     case 0x53:
          cart->rom_banks = 80;
          break;
     case 0x54:
          cart->rom_banks = 96;
          break;
     default:
          LOG_I("Unknown ROM size configuration: %x\n", cart->rom[GB_CART_OFF_ROM_BANKS]);
          goto error;
     }

     /* Figure out the number of RAM banks for this cartridge */
     switch (cart->rom[GB_CART_OFF_RAM_BANKS]) {
     case 0: /* No RAM */
          cart->ram_banks = 0;
          cart->ram_length = 0;
          break;
     case 1:
          /* One bank but only 2kB (so really 1/4 of a bank) */
          cart->ram_banks = 1;
          cart->ram_length = GB_RAM_BANK_SIZE / 4;
          break;
     case 2:
          cart->ram_banks = 1;
          cart->ram_length = GB_RAM_BANK_SIZE;
          break;
     case 3:
          cart->ram_banks = 4;
          cart->ram_length = GB_RAM_BANK_SIZE * 4;
          break;
     case 4:
          cart->ram_banks = 16;
          cart->ram_length = GB_RAM_BANK_SIZE * 16;
          break;
     default:
          LOG_I("Unknown RAM size configuration: %x\n", cart->rom[GB_CART_OFF_RAM_BANKS]);
          goto error;
     }

               
     uint32_t availableStorageSize = 0;
     uint8_t* readOnlyPtr = k_GetStorageReadOnlyAddressForApp(k_getCurrentApp(), &availableStorageSize);

     //Load the save file if possible (enough space)
     if(cart->ram_length <= availableStorageSize){
          memcpy(storageRAMPtr, readOnlyPtr, cart->ram_length);
          LOG_I("Loaded RAM save\n");
     }
  

     switch (cart->rom[GB_CART_OFF_TYPE]) {
     case 0x00:
          cart->model = GB_CART_SIMPLE;
          break;
     case 0x01: /* MBC1, no RAM */
     case 0x02: /* MBC1, with RAM */
     case 0x03: /* MBC1, with RAM and battery backup */
          cart->model = GB_CART_MBC1;
          break;
     case 0x05: /* MBC2 */
     case 0x06: /* MBC2 with battery backup */
          cart->model = GB_CART_MBC2;
          /* MBC2 always has 512 * 4bits of RAM available */
          cart->ram_banks = 1;
          /* Allocate 512 bytes for convenience, but only the low 4 bits should
           * be used */
          cart->ram_length = 512;
          break;
     case 0x0f: /* MBC3, with battery backup and RTC */
     case 0x10: /* MBC3, with RAM, battery backup and RTC */
     case 0x11: /* MBC3, no RAM */
     case 0x12: /* MBC3, with RAM */
     case 0x13: /* MBC3, with RAM and battery backup */
          cart->model = GB_CART_MBC3;
          break;
     case 0x19: /* MBC5, no RAM */
     case 0x1a: /* MBC5, with RAM */
     case 0x1b: /* MBC5, with RAM and battery backup */
          cart->model = GB_CART_MBC5;
          break;
     default:
          LOG_I("Unsupported cartridge type %x\n", cart->rom[GB_CART_OFF_TYPE]);
          goto error;
     }

     /* Check if cart has a battery for memory backup */
     switch (cart->rom[GB_CART_OFF_TYPE]) {
     case 0x03:
     case 0x06:
     case 0x09:
     case 0x0f:
     case 0x10:
     case 0x13:
     case 0x1b:
     case 0x1e:
     case 0xff:
          has_battery_backup = true;
     }

     /* Check if cart has an RTC */
     switch (cart->rom[GB_CART_OFF_TYPE]) {
     case 0xf:
     case 0x10:
          cart->has_rtc = true;
     }

     /* Allocate RAM buffer */
     if (cart->ram_length > 0) {
          if(cart->ram_length>maxStorageRAMSize){
               LOG_I("RAM storage size is too big: %d\n. Max is %d", cart->ram_length, maxStorageRAMSize);
               goto error;
          }
          cart->ram = storageRAMPtr; //(of size cart->ram_length)         
     } else if (!cart->has_rtc) {
          /* Memory backup without RAM or RTC doesn't make a lot of sense */
          has_battery_backup = false;
     }

     if (has_battery_backup) {
          
          //TODO: load save data to cart->ram, with cart->ram_length size           

          /*if (cart->has_rtc) {
               gb_rtc_load(gb, f);
          }*/          
          
     } else {
          /* No save file */
          if (cart->has_rtc) {
               gb_rtc_init(gb);
          }
     }

     /* See if we have a DMG or GBC game */
     gb->gbc = (cart->rom[GB_CART_OFF_GBC] & 0x80);

     gb_cart_get_rom_title(gb, rom_title);

     LOG_I("Succesfully Loaded");
     LOG_I("Title: '%s'", rom_title);
     LOG_I("ROM banks: %u (%uKiB)", cart->rom_banks, cart->rom_banks * GB_ROM_BANK_SIZE / 1024);
     LOG_I("RAM banks: %u (%uKiB)", cart->ram_banks, cart->ram_length / 1024);
     return;

error:
     if (cart->rom) {
          cart->rom = NULL;
     }

     if (cart->ram) {
          cart->ram = NULL;
     }

     LOG_E("Failed to load cartridge!");
     SpinLoop();
}

static void gb_cart_ram_save(struct gb *gb) {
     struct gb_cart *cart = &gb->cart;

     uint32_t availableStorageSize = 0;     
     uint8_t* readOnlyPtr = k_GetStorageReadOnlyAddressForApp(k_getCurrentApp(), &availableStorageSize);

     //Load the save file if possible (enough space)
     if(cart->ram_length <= availableStorageSize){
           k_WriteToAppStorage(k_getCurrentApp(), 0, gb->cart.ram, cart->ram_length);
            LOG_I("Saved RAM");
     }
    
    // memcpy(, storageRAMPtr, cart->ram_length);
     /*FILE *f;

     if (cart->save_file == NULL) {
          //No battery backup, nothing to do
          return;
     }

     if (!cart->dirty_ram) {
          //No changes to RAM since last save, nothing to do
          return;
     }

     f = fopen(cart->save_file, "wb");
     if (f == NULL) {
          LOG_I("Can't create or open save file '%s': %s",
                  cart->save_file, strerror(errno));
          SpinLoop();
     }

     if (cart->ram_length > 0) {
          //Dump RAM to file
          if (fwrite(cart->ram, 1, cart->ram_length, f) < 0) {
               perror("fwrite failed");
               fclose(f);
               SpinLoop();
          }
     }

     if (cart->has_rtc) {
          gb_rtc_dump(gb, f);
     }

     fflush(f);
     fclose(f);*/

     //TODO: Implement saving

    
     cart->dirty_ram = false;
}

void gb_cart_unload(struct gb *gb) {
     gb_cart_ram_save(gb);
}

uint8_t gb_cart_rom_readb(struct gb *gb, uint16_t addr) {
     struct gb_cart *cart = &gb->cart;
     unsigned rom_off = addr;

     switch (cart->model) {
     case GB_CART_SIMPLE:
          /* No mapper */
          break;
     case GB_CART_MBC1:
          if (addr >= GB_ROM_BANK_SIZE) {
               /* Bank 1 can be remapped through this controller */
               unsigned bank = cart->cur_rom_bank;

               if (cart->mbc1_bank_ram) {
                    /* When MBC1 is configured to bank RAM it can only address
                     * 16 ROM banks */
                    bank %= 32;
               } else {
                    bank %= 128;
               }

               if (bank == 0) {
                    /* Bank 0 can't be mirrored that way, using a bank of 0 is
                     * the same thing as using 1 */
                    bank = 1;
               }

               bank %= cart->rom_banks;

               rom_off += (bank - 1) * GB_ROM_BANK_SIZE;
          }
          break;
     case GB_CART_MBC2:
     case GB_CART_MBC3:
          if (addr >= GB_ROM_BANK_SIZE) {
               rom_off += (cart->cur_rom_bank - 1) * GB_ROM_BANK_SIZE;
          }
          break;
     case GB_CART_MBC5:
          if (addr >= GB_ROM_BANK_SIZE) {
               /* Bank 0 can be remapped as bank 1 with this controller, so we
                * need to be careful to handle that case correctly */
               unsigned bank = cart->cur_rom_bank % cart->rom_banks;

               rom_off -= GB_ROM_BANK_SIZE;
               rom_off += bank * GB_ROM_BANK_SIZE;
          }
          break;
     default:
          /* Should not be reached */
          SpinLoop();
     }

     return GET_XIP_CHACHED_VALUE_8(cart->rom+rom_off);
     //return cart->rom[rom_off];
 }

void gb_cart_rom_writeb(struct gb *gb, uint16_t addr, uint8_t v) {
     struct gb_cart *cart = &gb->cart;

     switch (cart->model) {
     case GB_CART_SIMPLE:
          /* Nothing to be done */
          break;
     case GB_CART_MBC1:
          if (addr < 0x2000) {
               cart->ram_write_protected = ((v & 0xf) != 0xa);
          } else if (addr < 0x4000) {
               /* Set ROM bank, bits [4:0] */
               cart->cur_rom_bank &= ~0x1f;
               cart->cur_rom_bank |= v & 0x1f;
          } else if (addr < 0x6000) {
               /* Set RAM bank *or* ROM bank [6:5] depending on the mode */
               cart->cur_rom_bank &= 0x1f;
               cart->cur_rom_bank |= (v & 3) << 5;

               if (cart->ram_banks > 0) {
                    cart->cur_ram_bank = (v & 3) % cart->ram_banks;
               }
          } else {
               /* Change MBC1 banking mode */
               cart->mbc1_bank_ram = v & 1;
          }
          break;
     case GB_CART_MBC2:
          if (addr < 0x2000) {
               cart->ram_write_protected = ((v & 0xf) != 0xa);
          } else if (addr < 0x4000) {
               cart->cur_rom_bank = v & 0xf;
               if (cart->cur_rom_bank == 0) {
                    cart->cur_rom_bank = 1;
               }
          }
          break;
     case GB_CART_MBC3:
          if (addr < 0x2000) {
               cart->ram_write_protected = ((v & 0xf) != 0xa);
          } else if (addr < 0x4000) {
               /* Set ROM bank */
               cart->cur_rom_bank = (v & 0x7f) % cart->rom_banks;
               if (cart->cur_rom_bank == 0) {
                    cart->cur_rom_bank = 1;
               }
          } else if (addr < 0x6000) {
               /* Set RAM bank (v < 3) *or* RTC access */
               cart->cur_ram_bank = v;
          } else if (addr < 0x8000) {
               if (cart->has_rtc) {
                    gb_rtc_latch(gb, v == 1);
               }
          }
          break;
     case GB_CART_MBC5:
          if (addr < 0x2000) {
               cart->ram_write_protected = ((v & 0xf) != 0xa);
          } else if (addr < 0x3000) {
               /* Set ROM bank, low 8 bits */
               cart->cur_rom_bank &= 0x100;
               cart->cur_rom_bank |= v;
          } else if (addr < 0x4000) {
               /* Set ROM bank, MSB */
               cart->cur_rom_bank &= 0xff;
               cart->cur_rom_bank |= (v & 1) << 8;
          } else if (addr < 0x6000) {
               /* Set RAM bank */
               if (cart->ram_banks > 0) {
                    cart->cur_ram_bank = (v & 0xf) % cart->ram_banks;
               }
          }
          break;
     default:
          /* Should not be reached */
          SpinLoop();
     }
}

unsigned gb_cart_mbc1_ram_off(struct gb *gb, uint16_t addr) {
     struct gb_cart *cart = &gb->cart;
     unsigned bank;

     if (cart->ram_banks == 1) {
          /* Cartridges which only have one RAM bank can have only a
           * partial 2KB RAM chip that's mirrored 4 times. */
          return addr % cart->ram_length;
     }

     bank = cart->cur_ram_bank;

     if (cart->mbc1_bank_ram) {
          bank %= 4;
     } else {
          /* In this mode we only support one bank */
          bank = 0;
     }

     return bank * GB_RAM_BANK_SIZE + addr;
}

uint8_t gb_cart_ram_readb(struct gb *gb, uint16_t addr) {
     struct gb_cart *cart = &gb->cart;
     unsigned ram_off;

     switch (cart->model) {
     case GB_CART_SIMPLE:
          /* No RAM */
          return 0xff;
     case GB_CART_MBC1:
          if (cart->ram_banks == 0) {
               /* No RAM */
               return 0xff;
          }

          ram_off = gb_cart_mbc1_ram_off(gb, addr);
          break;
     case GB_CART_MBC2:
          ram_off = addr % 512;
          break;
     case GB_CART_MBC3:
          if (cart->cur_ram_bank <= 3) {
               /* RAM access */
               unsigned b;

               if (cart->ram_banks == 0) {
                    /* No RAM */
                    return 0xff;
               }

               b = cart->cur_ram_bank % cart->ram_banks;

               ram_off = b * GB_RAM_BANK_SIZE + addr;
          } else {
               /* RTC access. Only accessible when the RAM is not write
                * protected (even for reads) */
               if (cart->has_rtc && !cart->ram_write_protected) {
                    return gb_rtc_read(gb, cart->cur_ram_bank);
               } else {
                    return 0xff;
               }
          }

          break;
     case GB_CART_MBC5:
          if (cart->ram_banks == 0) {
               /* No RAM */
               return 0xff;
          }

          ram_off = cart->cur_ram_bank * GB_RAM_BANK_SIZE + addr;
          break;
     default:
          /* Should not be reached */
          SpinLoop();
          return 0xff;
     }

     return cart->ram[ram_off];
}

void gb_cart_ram_writeb(struct gb *gb, uint16_t addr, uint8_t v) {
     struct gb_cart *cart = &gb->cart;
     unsigned ram_off = 0;

     if (cart->ram_write_protected) {
          return;
     }

     switch (cart->model) {
     case GB_CART_SIMPLE:
          /* No RAM */
          return;
     case GB_CART_MBC1:
          if (cart->ram_banks == 0) {
               /* No RAM */
               return;
          }

          ram_off = gb_cart_mbc1_ram_off(gb, addr);
          break;
     case GB_CART_MBC2:
          ram_off = addr % 512;
          /* MBC2 only has 4 bits per address, so the high nibble is unusable */
          v |= 0xf0;
          break;
     case GB_CART_MBC3:
          if (cart->cur_ram_bank <= 3) {
               /* RAM access */
               unsigned b;

               if (cart->ram_banks == 0) {
                    /* No RAM */
                    return;
               }

               b = cart->cur_ram_bank % cart->ram_banks;

               ram_off = b * GB_RAM_BANK_SIZE + addr;
          } else {
               /* RTC access. Only accessible when the RAM is not write
                * protected (even for reads) */
               if (cart->has_rtc) {
                    gb_rtc_write(gb, cart->cur_ram_bank, v);
               }
               goto write_done;
          }

          break;
     case GB_CART_MBC5:
          if (cart->ram_banks == 0) {
               /* No RAM */
               return;
          }

          ram_off = cart->cur_ram_bank * GB_RAM_BANK_SIZE + addr;
          break;
     default:
          /* Should not be reached */
          SpinLoop();
     }

     if(ram_off > 180000){
          SpinLoop();
     }
     cart->ram[ram_off] = v;

write_done:
     if (cart->save_file) {
          cart->dirty_ram = true;
     }
}
