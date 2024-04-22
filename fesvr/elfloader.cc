// See LICENSE for license details.

#include "swap_mem.h"
#include "config.h"
#include "elf.h"
#include "memif.h"
#include "byteorder.h"
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <assert.h>
#include <unistd.h>
#include <stdexcept>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <map>
#include <cerrno>
#include <fstream>

std::map<std::string, uint64_t> load_elf(const char* fn, memif_t* memif, reg_t* entry, unsigned required_xlen = 0)
{
  int fd = open(fn, O_RDONLY);
  struct stat s;
  if (fd == -1)
      throw std::invalid_argument(std::string("Specified ELF can't be opened: ") + strerror(errno));
  if (fstat(fd, &s) < 0)
    abort();
  size_t size = s.st_size;

  char* buf = (char*)mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (buf == MAP_FAILED)
      throw std::invalid_argument(std::string("Specified ELF can't be mapped: ") + strerror(errno));
  close(fd);

  assert(size >= sizeof(Elf64_Ehdr));
  const Elf64_Ehdr* eh64 = (const Elf64_Ehdr*)buf;
  assert(IS_ELF32(*eh64) || IS_ELF64(*eh64));
  unsigned xlen = IS_ELF32(*eh64) ? 32 : 64;
  if (required_xlen != 0 && required_xlen != xlen) {
    throw incompat_xlen(required_xlen, xlen);
  }
  assert(IS_ELFLE(*eh64) || IS_ELFBE(*eh64));
  assert(IS_ELF_EXEC(*eh64));
  assert(IS_ELF_RISCV(*eh64) || IS_ELF_EM_NONE(*eh64));
  assert(IS_ELF_VCURRENT(*eh64));

  std::vector<uint8_t> zeros;
  std::map<std::string, uint64_t> symbols;

#define LOAD_ELF(ehdr_t, phdr_t, shdr_t, sym_t, bswap)                         \
  do {                                                                         \
    ehdr_t* eh = (ehdr_t*)buf;                                                 \
    phdr_t* ph = (phdr_t*)(buf + bswap(eh->e_phoff));                          \
    *entry = bswap(eh->e_entry);                                               \
    assert(size >= bswap(eh->e_phoff) + bswap(eh->e_phnum) * sizeof(*ph));     \
    for (unsigned i = 0; i < bswap(eh->e_phnum); i++) {                        \
      if (bswap(ph[i].p_type) == PT_LOAD && bswap(ph[i].p_memsz)) {            \
        if (bswap(ph[i].p_filesz)) {                                           \
          assert(size >= bswap(ph[i].p_offset) + bswap(ph[i].p_filesz));       \
          memif->write(bswap(ph[i].p_paddr), bswap(ph[i].p_filesz),            \
                       (uint8_t*)buf + bswap(ph[i].p_offset));                 \
        }                                                                      \
        if (size_t pad = bswap(ph[i].p_memsz) - bswap(ph[i].p_filesz)) {       \
          zeros.resize(pad);                                                   \
          memif->write(bswap(ph[i].p_paddr) + bswap(ph[i].p_filesz), pad,      \
                       zeros.data());                                          \
        }                                                                      \
      }                                                                        \
    }                                                                          \
    shdr_t* sh = (shdr_t*)(buf + bswap(eh->e_shoff));                          \
    assert(size >= bswap(eh->e_shoff) + bswap(eh->e_shnum) * sizeof(*sh));     \
    assert(bswap(eh->e_shstrndx) < bswap(eh->e_shnum));                        \
    assert(size >= bswap(sh[bswap(eh->e_shstrndx)].sh_offset) +                \
                       bswap(sh[bswap(eh->e_shstrndx)].sh_size));              \
    char* shstrtab = buf + bswap(sh[bswap(eh->e_shstrndx)].sh_offset);         \
    unsigned strtabidx = 0, symtabidx = 0;                                     \
    for (unsigned i = 0; i < bswap(eh->e_shnum); i++) {                        \
      unsigned max_len =                                                       \
          bswap(sh[bswap(eh->e_shstrndx)].sh_size) - bswap(sh[i].sh_name);     \
      assert(bswap(sh[i].sh_name) < bswap(sh[bswap(eh->e_shstrndx)].sh_size)); \
      assert(strnlen(shstrtab + bswap(sh[i].sh_name), max_len) < max_len);     \
      if (bswap(sh[i].sh_type) & SHT_NOBITS) continue;                         \
      assert(size >= bswap(sh[i].sh_offset) + bswap(sh[i].sh_size));           \
      if (strcmp(shstrtab + bswap(sh[i].sh_name), ".strtab") == 0)             \
        strtabidx = i;                                                         \
      if (strcmp(shstrtab + bswap(sh[i].sh_name), ".symtab") == 0)             \
        symtabidx = i;                                                         \
    }                                                                          \
    if (strtabidx && symtabidx) {                                              \
      char* strtab = buf + bswap(sh[strtabidx].sh_offset);                     \
      sym_t* sym = (sym_t*)(buf + bswap(sh[symtabidx].sh_offset));             \
      for (unsigned i = 0; i < bswap(sh[symtabidx].sh_size) / sizeof(sym_t);   \
           i++) {                                                              \
        unsigned max_len =                                                     \
            bswap(sh[strtabidx].sh_size) - bswap(sym[i].st_name);              \
        assert(bswap(sym[i].st_name) < bswap(sh[strtabidx].sh_size));          \
        assert(strnlen(strtab + bswap(sym[i].st_name), max_len) < max_len);    \
        symbols[strtab + bswap(sym[i].st_name)] = bswap(sym[i].st_value);      \
      }                                                                        \
    }                                                                          \
  } while (0)

  if (IS_ELFLE(*eh64)) {
    if (memif->get_target_endianness() != endianness_little) {
      throw std::invalid_argument("Specified ELF is little endian, but system uses a big-endian memory system. Rerun without --big-endian");
    }
    if (IS_ELF32(*eh64))
      LOAD_ELF(Elf32_Ehdr, Elf32_Phdr, Elf32_Shdr, Elf32_Sym, from_le);
    else
      LOAD_ELF(Elf64_Ehdr, Elf64_Phdr, Elf64_Shdr, Elf64_Sym, from_le);
  } else {
#ifndef RISCV_ENABLE_DUAL_ENDIAN
    throw std::invalid_argument("Specified ELF is big endian.  Configure with --enable-dual-endian to enable support");
#else
    if (memif->get_target_endianness() != endianness_big) {
      throw std::invalid_argument("Specified ELF is big endian, but system uses a little-endian memory system. Rerun with --big-endian");
    }
    if (IS_ELF32(*eh64))
      LOAD_ELF(Elf32_Ehdr, Elf32_Phdr, Elf32_Shdr, Elf32_Sym, from_be);
    else
      LOAD_ELF(Elf64_Ehdr, Elf64_Phdr, Elf64_Shdr, Elf64_Sym, from_be);
#endif
  }

  munmap(buf, size);

  return symbols;
}

