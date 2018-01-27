/*!
 * @author      Dirk W. Hoffmann, www.dirkwhoffmann.de
 * @copyright   2018 Dirk W. Hoffmann
 */
/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "C64.h"

// -----------------------------------------------------------------------------------------
//                                Final Cartridge III
// -----------------------------------------------------------------------------------------

void
FinalIII::reset()
{
    debug("FinalCartridge::reset\n");
    
    /* Final cartridge III contains four 16KB ROMs residing at $8000 - $BFFF
     *
     *     Bank 0:  BASIC, Monitor, Disk-Turbo
     *     Bank 1:  Notepad, BASIC (Menu Bar)
     *     Bank 2:  Desktop, Freezer/Print
     *     Bank 3:  Freezer, Compression
     *
     * Final cartrige III switches frequently betwenn different exrom/game line
     * configurations. I.e., it uses ultimax mode to override the NMI vectors
     * stored in Kernel ROM. Switching between configurations causes ROMH sometimes
     * to be visible at $A000 and sometimes at $E000. As we do not want to copy memory
     * back and forth, we simply add four new chips at location $E000 which
     * reflect the upper half of the original four cartridge chips.
     */
    for (unsigned i = 0; i < 4; i++) {
        
        assert(chip[i] != NULL);
        chipStartAddress[i+4] = 0xE000;
        chipSize[i+4]         = 0x2000;
        chip[i+4]             = (uint8_t *)malloc(0x2000);
        memcpy(chip[i+4], chip[i] + 0x2000, 0x2000);
    }
    
    // c64->cpu.setNMILineExpansionPort();
    bankIn(0);
    initialGameLine = 0;
    initialExromLine = 0;
}

uint8_t
FinalIII::peekIO(uint16_t addr)
{
    assert(addr >= 0xDE00 && addr <= 0xDFFF);
    
    if (addr == 0xDFFF) {
        debug("Peeking from 0xDFFF\n");
    }
    
    // The I/O space mirrors $1E00 to $1EFF from the selected bank.
    uint16_t offset = addr - 0xDE00;
    return peek(0x8000 + 0x1E00 + offset);
}

void
FinalIII::poke(uint16_t addr, uint8_t value) {
    
    assert(addr >= 0xDE00 && addr <= 0xDFFF);
    
    // 0xDFFF is Final Cartridge's internal control register
    if (addr == 0xDFFF) {
        
        /*  "7      Hide this register (1 = hidden)
         *   6      NMI line   (0 = low = active) *1)
         *   5      GAME line  (0 = low = active) *2)
         *   4      EXROM line (0 = low = active)
         *   2-3    unassigned (usually set to 0)
         *   0-1    number of bank to show at $8000
         *
         *   1) if either the freezer button is pressed,
         *      or bit 6 is 0, then an NMI is generated
         *
         *   2) if the freezer button is pressed, GAME
         *      is also forced low" [VICE]
         */
        
        uint8_t hide  = value & 0x80;
        uint8_t nmi   = value & 0x40;
        uint8_t game  = value & 0x20;
        uint8_t exrom = value & 0x10;
        uint8_t bank  = value & 0x03;
        
        // debug("hide: %d nmi:%d game:%d exrom:%d bank:%d\n", hide != 0, nmi != 0, game != 0, exrom != 0, bank);
        
        // Bit 7
        if (hide) {
            c64->expansionport.setGameLine(1);
            c64->expansionport.setExromLine(1);
        }
        
        // Bit 6
        nmi ? c64->cpu.clearNMILineExpansionPort()
        : c64->cpu.setNMILineExpansionPort();
        
        // Bit 5 and 4
        c64->expansionport.setGameLine(game);
        c64->expansionport.setExromLine(exrom);
        
        // Bit 1 and 0
        bankIn(bank);
        bankIn(bank+4);
    }
}

void
FinalIII::pressFirstButton() {
    
    debug("Final cartridge III: Freeze Button\n");
    
    // The freezer is enabled by selecting bank 0 in unimax mode and triggering an NMI
    poke(0xDFFF, 0x10);
}

