#include "cj.h"
#include "elfloader.h"
#include "dts.h"
#include "libfdt.h"
#include "mmu.h"
#include "../VERSION"

#include <cstdio>
#include <sstream>

cosim_cj_t::cosim_cj_t(config_t& cfg) :
  finish(false), tohost_addr(0), tohost_data(0),
  start_randomize(false) {
  isa_parser_t isa(cfg.isa(), cfg.priv());
  std::ostream sout_(nullptr);
  sout_.rdbuf(std::cerr.rdbuf());

  // create memory and debug mmu
  mems.push_back(
      std::make_pair(cfg.mem_base(),
        new mem_t(cfg.mem_size() << 20)));
  bus.add_device(mems[0].first, mems[0].second);
  debug_mmu = new mmu_t(this, NULL);

  // create processor
  for (size_t i = 0; i < cfg.nprocs(); i++) {
    procs.push_back(new processor_t(isa, cfg.varch(), this, i, false, cfg.logfile(), sout_));
    if (!procs[i]) {
      std::cerr << "processor [" << i << "] create failed!\n";
      exit(1);
    }
    procs[i]->set_debug(true);
    procs[i]->cosim_verbose = cfg.verbose();
  }

  // create device tree and compile
  if (cfg.dtsfile()) {
    std::ifstream fin(cfg.dtsfile(), std::ios::binary);
    if (!fin.good()) {
      std::cerr << "can't find dtb file: " << dtb_file << std::endl;
      exit(-1);
    }

    std::stringstream strstream;
    strstream << fin.rdbuf();
    dtb = strstream.str();
  } else {
    std::pair<reg_t, reg_t> initrd_bounds = cfg.initrd_bounds();
    dts = make_dts(cfg.time_freq_count(), cfg.cycle_freq(),
                   initrd_bounds.first, initrd_bounds.second,
                   cfg.bootargs(), procs, mems);
    dtb = dts_compile(dts);
  }

  int fdt_code = fdt_check_header(dtb.c_str());
  if (fdt_code) {
    std::cerr << "Failed to read DTB from ";
    if (dtb_file.empty()) {
      std::cerr << "auto-generated DTS string";
    } else {
      std::cerr << "`" << dtb_file << "'";
    }
    std::cerr << ": " << fdt_strerror(fdt_code) << ".\n";
    exit(-1);
  }

  // create clint
  void *fdt = (void *)dtb.c_str();
  reg_t clint_base;
  if (fdt_parse_clint(fdt, &clint_base, "riscv,clint0") == 0) {
    clint.reset(new clint_t(procs, cfg.cycle_freq() / cfg.time_freq_count(), false));
    bus.add_device(clint_base, clint.get());
  }

  // configure processor property
  int cpu_offset = 0, rc;
  size_t cpu_idx = 0;
  cpu_offset = fdt_get_offset(fdt, "/cpus");
  if (cpu_offset < 0)
    return;

  for (cpu_offset = fdt_get_first_subnode(fdt, cpu_offset); cpu_offset >= 0;
       cpu_offset = fdt_get_next_subnode(fdt, cpu_offset)) {

    if (cpu_idx >= cfg.nprocs())
      break;

    // handle pmp
    reg_t pmp_num = 0, pmp_granularity = 0;
    if (fdt_parse_pmp_num(fdt, cpu_offset, &pmp_num) == 0) {
      if (pmp_num <= 64) {
        procs[cpu_idx]->set_pmp_num(pmp_num);
      } else {
        std::cerr << "core doesn't have valid 'riscv,pmpregions': "
                  << pmp_num << ".\n";
        exit(1);
      }
    } else {
      procs[cpu_idx]->set_pmp_num(0);
    }

    if (fdt_parse_pmp_alignment(fdt, cpu_offset, &pmp_granularity) == 0) {
      procs[cpu_idx]->set_pmp_granularity(pmp_granularity);
    }

    //handle mmu-type
    const char *mmu_type;
    rc = fdt_parse_mmu_type(fdt, cpu_offset, &mmu_type);
    if (rc == 0) {
      procs[cpu_idx]->set_mmu_capability(IMPL_MMU_SBARE);
      if (strncmp(mmu_type, "riscv,sv32", strlen("riscv,sv32")) == 0) {
        procs[cpu_idx]->set_mmu_capability(IMPL_MMU_SV32);
      } else if (strncmp(mmu_type, "riscv,sv39", strlen("riscv,sv39")) == 0) {
        procs[cpu_idx]->set_mmu_capability(IMPL_MMU_SV39);
      } else if (strncmp(mmu_type, "riscv,sv48", strlen("riscv,sv48")) == 0) {
        procs[cpu_idx]->set_mmu_capability(IMPL_MMU_SV48);
      } else if (strncmp(mmu_type, "riscv,sbare", strlen("riscv,sbare")) == 0) {
        //has been set in the beginning
      } else {
        std::cerr << "core has an invalid 'mmu-type': "
                  << mmu_type << ".\n";
        exit(1);
      }
    } else {
      procs[cpu_idx]->set_mmu_capability(IMPL_MMU_SBARE);
    }

    cpu_idx++;
  }

  // load test elf and setup symbol table
  if (!cfg.elffile()) {
    std::cerr << "At least one testcase is required!\n";
    exit(1);
  }
  memif_t tmp(this);
  reg_t elf_entry;
  std::map<std::string, uint64_t> symbols = load_elf(cfg.elffile(), &tmp, &elf_entry);
  if (symbols.count("tohost"))
    tohost_addr = symbols["tohost"];
  else
    fprintf(stderr, "warning: tohost symbols not in ELF; can't communicate with target\n");
  
  for (auto i : symbols) {
    auto it = addr2symbol.find(i.second);
    if ( it == addr2symbol.end())
      addr2symbol[i.second] = i.first;
  }

  // create bootrom
  const int reset_vec_size = 8;
  start_pc = cfg.start_pc() == reg_t(-1) ? elf_entry : cfg.start_pc();
  uint32_t reset_vec[reset_vec_size] = {
      0x297,                                      // auipc  t0, 0x0
      0x28593 + (reset_vec_size * 4 << 20),       // addi   a1, t0, &dtb
      0xf1402573,                                 // csrr   a0, mhartid
      get_core(0)->get_xlen() == 32 ?
      0x0182a283u :                                   // lw     t0, 24(t0)
      0x0182b283u,                                    // ld     t0, 24(t0)
      0x28067,                                    // jr     t0
      0,
      (uint32_t) (start_pc & 0xffffffff),
      (uint32_t) (start_pc >> 32)
  };
  if (get_target_endianness() == memif_endianness_big) {
    int i;
    // Instuctions are little endian
    for (i = 0; reset_vec[i] != 0; i++)
      reset_vec[i] = to_le(reset_vec[i]);
    // Data is big endian
    for (; i < reset_vec_size; i++)
      reset_vec[i] = to_be(reset_vec[i]);

    // Correct the high/low order of 64-bit start PC
    if (get_core(0)->get_xlen() != 32)
      std::swap(reset_vec[reset_vec_size-2], reset_vec[reset_vec_size-1]);
  } else {
    for (int i = 0; i < reset_vec_size; i++)
      reset_vec[i] = to_le(reset_vec[i]);
  }

  std::vector<char> rom((char*)reset_vec, (char*)reset_vec + sizeof(reset_vec));

  rom.insert(rom.end(), dtb.begin(), dtb.end());
  const int align = 0x1000;
  rom.resize((rom.size() + align - 1) / align * align);

  boot_rom.reset(new rom_device_t(rom));
  bus.add_device(DEFAULT_RSTVEC, boot_rom.get());

  magic.reset(new magic_t());
  bus.add_device(0, magic.get());

 // done
  fprintf(stderr, "[*] `Commit & Judge' General Co-simulation Framework\n");
  fprintf(stderr, "\t\tpowered by Spike " SPIKE_VERSION "\n");
  fprintf(stderr, "- core %ld, isa: %s %s %s\n",
          cfg.nprocs(), cfg.isa(), cfg.priv(), cfg.varch());
  fprintf(stderr, "- memory configuration: 0x%lx 0x%lx, tohost: 0x%lx\n",
          cfg.mem_base(), cfg.mem_size() << 20, tohost_addr);
  fprintf(stderr, "- elf file: %s\n", cfg.elffile());
  procs[0]->get_state()->pc = cfg.mem_base();
}

