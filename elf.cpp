#include <elf.h>
#include <fstream>
#include <vector>
#include <iostream>
#include <map>
#include <sstream>

#include "disass.hpp"

#include "dbgtypes.hpp"

#include "elftypes.hpp"
#include "elf.hpp"

//ELF::ELF(const char* filename): filename(filename){
ELF::ELF(std::string filename): filename(filename){
  content = NULL;
  //machine = -1;
  is_pie = false;
  base = 0;
    
  std::ifstream elf_file(filename, std::ios::binary | std::ios::ate);
    
  if (!elf_file.is_open()) {
    std::cout << "file does not exist" << std::endl;
    return;
  }
    

 
  elf_file.seekg(0, std::ios::end); 
  content_size = elf_file.tellg();
  elf_file.seekg(0, std::ios::beg);
   
  // rough check if to make sure file is large enough to be ELF 
  if (content_size < 16+3) {
    std::cout << "size to small" << std::endl;
    return;
  } 
 
  
  content = new char[content_size];
 
  if (!elf_file.read(content, content_size)) {
    std::cout << "something went wrong reading file" << std::endl;
    return;
  }
    

 
  // checking magic bytes 
  if (!(content[0] == 0x7f &&
        content[1] == 'E' &&
        content[2] == 'L' &&
        content[3] == 'F')) {
    std::cout << "File is not ELF" << std::endl;
    return;     
  }

  // get the machien architecture
  auto machine = static_cast<uint16_t>(content[16+2]);
  // read sections based on architecture
  switch (machine) {
    case EM_X86_64:
      //std::cout << "64 bit ELF" << std::endl; 
      architecture =  arch_t::ARCH_X86_64;
      read_sections_x86_64();
      break;
    case EM_386:
      //std::cout << "32 bit ELF" << std::endl; 
      architecture =  arch_t::ARCH_X86_32;
      read_sections_i386();
      break;
    default:
      architecture =  arch_t::ARCH_UNDEF;
      std::cout << "Format not supported";
      return;
  }

  //elf_file.close();
  
}

ELF::~ELF() {
  //std::cout << "call destructor for; " << filename << std::endl;
  if (content != NULL) {
    delete[] content;
  }
}

////////////////////
// RESDING SYMTABLE
////////////////////
template<typename T>
void ELF::read_symtab(uint32_t sh_offset, uint32_t sh_size, uint32_t strtab_offset) {
  auto sym_table = reinterpret_cast<T*>(&content[sh_offset]);
  auto sym_count = sh_size/sizeof(T);

  for (uint32_t i = 0; i < sym_count; i++ ) {
    auto name = static_cast<std::string>(&(content[strtab_offset + sym_table[i].st_name]));
    auto addr = static_cast<uint64_t>(sym_table[i].st_value);

    // find correscponding section so we can compute the offset
    auto sect = sections.begin();
    while (sect->second.contains(sym_table[i].st_value) && sect != sections.end())
      ++sect;

    uint32_t offset = (addr - sect->second.get_start()) + sect->second.get_offset(); 

    //std::cout << std::hex << offset << "   " << name << std::endl;

    symtab.emplace(std::pair(name, Symbol(addr, offset, static_cast<uint32_t>(sym_table[i].st_size), name)));
  }
}


//void ELF::read_symtab_i386(uint32_t strtab_offset, Elf32_Shdr *shdr) {
void ELF::read_symtab_i386(uint32_t sh_offset, uint32_t sh_size, uint32_t strtab_offset) {
  read_symtab<Elf32_Sym>(sh_offset, sh_size, strtab_offset);
} 

void ELF::read_symtab_x86_64(uint32_t sh_offset, uint32_t sh_size, uint32_t strtab_offset) {
  read_symtab<Elf64_Sym>(sh_offset, sh_size, strtab_offset);
}


/////////////////
// READ SECTONS 
/////////////////
template<typename Elf_hd, typename Elf_Sh>
void ELF::get_sections() {
  auto elf_header = reinterpret_cast<Elf_hd*>(content);

  if (elf_header->e_type == ET_DYN) {
    is_pie = true;
  }


  // optional to get name of section
  auto shdr_string = reinterpret_cast<Elf_Sh*>(&content[elf_header->e_shoff + (elf_header->e_shstrndx * elf_header->e_shentsize)]);

  for (int i = 0; i < elf_header->e_shnum; i++) {
    // compute offset for section table entry in file
    uint64_t sht_e_offset = elf_header->e_shoff + (i*elf_header->e_shentsize);

    // read section table 
    auto *shdr = reinterpret_cast<Elf_Sh*>(&content[sht_e_offset]);

    auto name = static_cast<std::string>((&content[shdr_string->sh_offset + shdr->sh_name]));

    sections.emplace(std::pair(name, Section(shdr, name)));
  }

}