void
FinalIII::pressSecondButton() {
    
    uint8_t ram[0xFFFF];
    
    debug("Final cartridge III: Reset Button\n");
    
    // Note: Cartridge requires to keep RAM contants intact
    memcpy(ram, c64->mem.ram, 0xFFFF);
    c64->reset();
    memcpy(c64->mem.ram, ram, 0xFFFF);
}


// -----------------------------------------------------------------------------------------
//                                  Simons Basic
// -----------------------------------------------------------------------------------------

void
SimonsBasic::reset()
{
    bankIn(0);
    bankIn(1);
}

uint8_t
SimonsBasic::peekIO(uint16_t addr)
{
    assert(addr >= 0xDE00 && addr <= 0xDFFF);
    
    if (addr == 0xDE00) {
        // Switch to 8KB configuration
        c64->expansionport.setGameLine(1);
    }
    return Cartridge::peekIO(addr);
}

void
SimonsBasic::poke(uint16_t addr, uint8_t value)
{
    assert(addr >= 0xDE00 && addr <= 0xDFFF);
    
    if (addr == 0xDE00) {
        // Switch to 16KB configuration
        c64->expansionport.setGameLine(0);
    }
}

// -----------------------------------------------------------------------------------------
//                                   Ocean type 1
// -----------------------------------------------------------------------------------------

/*
 * For more information: http://codebase64.org/doku.php?id=base:crt_file_format
 *
 * "Memory is divided into 8Kb ($2000) banks. For the lower 128Kb, memory is banked
 *  into $8000-$9FFF and for the upper 128Kb, memory is banked into $A000-$BFFF.
 *  Note that the Terminator 2 cartridge loads all 64 banks at $8000-$9FFF.
 *
 * Bank switching is done by writing to $DE00. The lower six bits give the bank number
 * (ranging from 0-63). Bit 8 in this selection word is always set."
 */

void
OceanType1::poke(uint16_t addr, uint8_t value)
{
    assert(addr >= 0xDE00 && addr <= 0xDFFF);
    
    if (addr == 0xDE00) {
        bankIn(value & 0x3F);
    }
}


// -----------------------------------------------------------------------------------------
//                                Powerplay / Funplay
// -----------------------------------------------------------------------------------------

void
Powerplay::poke(uint16_t addr, uint8_t value)
{
    assert(addr >= 0xDE00 && addr <= 0xDFFF);
    
    /*
     * Bank switching is done by writing to $DE00.
     *
     * Bit in DE00 -> 76543210
     *                xx210xx3 <- Bit in selected bank number
     *
     * A value of $86 is written to disable the cartridge.
     */

    if (addr == 0xDE00) {
       
        if (value == 0x86) {
            c64->expansionport.setGameLine(1);
            c64->expansionport.setExromLine(1);
            return;
        }
        
        uint8_t bank = ((value >> 3) & 0x07) | ((value << 3) & 0x08);
        assert(bank < 16);
        bankIn(bank);
    }
}

// -----------------------------------------------------------------------------------------
//                                     Supergames
// -----------------------------------------------------------------------------------------

uint8_t
Supergames::peekIO(uint16_t addr)
{
    debug("Peeking %04X\n", addr);
    return 0;
}

void
Supergames::poke(uint16_t addr, uint8_t value)
{
    assert(addr >= 0xDE00 && addr <= 0xDFFF);
    
    /*
     * Bits 0 and 1: Bank bits 0 and 1
     * Bits 2:       Exrom / Game control
     * Bits 3:       when 1, further writes to DE00 have no effect (not implemented)
     */
    
    if (addr == 0xDF00) {
        
        uint8_t bank    = value & 0x03;
        uint8_t ctrl    = value & 0x04;
        // uint8_t protect = value & 0x08;
        
        // debug ("value = %02X, bank = %d, ctrl = %d", value, bank, ctrl);
        
        if (ctrl) {
            c64->expansionport.setExromLine(false);
            c64->expansionport.setGameLine(true);
        } else {
            c64->expansionport.setExromLine(0);
            c64->expansionport.setGameLine(0);
        }
        
        bankIn(bank);
    }
}