std::map<std::string, uint64_t> load_dist(const char* fn, memif_t* memif, reg_t* entry, unsigned required_xlen = 0){
  std::ifstream bin_dist(fn);

  uint64_t mem_begin, mem_end;
  bin_dist >> std::hex >> mem_begin >> std::hex >> mem_end;
  *entry = (reg_t)mem_begin;

  uint64_t block_begin;
  size_t block_len;
  std::string block_kind;
  std::string init_file_name;
  while(bin_dist >> std::hex >> block_begin >> std::hex >> block_len >> block_kind >> init_file_name){
    std::ifstream init_file(init_file_name);
    init_file.seekg(0, std::ios_base::end);
    size_t file_size = init_file.tellg();
    init_file.seekg(0, std::ios_base::beg);
    if(file_size>block_len){
      throw std::invalid_argument(std::string("the binay file length is out of block bound"));
    }
    char* file_buffer = new char[file_size];
    init_file.read(file_buffer, file_size);

    if(block_kind == "keep"){
      memif->write(block_begin, file_size, file_buffer);
      delete [] file_buffer;
    }else if(block_kind == "swap"){
      int swap_idx;
      bin_dist >> swap_idx;
      swap_mem.register_swap_mem(block_begin, file_size, file_buffer, swap_idx);
    }else{
      throw std::invalid_argument(std::string("the block kind is invalid"));
      delete [] file_buffer;
    }
  }

  return std::map<std::string, uint64_t>();
}
