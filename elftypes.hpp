#pragma once
#include <string>
#include <elf.h>

class Section {
private:
  uint64_t start_addr;
  uint64_t offset;
  uint64_t size;
  const std::string name;

public:
  Section(uint64_t start_addr, uint32_t offset, size_t size, std::string name);

  Section(Elf64_Shdr *shdr, std::string name);

  Section(Elf32_Shdr *shdr, std::string name);

  void rebase(uint64_t base_addr);

  uint64_t get_offset();
  
  uint64_t get_start();

  uint64_t get_size();

  void print_section();

  // checks if addr is contrained within the section
  bool contains(uint64_t addr);
};


class Symbol {
private:
  uint64_t addr;
  uint32_t offset;
  uint32_t size;
  std::string name;

public:
  Symbol(uint64_t addr, uint32_t offset, uint32_t size, std::string name);
 
  std::string str();
  
  void rebase(uint64_t base_addr);

  uint64_t get_addr();
 
  uint32_t get_offset();

  uint32_t get_size();

  void print_symbol();
};