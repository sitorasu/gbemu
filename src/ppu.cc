#include "ppu.h"

#include <cstdint>
#include <vector>

using namespace gbemu;

std::uint8_t Ppu::ReadVRam8(std::uint16_t address) {
  return vram_.at(GetVRamAddressOffset(address));
}

void Ppu::WriteVRam8(std::uint16_t address, std::uint8_t value) {
  vram_.at(GetVRamAddressOffset(address)) = value;
}

std::uint8_t Ppu::ReadOam8(std::uint16_t address) {
  return oam_.at(GetOamAddressOffset(address));
}

void Ppu::WriteOam8(std::uint16_t address, std::uint8_t value) {
  oam_.at(GetOamAddressOffset(address)) = value;
}
