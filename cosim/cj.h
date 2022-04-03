#ifndef _COSIM_CJ_H
#include "config.h"
#include "platform.h"
#include "cfg.h"
#include "processor.h"
#include "mmu.h"
#include "simif.h"
#include "memif.h"
#include "devices.h"

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


template <class T, size_t N, bool ignore_zero>
class checkboard_t {
public:
  int set(size_t i, T value) {
    if (!ignore_zero || i != 0) {
//      printf("set %d\n", i);
      if (valid[i])
        return 1;
      valid[i] = !valid[i];
      data[i] = value;
    }
    return 0;
  }
  int clear(size_t i, T value) {
    if (!ignore_zero || i != 0) {
//      printf("clear %d\n", i);
      if (!valid[i])
        return 1;
      if (data[i] != value) {
        printf("\x1b[33m[error] WDATA EMU %016lx \x1b[31m, DUT \x1b[36m%016lx \x1b[0m\n", data[i], value);
        return 2;
      }

      valid[i] = !valid[i];
    }
    return 0;
  }

  checkboard_t() {
    reset();
  }
  void reset() {
    std::fill(std::begin(valid), std::end(valid), false);
  }
private:
  T data[N];
  bool valid[N];
};

class cosim_cj_t : simif_t, chunked_memif_t {
 public:
  cosim_cj_t(config_t& cfg);
  ~cosim_cj_t();

  int cosim_commit_stage(int hartid, reg_t dut_pc, uint32_t dut_insn, bool check);
  int cosim_judge_stage(int hartid, int dut_waddr, reg_t dut_wdata, bool fc);
  void cosim_raise_trap(int hartid, reg_t cause);

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
  bus_t bus;

  addr_t tohost_addr;
  reg_t tohost_data;
  std::map<uint64_t, std::string> addr2symbol;

  void reset();
  void idle();

};

#endif
