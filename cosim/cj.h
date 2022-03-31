#ifndef _COSIM_CJ_H
#include "config.h"
#include "cfg.h"
#include "sim.h"
#include "mmu.h"

// Using json to configure in future
class config_t : public cfg_t {
public:
  config_t()
    : cfg_t(
      std::make_pair((reg_t)0, (reg_t)0),
      nullptr, 1, DEFAULT_ISA, DEFAULT_PRIV),
      varch(DEFAULT_VARCH),
      logfile(NULL), dtsfile(NULL),
      mem_base(DRAM_BASE), mem_size(2048),
      cycle_freq(1000000000), time_freq_count(100)
      {}

  cfg_arg_t<const char *> varch;
  cfg_arg_t<const char *> logfile;
  cfg_arg_t<const char *> dtsfile;
  cfg_arg_t<reg_t> mem_base;
  cfg_arg_t<reg_t> mem_size;
  cfg_arg_t<size_t> cycle_freq;
  cfg_arg_t<size_t> time_freq_count;
};


class cosim_cj_t : simif_t {
 public:
  cosim_cj_t(cfg_t *cfg, reg_t start_pc, reg_t mem_size);
  cosim_cj_t(config_t& cfg);
  ~cosim_cj_t();

  char* addr_to_mem(reg_t addr) { return "FIXME";}
  bool mmio_load(reg_t addr, size_t len, uint8_t* bytes) {return false;}
  bool mmio_store(reg_t addr, size_t len, const uint8_t* bytes) {return false;}
  void proc_reset(unsigned id) {}
  const char* get_symbol(uint64_t addr) { return "FIXME";}

private:
  std::vector<std::pair<reg_t, mem_t*>> mems;
  mmu_t* debug_mmu;  // debug port into main memory
  std::vector<processor_t*> procs;
  std::pair<reg_t, reg_t> initrd_range;
  reg_t start_pc;
  std::string dts;
  std::string dtb;
  std::string dtb_file;
  bool dtb_enabled;
  std::unique_ptr<rom_device_t> boot_rom;
  std::unique_ptr<clint_t> clint;
  bus_t bus;

  void reset();
  void idle();
  void read_chunk(addr_t taddr, size_t len, void* dst);
  void write_chunk(addr_t taddr, size_t len, const void* src);
  size_t chunk_align() { return 8; }
  size_t chunk_max_size() { return 8; }
  void set_target_endianness(memif_endianness_t endianness);
  memif_endianness_t get_target_endianness() const;







};

#endif