cosim_cj_t::~cosim_cj_t() {
  for (size_t i = 0; i < procs.size(); i++)
    delete procs[i];
  delete debug_mmu;
}

int cosim_cj_t::cosim_commit_stage(int hartid, reg_t dut_pc, uint32_t dut_insn, bool check) {
  processor_t* p = get_core(hartid);
  state_t* s = p->get_state();
  mmu_t* mmu = p->get_mmu();

  if (!start_randomize && dut_insn == 0x00002013UL) {
    printf("[CJ] Enable insn randomization\n");
    start_randomize = true;
  } else if (start_randomize && dut_insn == 0xfff02013UL) {
    printf("[CJ] Disable insn randomization\n");
    start_randomize = false;
  }

  printf("[CJ] mutation condition: %d %016lx %016lx %016lx\n", 
          start_randomize, s->pc, fuzz_start_addr, fuzz_end_addr);

  if (start_randomize && (s->pc <= fuzz_end_addr && s->pc >= fuzz_start_addr)) {
    mmu->set_insn_rdm(start_randomize);
  }

  do {
    p->step(1, p->pending_intrpt);
  } while (get_core(0)->fix_pc);

  // update tohost
  auto data = debug_mmu->to_target(debug_mmu->load_uint64(tohost_addr));
  memcpy(&tohost_data, &data, sizeof data);
  update_tohost_info();

  if (!check)
    return 0;

  reg_t sim_pc = s->last_pc;
  uint32_t sim_insn = s->last_insn;
  size_t regNo, fregNo;
  if (s->XPR.get_last_write(regNo)) {
//    printf("write back: %016lx\n", s->XPR[regNo]);
    if (!check_board.set(regNo, s->XPR[regNo], dut_insn)) {
      printf("\x1b[31m[error] check board set %ld error \x1b[0m\n", regNo);
      return 10;
    }
  }
  if (s->FPR.get_last_write(fregNo)) {
    if (!f_check_board.set(fregNo, s->FPR[fregNo], dut_insn)) {
      printf("\x1b[31m[error] float check board set %ld error \x1b[0m\n", fregNo);
      return 10;
    }
  }

  if (dut_pc != sim_pc ||
      dut_insn != sim_insn) {
    printf("\x1b[31m[error] PC SIM \x1b[33m%016lx\x1b[31m, DUT \x1b[36m%016lx\x1b[0m\n", sim_pc, dut_pc);
    printf("\x1b[31m[error] INSN SIM \x1b[33m%08x\x1b[31m, DUT \x1b[36m%08x\x1b[0m\n", sim_insn, dut_insn);
    return 255;
  }

  return 0;
}
int cosim_cj_t::cosim_judge_stage(int hartid, int dut_waddr, reg_t dut_wdata, bool fc) {
  processor_t* p = get_core(hartid);
  state_t* s = p->get_state();

  if (fc) {
    if (!f_check_board.clear(dut_waddr, freg(f64(dut_wdata)))) {
      if (magic->get_ignore() && ((f_check_board.get_insn(dut_waddr) & 0x7f) == 0x07)) {
        s->FPR.write(dut_waddr, freg(f64(dut_wdata)));
        magic->clear_ignore();
        f_check_board.clear(dut_waddr);
        return 0;
      } else {
        printf("\x1b[31m[error] float check board check %d error \x1b[0m\n", dut_waddr);
        return 10;
      }

    }
  } else {
    if (!check_board.clear(dut_waddr, dut_wdata)) {
      if (magic->get_ignore() && ((check_board.get_insn(dut_waddr) & 0x7f) == 0x03)) {
        s->XPR.write(dut_waddr, dut_wdata);
        magic->clear_ignore();
        check_board.clear(dut_waddr);
        return 0;
      } else {
        printf("\x1b[31m[error] check board clear %d error \x1b[0m\n", dut_waddr);
        return 10;
      }

    }
  }

  return 0;
}
void cosim_cj_t::cosim_raise_trap(int hartid, reg_t cause) {
  processor_t* p = get_core(hartid);
  state_t* s = p->get_state();
  reg_t except_code = cause & 63;
  s->mip->backdoor_write_with_mask(1 << except_code, 1 << except_code);
  p->pending_intrpt = true;
}
uint64_t cosim_cj_t::cosim_randomizer_insn(uint64_t in, uint64_t pc) {

  if (in == 0x00002013UL || in == 0xfff02013UL) {
    return in;
  } else if (start_randomize && (pc <= fuzz_end_addr && pc >= fuzz_start_addr)) {
    masker_inst_t insn(in, rv64, pc);
    decode_inst_opcode(&insn);
    decode_inst_oprand(&insn);
    insn.mutation(true);
    rv_inst enc = insn.encode(true);
    return enc;
  } else {
   return in;
  }
}