void ELF::read_sections_x86_64() {
  get_sections<Elf64_Ehdr, Elf64_Shdr>();

  auto symtab_sct = sections.find(".symtab");
  auto dynsym_sct = sections.find(".dynsym");
  auto strtab_sct = sections.find(".strtab");
  auto dynstr_sct = sections.find(".dynstr");

  if (symtab_sct != sections.end() && strtab_sct != sections.end()) {
    read_symtab_x86_64(symtab_sct->second.get_offset(), symtab_sct->second.get_size(), strtab_sct->second.get_offset());
  } else if (dynsym_sct != sections.end() && dynstr_sct != sections.end()) {
    read_symtab_x86_64(dynsym_sct->second.get_offset(), dynsym_sct->second.get_size(), dynstr_sct->second.get_offset());
  }
}


void ELF::read_sections_i386() {
  get_sections<Elf32_Ehdr, Elf32_Shdr>();

  auto symtab_sct = sections.find(".symtab");
  auto dynsym_sct = sections.find(".dynsym");
  auto strtab_sct = sections.find(".strtab");
  auto dynstr_sct = sections.find(".dynstr");

  if (symtab_sct != sections.end() && strtab_sct != sections.end()) {
    read_symtab_i386(symtab_sct->second.get_offset(), symtab_sct->second.get_size(), strtab_sct->second.get_offset());
  } else if (dynsym_sct != sections.end() && dynstr_sct != sections.end()) {
    read_symtab_i386(dynsym_sct->second.get_offset(), dynsym_sct->second.get_size(), dynstr_sct->second.get_offset());
  }
}
 
///////////////////////
// Interactive Functions
// ///////////////////

void ELF::rebase(uint64_t base_addr) {
  base = base_addr;
  if (is_pie) {
    for (auto& sect : sections) {
      sect.second.rebase(base_addr);
    }
    for (auto& sym : symtab) {
      sym.second.rebase(base_addr);
    }
  }
}
  
arch_t ELF::get_machine() const {
  return architecture;
}
  
std::string ELF::get_filename() const {
  return filename;
}

bool ELF::pie() const {
  return is_pie;
}


//////////////////////
// READ FROM MEM
/////////////////////

char ELF::get_byte_at_offset(uint32_t offset) const{
  if (offset < content_size) {
    return content[offset];
  } 
  return '\x00';
}

char ELF::get_byte_at_addr(uint64_t addr) const {
  return get_byte_at_offset(addr - base);
}

uint8_t *ELF::get_n_bytes_at_addr(uint64_t addr, uint32_t n) const {
  uint8_t *bytes = new uint8_t[n];
  
  for (uint64_t i = addr; i < addr + n; i++) {
    bytes[i-addr] = static_cast<uint8_t>(get_byte_at_addr(i));
  }

  return bytes;
}

uint8_t *ELF::get_n_bytes_at_offset(uint64_t offset, uint32_t n) const {
  uint8_t *bytes = new uint8_t[n];
  
  for (uint64_t i = offset; i < offset + n; i++) {
    bytes[i-offset] = static_cast<uint8_t>(get_byte_at_offset(offset));
  }

  return bytes;
}

///////////////////////
// SYMTAB functions
//////////////////////

uint32_t ELF::get_symbol_offset(std::string symbol) const {
  auto it = symtab.find(symbol);
  
  if (it == symtab.end()) {
    return 0;
  }
  return it->second.get_offset();
}


uint64_t ELF::get_symbol_addr(std::string symbol) const {
  auto it = symtab.find(symbol);
  
  if (it == symtab.end()) {
    return 0;
  }

  return it->second.get_addr();
}

uint32_t ELF::get_symbol_size(std::string symbol) const {
  auto it = symtab.find(symbol);

  if (it == symtab.end()) {
    return 0;
  }

  return it->second.get_size();
}

////////////////////
// DEBUG FUNCTIONS
///////////////////
void ELF::print_filename() const {
  std::cout << filename << std::endl;
}

void ELF::print_sections() const {
  for (auto& s: sections) {
    std::cout << s.second.str(architecture) << std::endl;
  }
}
   
void ELF::print_symtab() const {
  for (auto& sym_entry: symtab) {
    std::cout << sym_entry.second.str(architecture) << std::endl;
  }
}
