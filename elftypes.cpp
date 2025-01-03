#include <string.h>
#include <cstdint>
#include <iostream>
#include <sstream>
#include "dbgtypes.hpp"
#include "elftypes.hpp"
#include "fmt.hpp"

Section::Section(uint64_t start_addr, uint32_t offset, size_t size, std::string name = "None") 
   : start_addr(start_addr), offset(offset), size(size), name(name) {}

Section::Section(Elf64_Shdr *shdr, std::string name) 
  : start_addr(shdr->sh_addr), offset(shdr->sh_offset), size(shdr->sh_size), name(name) {}

Section::Section(Elf32_Shdr *shdr, std::string name) 
  : start_addr(shdr->sh_addr), offset(shdr->sh_offset), size(shdr->sh_size), name(name) {}


void Section::rebase(uint64_t base_addr) {
  start_addr = offset + base_addr;
}
uint64_t Section::get_offset() const {
  return offset;
}
  
uint64_t Section::get_start() const {
  return start_addr;
}

uint64_t Section::get_size() const {
  return size;
}

std::string Section::str(arch_t architecture) const {
  /*
    std::cout << "0x" << std::hex << start_addr << "   " 
             << "0x" << std::hex << offset << "   " 
             << "0x" << std::hex << size << "   "  <<  name << std::endl;*/
  std::stringstream ss;
  if (architecture == arch_t::ARCH_X86_64) {
    ss << fmt::yellow << fmt::addr_64(start_addr) << fmt::endc << "  " 
       << fmt::fleft(6) << std::hex << offset << " "
       << fmt::fleft(6) << std::hex << fmt::red << fmt::fleft(6) << size  << fmt::blue << name << fmt::endc; 

  } else if (architecture == arch_t::ARCH_X86_32) {
    ss << fmt::yellow << fmt::addr_32(start_addr) << fmt::endc << "  " 
       << fmt::fleft(6) << std::hex << offset << " "
       << fmt::fleft(6) << std::hex << fmt::red << fmt::fleft(6) << size << fmt::blue << name << fmt::endc; 

  } else {
    if (start_addr > 0xffffffff) {
      ss << fmt::yellow << fmt::addr_64(start_addr) << fmt::endc << "  " 
         << fmt::fleft(6) << std::hex << offset << " "
         << fmt::fleft(6) << std::hex << fmt::red << fmt::fleft(6) << size  << fmt::blue << name << fmt::endc; 

    } else {
      ss << fmt::yellow << fmt::addr_32(start_addr) << fmt::endc << "  " 
         << fmt::fleft(6) << std::hex << offset << " "
         << fmt::fleft(6) << std::hex << fmt::red << fmt::fleft(6) << size << fmt::blue << name << fmt::endc; 

    }
  }

  return ss.str();
}

  // checks if addr is contrained within the section
bool Section::contains(uint64_t addr) const {
  if (addr >= start_addr && addr < start_addr + size) {
    return true;
  } 
  return false;
}

// TODO: check offset addr (something is not working
Symbol::Symbol(uint64_t addr, uint32_t offset, uint32_t size, std::string name) : addr(addr), offset(addr), size(size), name(name) {}

void Symbol::rebase(uint64_t base_addr) {
  addr = base_addr + offset;
}


uint64_t Symbol::get_addr() const {
  return addr;
}

uint32_t Symbol::get_offset() const {
  return offset;
}
  
uint32_t Symbol::get_size() const {
  return size;
}

std::string Symbol::str(arch_t architecture) const {
  //std::cout << "0x" << std::hex << addr << "  " << std::dec << size << "  " << name << std::endl; 
  std::stringstream ss;
  if (architecture == arch_t::ARCH_X86_64) {
    ss << fmt::yellow << fmt::addr_64(addr) << fmt::endc << "  " << fmt::fleft(7) << std::dec << size << fmt::cyan << name << fmt::endc; 
  } else if (architecture == arch_t::ARCH_X86_32) {
    ss << fmt::yellow << fmt::addr_32(addr) << fmt::endc << "  " << fmt::fleft(7) << std::dec << size << fmt::cyan << name << fmt::endc; 
  } else {
    if (addr > 0xffffffff) {
      ss << fmt::yellow << fmt::addr_64(addr) << fmt::endc << "  " << fmt::fleft(7) << std::dec << size << fmt::cyan << name << fmt::endc; 
    } else {
      ss << fmt::yellow << fmt::addr_32(addr) << fmt::endc << "  " << fmt::fleft(7) << std::dec << size << fmt::cyan << name << fmt::endc; 
    }
  }
  return ss.str();
}