// chunked_memif_t virtual function
void cosim_cj_t::read_chunk(addr_t taddr, size_t len, void* dst) {
  assert(len == 8);
  auto data = debug_mmu->to_target(debug_mmu->load_uint64(taddr));
  memcpy(dst, &data, sizeof data);
}

void cosim_cj_t::write_chunk(addr_t taddr, size_t len, const void* src) {
  assert(len == 8);
  target_endian<uint64_t> data;
  memcpy(&data, src, sizeof data);
  debug_mmu->store_uint64(taddr, debug_mmu->from_target(data));
}

void cosim_cj_t::clear_chunk(addr_t taddr, size_t len) {
  char zeros[chunk_max_size()];
  memset(zeros, 0, chunk_max_size());

  for (size_t pos = 0; pos < len; pos += chunk_max_size())
    write_chunk(taddr + pos, std::min(len - pos, chunk_max_size()), zeros);
}

void cosim_cj_t::set_target_endianness(memif_endianness_t endianness) {
  assert(endianness == memif_endianness_little);
}

memif_endianness_t cosim_cj_t::get_target_endianness() const {
  return memif_endianness_little;
}

// simif_t virtual function
static bool paddr_ok(reg_t addr) {
  return (addr >> MAX_PADDR_BITS) == 0;
}

