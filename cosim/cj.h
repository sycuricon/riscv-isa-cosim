#ifndef _COSIM_CJ_H
#define _COSIM_CJ_H
#include "cfg.h"
#include "mmu.h"
#include "simif.h"
#include "memif.h"
#include "config.h"
#include "masker.h"
#include "devices.h"
#include "platform.h"
#include "processor.h"
#include "magic_device.h"

#include <set>
#include <array>
#include <queue>
#include <random>
#include <functional>


// Using json to configure in future
class config_t : public cfg_t {
public:
  config_t()
    : cfg_t(
      std::make_pair((reg_t)0, (reg_t)0),
      nullptr, 1, DEFAULT_ISA, DEFAULT_PRIV),
      varch(DEFAULT_VARCH), logfile(stderr), dtsfile(NULL), elffile(NULL),
      mem_base(DRAM_BASE), mem_size(2048),
      cycle_freq(1000000000), time_freq_count(100),
      start_pc(-1), verbose(false), va_mask(0xffffffffffe00000UL)
      {}

  cfg_arg_t<const char *> varch;
  cfg_arg_t<FILE *> logfile;
  cfg_arg_t<const char *> dtsfile;
  cfg_arg_t<const char *> elffile;
  cfg_arg_t<reg_t> mem_base;
  cfg_arg_t<reg_t> mem_size;
  cfg_arg_t<size_t> cycle_freq;
  cfg_arg_t<size_t> time_freq_count;
  cfg_arg_t<reg_t> start_pc;
  cfg_arg_t<bool> verbose;
  cfg_arg_t<reg_t> va_mask;
};

inline bool operator==(const float128_t& lhs, const float128_t& rhs) {
  return (lhs.v[0] == rhs.v[0]) && (lhs.v[1] == rhs.v[1]);
}

inline bool operator!=(const float128_t& lhs, const float128_t& rhs) {
  return (lhs.v[0] != rhs.v[0]) || (lhs.v[1] != rhs.v[1]);
}


template <class T, size_t N, bool ignore_zero>
class checkboard_t {
public:
  bool set(size_t i, T value, uint32_t dut_insn, reg_t dut_pc, bool mmio_ignore = false) {
    if (!ignore_zero || i != 0) {
      if (valid[i])
        return false;
      valid[i] = !valid[i];
      ignore[i] = mmio_ignore;
      data[i] = value;
      insn[i] = dut_insn;
      pc[i] = dut_pc;
    }
    return true;
  }
  bool clear(size_t i, T value) {
    if (!ignore_zero || i != 0) {
      if (!valid[i])
        return false;
      if (data[i] != value) {
        return false;
      }
      valid[i] = !valid[i];
    }
    return true;
  }
  void clear(size_t i) {
    valid[i] = false;
  }

  checkboard_t() { reset(); }
  void reset() { 
    std::fill(std::begin(valid), std::end(valid), false);
    std::fill(std::begin(ignore), std::end(ignore), false);
  }
  uint32_t get_insn(size_t i) { return insn[i]; }
  reg_t get_pc(size_t i) { return pc[i]; }
  T get_data(size_t i) { return data[i]; }
  bool get_ignore(size_t i) { return ignore[i]; }

private:
  T data[N];
  uint32_t insn[N];
  reg_t pc[N];
  bool valid[N];
  bool ignore[N];
};

class magic_t;
class dummy_device_t;

class cosim_cj_t : simif_t, chunked_memif_t {
public:
  cosim_cj_t(config_t& cfg);
  ~cosim_cj_t();

  void load_testcase(const char* elffile);

  // dpi
  int cosim_commit_stage(int hartid, reg_t dut_pc, uint32_t dut_insn, bool check);
  int cosim_judge_stage(int hartid, int dut_waddr, reg_t dut_wdata, bool fc);
  void cosim_raise_trap(int hartid, reg_t cause);
  uint64_t cosim_randomizer_insn(uint64_t in, uint64_t pc);
  uint64_t cosim_randomizer_data(unsigned int read_select);
  uint64_t get_random_text_address(std::default_random_engine &random);
  uint64_t get_random_data_address(std::default_random_engine &random);
  uint64_t get_exception_return_address(std::default_random_engine &random);

  // simif_t virtual function
  char* addr_to_mem(reg_t addr);
  bool mmio_load(reg_t addr, size_t len, uint8_t* bytes);
  bool mmio_store(reg_t addr, size_t len, const uint8_t* bytes);
  void proc_reset(unsigned id);
  const char* get_symbol(uint64_t addr);

  bool in_fuzz_loop_range(uint64_t addr) {
    if (va_enable) {
      return 0x1000 <= addr && addr < (fuzz_loop_exit_addr - fuzz_loop_entry_addr + 0x1000);
    } else {
      return fuzz_loop_entry_addr <= addr && addr < fuzz_loop_exit_addr;
    }
  }

