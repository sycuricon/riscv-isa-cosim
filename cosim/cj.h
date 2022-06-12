#ifndef _COSIM_CJ_H
#define _COSIM_CJ_H
#include "config.h"
#include "platform.h"
#include "cfg.h"
#include "processor.h"
#include "mmu.h"
#include "simif.h"
#include "memif.h"
#include "devices.h"

#include "masker.h"

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

  int cosim_commit_stage(int hartid, reg_t dut_pc, uint32_t dut_insn, bool check);
  int cosim_judge_stage(int hartid, int dut_waddr, reg_t dut_wdata, bool fc);
  void cosim_raise_trap(int hartid, reg_t cause);
  uint64_t cosim_randomizer_insn(uint64_t in, uint64_t pc);

  // simif_t virtual function
  char* addr_to_mem(reg_t addr);
  bool mmio_load(reg_t addr, size_t len, uint8_t* bytes);
  bool mmio_store(reg_t addr, size_t len, const uint8_t* bytes);
  void proc_reset(unsigned id);
  const char* get_symbol(uint64_t addr);

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

  void update_tohost_info() {
      if ((tohost_data & 0xff) == 0x02) {
        fuzz_start_addr = (int64_t)tohost_data >> 8;
        printf("[CJ] fuzz_start_addr: %016lx(%016lx)\n", tohost_data, fuzz_start_addr);
        debug_mmu->store_uint64(tohost_addr, 0);
      } else if ((tohost_data & 0xff) == 0x12) {
        fuzz_end_addr = (int64_t)tohost_data >> 8;
        printf("[CJ] fuzz_end_addr: %016lx(%016lx)\n", tohost_data, fuzz_end_addr);
        debug_mmu->store_uint64(tohost_addr, 0);       
      }
  }
private:
  std::vector<std::pair<reg_t, mem_t*>> mems;
  mmu_t* debug_mmu;
  std::vector<processor_t*> procs;
  reg_t start_pc;
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


class magic_t : public abstract_device_t {
 public:
  magic_t() : seed(0), ignore(false) {}
  bool load(reg_t addr, size_t len, uint8_t* bytes) {
    reg_t tmp = 0; 
    ignore = true;
    memcpy(bytes, &tmp, len);
    printf("[CJ] Magic read: %016lx[%ld]\n", addr, len);
    return true;
  }
  bool store(reg_t addr, size_t len, const uint8_t* bytes) { return true; }
  size_t size() { return 4096; }
  void set_seed(reg_t new_seed) { seed = new_seed; }
  bool get_ignore() { return ignore; }
  void clear_ignore() { ignore = false; }

 private:
  reg_t seed;
  bool ignore;
};


#endif