char* cosim_cj_t::addr_to_mem(reg_t addr) {
  if (!paddr_ok(addr))
    return NULL;
  auto desc = bus.find_device(addr);
  if (auto mem = dynamic_cast<mem_t*>(desc.second))
    if (addr - desc.first < mem->size())
      return mem->contents(addr - desc.first);
  return NULL;
}

bool cosim_cj_t::mmio_load(reg_t addr, size_t len, uint8_t* bytes) {
  if (addr + len < addr || !paddr_ok(addr + len - 1))
    return false;
  return bus.load(addr, len, bytes);
}

bool cosim_cj_t::mmio_store(reg_t addr, size_t len, const uint8_t* bytes) {
  if (addr + len < addr || !paddr_ok(addr + len - 1))
    return false;
  return bus.store(addr, len, bytes);
}

void cosim_cj_t::proc_reset(unsigned id) {
  // not implement here
  printf("ToDo: cosim_cj_t::proc_reset\n");
}

const char* cosim_cj_t::get_symbol(uint64_t addr) {
  auto it = addr2symbol.find(addr);
  if(it == addr2symbol.end())
    return nullptr;
  return it->second.c_str();
}

//__attribute__((weak)) int main() {
//  printf("Hello v0.3\n");
//  config_t cfg;
//  cfg.elffile = "/eda/project/riscv-tests/build/isa/rv64mi-p-illegal";
//  cosim_cj_t cj(cfg);
//
//  while (true) {
//    cj.cosim_commit_stage(0, 0, 0, false);
//  }
//
//  printf("Exit ...\n");
//
//  return 0;
//}