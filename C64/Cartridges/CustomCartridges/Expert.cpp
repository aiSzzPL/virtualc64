/*!
 * @file        Expert.cpp
 * @author      Dirk W. Hoffmann, www.dirkwhoffmann.de
 * @copyright   Dirk W. Hoffmann. All rights reserved.
 */
/*
 * This program is free software; you can redistribute it and/or modify
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

// This implementation is based mainly by the following documents:
// Schematics and explanation by Martin Sikström:
// https://people.kth.se/~e93_msi/c64/expert.html

#include "C64.h"

Expert::Expert(C64 *c64) : Cartridge(c64)
{
    setDescription("Expert");
    
    // Allocate 8KB bytes on-board RAM
    setRamCapacity(0x2000);
    
    active = false;
    
    debug("Expert cartridge created\n");
}

void
Expert::reset()
{
    Cartridge::reset();
    active = false;
    memset(externalRam, 0, ramCapacity);
}

size_t
Expert::stateSize()
{
    return Cartridge::stateSize() + 1;
}

void
Expert::loadFromBuffer(uint8_t **buffer)
{
    uint8_t *old = *buffer;
    Cartridge::loadFromBuffer(buffer);
    active = read8(buffer);
    if (*buffer - old != stateSize()) {
        assert(false);
    }
}

void
Expert::saveToBuffer(uint8_t **buffer)
{
    uint8_t *old = *buffer;
    Cartridge::saveToBuffer(buffer);
    write8(buffer, (uint8_t)active);
    if (*buffer - old != stateSize()) {
        assert(false);
    }
}

void
Expert::loadChip(unsigned nr, CRTFile *c)
{
    debug("nr = %d\n", nr);
    
    uint16_t chipSize = c->chipSize(nr);
    uint16_t chipAddr = c->chipAddr(nr);
    uint8_t *chipData = c->chipData(nr);
    
    if (nr != 0 || chipSize != 0x2000 || chipAddr != 0x8000) {
        warn("Corrupted CRT file. Aborting.");
        return;
    }

    // Initialize RAM with data from CRT file
    debug("Copying file contents into Expert RAM\n");
    assert(externalRam != NULL);
    assert(ramCapacity == chipSize);
    memcpy(externalRam, chipData, chipSize);
    
}

void
Expert::updatePeekPokeLookupTables()
{
    // Redirect all memory accesses to the cartridge
    for (int i = 1; i < 16; i++) {
        c64->mem.peekSrc[i] = c64->mem.pokeTarget[i] = M_CRTLO;
    }
}

uint8_t
Expert::peek(uint16_t addr)
{
    return 0;
}

uint8_t
Expert::peekIO1(uint16_t addr)
{
    assert(addr >= 0xDE00 && addr <= 0xDEFF);
    
    return 0;
}

uint8_t
Expert::peekIO2(uint16_t addr)
{
    assert(addr >= 0xDF00 && addr <= 0xDFFF);
    
    return 0;
}

void
Expert::poke(uint16_t addr, uint8_t value)
{
  
}

void
Expert::pokeIO1(uint16_t addr, uint8_t value)
{
    assert(addr >= 0xDE00 && addr <= 0xDEFF);
}

void
Expert::pokeIO2(uint16_t addr, uint8_t value)
{
 
}

void
Expert::setSwitch(int8_t pos)
{
    debug("Setting switch to %d\n", pos);

    Cartridge::setSwitch(pos);
}