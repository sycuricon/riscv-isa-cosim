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
      start_pc(-1), verbose(false)
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
  bool set(size_t i, T value, uint32_t dut_insn) {
    if (!ignore_zero || i != 0) {
      if (valid[i])
        return false;
      valid[i] = !valid[i];
      data[i] = value;
      insn[i] = dut_insn;
    }
    return true;
  }
  bool clear(size_t i, T value) {
    if (!ignore_zero || i != 0) {
      if (!valid[i])
        return false;
      if (data[i] != value) {
        printf("\x1b[31m[error] WDATA \x1b[33mSIM %016lx\x1b[31m, DUT \x1b[36m%016lx \x1b[0m\n", 
          dump(data[i]), dump(value));
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
  void reset() { std::fill(std::begin(valid), std::end(valid), false); }
  uint32_t get_insn(size_t i) { return insn[i]; }

private:
  T data[N];
  uint32_t insn[N];
  bool valid[N];

  long unsigned int dump(const freg_t& f) { return f.v[0]; }
  long unsigned int dump(const reg_t& x) { return x; }
};

class magic_t;

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

  // simif_t virtual function
  char* addr_to_mem(reg_t addr);
  bool mmio_load(reg_t addr, size_t len, uint8_t* bytes);
  bool mmio_store(reg_t addr, size_t len, const uint8_t* bytes);
  void proc_reset(unsigned id);
  const char* get_symbol(uint64_t addr);

  uint64_t get_random_executable_address(std::default_random_engine &random);
  bool in_fuzz_range(uint64_t addr) {
    return fuzz_start_addr <= addr && addr < fuzz_end_addr;
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

  checkboard_t<reg_t, NXPR, true> check_board;
  checkboard_t<freg_t, NFPR, false> f_check_board;

  void update_tohost_info();
private:
  std::vector<std::pair<reg_t, mem_t*>> mems;
  mmu_t* debug_mmu;
  std::vector<processor_t*> procs;
  reg_t start_pc;
  reg_t elf_entry;
  std::string dts;
  std::string dtb;
  std::string dtb_file;
  std::unique_ptr<rom_device_t> boot_rom;
  std::unique_ptr<clint_t> clint;
  std::unique_ptr<magic_t> magic;
  bus_t bus;
  
  bool finish;
  addr_t tohost_addr;
  addr_t fuzz_start_addr;
  addr_t fuzz_end_addr;
  reg_t tohost_data;
  std::map<uint64_t, std::string> addr2symbol;

  void reset();
  void idle();

  bool start_randomize;
};

extern const std::vector<magic_type*> magic_generator_type;

class magic_t : public abstract_device_t {
 public:
  magic_t(): seed(0), ignore(false), rand2(0, 1), generator({
    std::bind(&magic_t::rdm_dword, this, 64, 0),
    std::bind(&magic_t::rdm_dword, this, 32, 1),
    std::bind(&magic_t::rdm_float, this, -1, -1, 23, 31),
    std::bind(&magic_t::rdm_float, this, -1, -1, 52, 63),
    std::bind(&magic_t::rdm_address, this, -1, -1, -1, -1)
  }){}
  
  bool load(reg_t addr, size_t len, uint8_t* bytes) {
    reg_t id = addr / 8;
    reg_t tmp = id < generator.size() ? generator[id]() : 0;
    ignore = true;
    memcpy(bytes, &tmp, len);
    printf("[CJ] Magic read: %016lx[%ld] = %016lx\n", addr, len, tmp);
    return true;
  }
  
  bool store(reg_t addr, size_t len, const uint8_t* bytes) { return true; }
  size_t size() { return 4096; }
  void set_seed(reg_t new_seed) { seed = new_seed; random.seed(seed); }
  bool get_ignore() { return ignore; }
  void clear_ignore() { ignore = false; }

  reg_t rdm_dword(int width, int sgned);                    // 1 for signed, 0 for unsigned, -1 for random
  reg_t rdm_float(int type, int sgn, int botE, int botS);   // 0 for 0, 1 for INF, 2 for qNAN, 3 for sNAN, 4 for normal, 5 for tiny, -1 for random
  reg_t rdm_address(int r, int w, int x, int isLabel);      // 1 for yes, 0 for no, -1 for random

 private:
  reg_t seed;
  bool ignore;

  std::default_random_engine random;
  std::uniform_int_distribution<reg_t> rand2;
  std::vector<std::function<reg_t()>> generator;
};

extern cosim_cj_t* simulator;
#endif