  bool in_fuzz_handler_range(uint64_t addr) {
    if (va_enable) {
      return (fuzz_handler_start_addr <= addr && addr < fuzz_handler_end_addr) || 
             ((fuzz_handler_start_addr | va_mask) <= addr && addr < (fuzz_handler_end_addr | va_mask)) ;
    } else {
      return fuzz_handler_start_addr <= addr && addr < fuzz_handler_end_addr;
    }
  }

  // chunked_memif_t virtual function
  void read_chunk(addr_t taddr, size_t len, void* dst);
  void write_chunk(addr_t taddr, size_t len, const void* src);
  void clear_chunk(addr_t taddr, size_t len);
  size_t chunk_align() { return 8; }
  size_t chunk_max_size() { return 8; }
  void set_target_endianness(memif_endianness_t endianness);
  memif_endianness_t get_target_endianness() const;

  processor_t* get_core(size_t i) { return procs.at(i); }
  reg_t get_tohost() { return tohost_data; };
  void set_tohost(reg_t value) { tohost_data = value; };
  bool get_mmio_access() { return mmio_access; }
  void clear_mmio_access() { mmio_access = false; }

  checkboard_t<reg_t, NXPR, true> check_board;
  checkboard_t<freg_t, NFPR, false> f_check_board;

  void update_tohost_info();

  //debug
  void record_rd_mutation_stats(unsigned int matched_reg_count);

private:
  std::vector<std::pair<reg_t, mem_t*>> mems;
  mmu_t* debug_mmu;
  std::vector<processor_t*> procs;
  std::vector<dummy_device_t*> mmios;
  reg_t start_pc;
  reg_t elf_entry;
  std::string dts;
  std::string dtb;
  std::string dtb_file;
  std::unique_ptr<rom_device_t> boot_rom;
  std::unique_ptr<clint_t> clint;
  std::unique_ptr<magic_t> magic;
  bus_t bus;

  std::vector<unsigned int> matched_reg_count_stat;
  
  bool blind;
  bool cj_debug;
  bool mmio_access;
  addr_t tohost_addr;
  addr_t fuzz_loop_entry_addr;
  addr_t fuzz_loop_exit_addr;
  size_t fuzz_loop_page_num;
  addr_t fuzz_handler_start_addr;
  addr_t fuzz_handler_end_addr;
  size_t fuzz_handler_page_num;
  reg_t tohost_data;

  std::map<uint64_t, std::string> addr2symbol;
  std::vector<uint64_t> text_label;
  std::vector<uint64_t> data_label;

  void reset();
  void idle();

  bool start_randomize;
  reg_t va_mask;
  bool va_enable;
};

class dummy_device_t : public abstract_device_t {
 public:
  dummy_device_t(size_t addr, size_t size) : mmio_addr(addr), mmio_size(size) {}
  bool load(reg_t addr, size_t len, uint8_t* bytes) { return (addr + len) < mmio_size; }
  bool store(reg_t addr, size_t len, const uint8_t* bytes) { return (addr + len) < mmio_size; }
  size_t size() { return mmio_size; }
  size_t addr() { return mmio_addr; }
private:
  size_t mmio_addr;
  size_t mmio_size;
};

extern const std::vector<magic_type*> magic_generator_type;

class magic_t {
 public:
  magic_t(): seed(0), rand2(0, 1) {
    generator[MAGIC_RANDOM/8]         = std::bind(&magic_t::rdm_dword, this, 64, 0);
    generator[MAGIC_RDM_WORD/8]       = std::bind(&magic_t::rdm_dword, this, 32, 1);
    generator[MAGIC_RDM_FLOAT/8]      = std::bind(&magic_t::rdm_float, this, -1, -1, 23, 31);
    generator[MAGIC_RDM_DOUBLE/8]     = std::bind(&magic_t::rdm_float, this, -1, -1, 52, 63);
    generator[MAGIC_RDM_TEXT_ADDR/8]  = std::bind(&magic_t::rdm_address, this, 1, 1, 1);
    generator[MAGIC_RDM_DATA_ADDR/8]  = std::bind(&magic_t::rdm_address, this, 1, 1, 0);
    generator[MAGIC_EPC_NEXT/8]       = std::bind(&magic_t::rdm_exp_ret, this);
    generator[MAGIC_EPC_MAP/8]        = std::bind(&magic_t::rdm_dummy, this);
    generator[MAGIC_RDM_PTE/8]        = std::bind(&magic_t::rdm_dummy, this);
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
  reg_t rdm_exp_ret();
  reg_t rdm_dummy() { return 0; }

 private:
  reg_t seed;

  std::default_random_engine random;
  std::uniform_int_distribution<reg_t> rand2;
  std::array<std::function<reg_t()>, MAX_MAGIC_SPACE/8> generator;
};

extern cosim_cj_t* simulator;
#endif
