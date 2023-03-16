#ifndef _ZJV_MAGIC_DEVICE_H
#define _ZJV_MAGIC_DEVICE_H

#include "cj.h"

#define MAGIC_RANDOM        0x00
#define MAGIC_RDM_WORD      0x08
#define MAGIC_RDM_FLOAT     0x010
#define MAGIC_RDM_DOUBLE    0x018
#define MAGIC_RDM_TEXT_ADDR 0x020
#define MAGIC_RDM_DATA_ADDR 0x028
#define MAGIC_MEPC_NEXT     0x030
#define MAGIC_SEPC_NEXT     0x038
#define MAGIC_RDM_PTE       0x040
#define MAX_MAGIC_SPACE     0x048

class magic_t {
 public:
  magic_t(): seed(0), rand2(0, 1) {
    generator[MAGIC_RANDOM/8]         = std::bind(&magic_t::rdm_dword, this, 64, 0);
    generator[MAGIC_RDM_WORD/8]       = std::bind(&magic_t::rdm_dword, this, 32, 1);
    generator[MAGIC_RDM_FLOAT/8]      = std::bind(&magic_t::rdm_float, this, -1, -1, 23, 31);
    generator[MAGIC_RDM_DOUBLE/8]     = std::bind(&magic_t::rdm_float, this, -1, -1, 52, 63);
    generator[MAGIC_RDM_TEXT_ADDR/8]  = std::bind(&magic_t::rdm_address, this, 1, 1, 1);
    generator[MAGIC_RDM_DATA_ADDR/8]  = std::bind(&magic_t::rdm_address, this, 1, 1, 0);
    generator[MAGIC_MEPC_NEXT/8]      = std::bind(&magic_t::rdm_epc_next, this, 0);
    generator[MAGIC_SEPC_NEXT/8]      = std::bind(&magic_t::rdm_epc_next, this, 1);
    generator[MAGIC_RDM_PTE/8]        = std::bind(&magic_t::rdm_dword, this, 10, 0);
  }
  
  reg_t load(reg_t addr) {
    reg_t id = addr / 8;
    reg_t tmp = id < generator.size() ? generator[id]() : 0;
    // printf("[CJ] Magic read: %016lx -> %016lx\n", addr, tmp);
    return tmp;
  }
  
  size_t size() { return 4096; }
  void set_seed(reg_t new_seed) { seed = new_seed; random.seed(seed); }

  reg_t rdm_dword(int width, int sgned);                    // 1 for signed, 0 for unsigned, -1 for random
  reg_t rdm_float(int type, int sgn, int botE, int botS);   // 0 for 0, 1 for INF, 2 for qNAN, 3 for sNAN, 4 for normal, 5 for tiny, -1 for random
  reg_t rdm_address(int r, int w, int x);                   // 1 for yes, 0 for no, -1 for random
  reg_t rdm_epc_next(int smode);                            // 1 for sepc, 0 for mepc
  reg_t rdm_dummy() { return 0; }

 private:
  reg_t seed;

  std::default_random_engine random;
  std::uniform_int_distribution<reg_t> rand2;
  std::array<std::function<reg_t()>, MAX_MAGIC_SPACE/8> generator;
};

#endif