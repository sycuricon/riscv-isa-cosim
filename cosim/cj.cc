#include "cj.h"
#include "dts.h"
#include "mmu.h"
#include "libfdt.h"
#include "../VERSION"
#include "elfloader.h"
#include "magic_type.h"

#include <cstdio>
#include <sstream>

const char *reg_name[32] = {
    "x0", "ra", "sp",  "gp",  "tp", "t0", "t1", "t2",
    "s0", "s1", "a0",  "a1",  "a2", "a3", "a4", "a5",
    "a6", "a7", "s2",  "s3",  "s4", "s5", "s6", "s7",
    "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

const char *freg_name[32] = {
    "ft0", "ft1", "ft2",  "ft3",  "ft4", "ft5", "ft6", "ft7",
    "fs0", "fs1", "fa0",  "fa1",  "fa2", "fa3", "fa4", "fa5",
    "fa6", "fa7", "fs2",  "fs3",  "fs4", "fs5", "fs6", "fs7",
    "fs8", "fs9", "fs10", "fs11", "ft8", "ft9", "ft10", "ft11"
};

inline long unsigned int dump(const freg_t& f) { return f.v[0]; }
inline long unsigned int dump(const reg_t& x) { return x; }

cosim_cj_t::cosim_cj_t(config_t& cfg) :
  matched_reg_count_stat(33, 0), blind(false),
  mmio_access(false), tohost_addr(0), tohost_data(0),
  start_randomize(false) {

  isa_parser_t isa(cfg.isa(), cfg.priv());
  std::ostream sout_(nullptr);
  sout_.rdbuf(std::cerr.rdbuf());

  cj_debug = cfg.verbose();
  va_mask = cfg.va_mask();
  // blind = true;
  // cj_debug = false;

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
    if (cfg.verbose()) procs[i]->enable_log_commits();
  }

  // create device tree and compile
  // TODO: automatic create dummy device from dts
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

  // configure processor property
  void *fdt = (void *)dtb.c_str();
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

  // magic device, error device, rom*2, clint, uart, plic
  mmios.push_back(new dummy_device_t(0x0UL, 0x1000UL));
  mmios.push_back(new dummy_device_t(0x3000UL, 0x1000UL));
  mmios.push_back(new dummy_device_t(0x10000UL, 0x10000UL));
  mmios.push_back(new dummy_device_t(0x20000UL, 0x2000UL));
  mmios.push_back(new dummy_device_t(0x2000000UL, 0x10000UL));
  mmios.push_back(new dummy_device_t(0x64000000UL, 0x1000UL));
  mmios.push_back(new dummy_device_t(0xc000000UL, 0x4000000UL));
  for (size_t i = 0; i < mmios.size(); i++)
      bus.add_device(mmios[i]->addr(), mmios[i]);

  // load test elf and setup symbol table
  if (!cfg.elffile()) {
    std::cerr << "At least one testcase is required!\n";
    exit(1);
  }
  load_testcase(cfg.elffile());
  magic.reset(new magic_t());
  masker_inst_t::reset_mutation_history();

  // done
  fprintf(stderr, "[*] `Commit & Judge' General Co-simulation Framework\n");
  fprintf(stderr, "\t\tpowered by Spike " SPIKE_VERSION "\n");
  fprintf(stderr, "- core %ld, isa: %s %s %s\n",
          cfg.nprocs(), cfg.isa(), cfg.priv(), cfg.varch());
  fprintf(stderr, "- elf file: %s\n", cfg.elffile());
  fprintf(stderr, "- memory configuration: 0x%lx 0x%lx, tohost: 0x%lx\n",
          cfg.mem_base(), cfg.mem_size() << 20, tohost_addr);
  fprintf(stderr, "- fuzz information: [Handler] %ld page (0x%lx 0x%lx)\n",
          fuzz_handler_page_num, fuzz_handler_start_addr, fuzz_handler_end_addr);
  fprintf(stderr, "                    [Payload] %ld page (0x%lx 0x%lx)\n",
          fuzz_loop_page_num, fuzz_loop_entry_addr, fuzz_loop_exit_addr);  
  procs[0]->get_state()->pc = cfg.mem_base();

  for (auto it = text_label.begin(); it != text_label.end(); it++) {
      std::cout << std::hex << (*it) << ' ';
  } std::cout << '\n';
  for (auto it = data_label.begin(); it != data_label.end(); it++) {
      std::cout << std::hex << (*it) << ' ';
  } std::cout << '\n';
}

cosim_cj_t::~cosim_cj_t() {
  /*
  printf("[CJ] Matched reg count stat:\n");
  for (size_t i = 0; i <= 32; i++) {
    printf("[CJ] %d match(es): %u\n", i, matched_reg_count_stat[i]);
  }
  printf("\n");
  */
  for (size_t i = 0; i < procs.size(); i++)
    delete procs[i];
  for (size_t i = 0; i < mmios.size(); i++)
    delete mmios[i];
  delete debug_mmu;
}

void cosim_cj_t::load_testcase(const char* elffile) {
  // mems[0].second->reset();
  memif_t tmp(this);
  std::map<std::string, uint64_t> symbols = load_elf(elffile, &tmp, &elf_entry);
  if (symbols.count("tohost"))
    tohost_addr = symbols["tohost"];
  else
    fprintf(stderr, "warning: tohost symbols not in ELF; can't communicate with target\n");

  if (symbols.count("_fuzz_handler_start"))
    fuzz_handler_start_addr = symbols["_fuzz_handler_start"];
  else
    fuzz_handler_start_addr = 0;
  if (symbols.count("_fuzz_handler_end"))
    fuzz_handler_end_addr = symbols["_fuzz_handler_end"];
  else
    fuzz_handler_end_addr = 0;
  if (symbols.count("_fuzz_main_loop_entry"))
    fuzz_loop_entry_addr = symbols["_fuzz_main_loop_entry"];
  else
    fuzz_loop_entry_addr = 0;
  if (symbols.count("_fuzz_main_loop_exit"))
    fuzz_loop_exit_addr = symbols["_fuzz_main_loop_exit"];
  else
    fuzz_loop_exit_addr = 0;

  fuzz_loop_page_num = (fuzz_loop_exit_addr - fuzz_loop_entry_addr + 0xFFF) / 0x1000;
  fuzz_handler_page_num = (fuzz_handler_end_addr - fuzz_handler_start_addr + 0xFFF) / 0x1000;

  va_enable = !!symbols.count("vm_boot");
  for (auto i : symbols) {
    auto it = addr2symbol.find(i.second);
    if (it == addr2symbol.end())
      addr2symbol[i.second] = i.first;
    
    if (i.first.find("fuzztext_") != std::string::npos) {
      if (va_enable) {
        text_label.push_back(i.second - fuzz_loop_entry_addr + 0x1000);
      } else {
        text_label.push_back(i.second);
      }
    } else if (i.first.find("fuzzdata_") != std::string::npos) {
      if (va_enable) {
        data_label.push_back(i.second - fuzz_loop_entry_addr + 0x1000);
      } else {
        data_label.push_back(i.second);
      }
    }
  }
}


int cosim_cj_t::cosim_commit_stage(int hartid, reg_t dut_pc, uint32_t dut_insn, bool check) {
  processor_t* p = get_core(hartid);
  state_t* s = p->get_state();
  mmu_t* mmu = p->get_mmu();

  if (start_randomize && !in_fuzz_handler_range(s->pc)) {
    mmu->set_insn_rdm(true);
  } else {
    mmu->set_insn_rdm(false);
  }

  clear_mmio_access();

  do {
    // for (int i = 0; i < 8; i++) {
    //   printf("%s = 0x%016lx %s = 0x%016lx %s = 0x%016lx %s = 0x%016lx\n", 
    //     freg_name[4*i+0], dump(s->FPR[4*i+0]), freg_name[4*i+1], dump(s->FPR[4*i+1]),
    //     freg_name[4*i+2], dump(s->FPR[4*i+2]), freg_name[4*i+3], dump(s->FPR[4*i+3]));
    // }
    // for (int i = 0; i < 8; i++) {
    //   printf("%s = 0x%016lx %s = 0x%016lx %s = 0x%016lx %s = 0x%016lx\n", 
    //     reg_name[4*i+0], dump(s->XPR[4*i+0]), reg_name[4*i+1], dump(s->XPR[4*i+1]),
    //     reg_name[4*i+2], dump(s->XPR[4*i+2]), reg_name[4*i+3], dump(s->XPR[4*i+3]));
    // }
    p->step(1, p->pending_intrpt);
    mmu->set_insn_rdm(false);
  } while (get_core(0)->fix_pc);

  // printf("[CJ] current prev: %d mstatus : %lx\n", s->prv, s->mstatus->read());
  // printf("[CJ] tvec %lx %lx\n", s->mtvec->read(), s->stvec->read());
  
  if (!start_randomize && dut_insn == 0x00002013UL) {
    if (cj_debug) printf("[CJ] Enable insn randomization\n");
    start_randomize = true;
  } else if (start_randomize && dut_insn == 0xfff02013UL) {
    if (cj_debug) printf("[CJ] Disable insn randomization\n");
    start_randomize = false;
  } else if (dut_insn == 0x00102013UL) {
    if (cj_debug) printf("\e[1;33m[CJ] Reset mutation queue\e[0m\n");
    masker_inst_t::fence_mutation();
  } 


  // update tohost
  if (in_fuzz_handler_range(s->pc)) {
    auto data = debug_mmu->to_target(debug_mmu->load_uint64(tohost_addr));
    memcpy(&tohost_data, &data, sizeof(data));  
  }
  debug_mmu->store_uint64(tohost_addr, 0);  


  if (!check)
    return 0;

  reg_t sim_pc = s->last_pc;
  uint32_t sim_insn = s->last_insn;
  size_t regNo, fregNo;
  if (s->XPR.get_last_write(regNo)) {
  //  printf("write back: %d %016lx\n", regNo, s->XPR[regNo]);
    if (!check_board.set(regNo, s->XPR[regNo], dut_insn, dut_pc, get_mmio_access())) {
      printf("\x1b[31m[error] check board set %ld error \x1b[0m\n", regNo);
      if (blind) {
        tohost_data = 1;
        return 0;
      } else {
        return 10;
      }
    }
  }
  if (s->FPR.get_last_write(fregNo)) {
    if (!f_check_board.set(fregNo, s->FPR[fregNo], dut_insn, dut_pc, get_mmio_access())) {
      printf("\x1b[31m[error] float check board set %ld error \x1b[0m\n", fregNo);
      if (blind) {
        tohost_data = 1;
        return 0;
      } else {
        return 10;
      }
    }
  }

  if (dut_pc != sim_pc || dut_insn != sim_insn) {
    printf("\x1b[31m[error] PC SIM \x1b[33m%016lx\x1b[31m, DUT \x1b[36m%016lx\x1b[0m\n", sim_pc, dut_pc);
    printf("\x1b[31m[error] INSN SIM \x1b[33m%08x\x1b[31m, DUT \x1b[36m%08x\x1b[0m\n", sim_insn, dut_insn);
    for (int i = 0; i < 8; i++) {
      printf("%s = 0x%016lx %s = 0x%016lx %s = 0x%016lx %s = 0x%016lx\n", 
        freg_name[4*i+0], dump(s->FPR[4*i+0]), freg_name[4*i+1], dump(s->FPR[4*i+1]),
        freg_name[4*i+2], dump(s->FPR[4*i+2]), freg_name[4*i+3], dump(s->FPR[4*i+3]));
    }
    for (int i = 0; i < 8; i++) {
      printf("%s = 0x%016lx %s = 0x%016lx %s = 0x%016lx %s = 0x%016lx\n", 
        reg_name[4*i+0], dump(s->XPR[4*i+0]), reg_name[4*i+1], dump(s->XPR[4*i+1]),
        reg_name[4*i+2], dump(s->XPR[4*i+2]), reg_name[4*i+3], dump(s->XPR[4*i+3]));
    }
    if (blind) {
      tohost_data = 1;
      return 0;
    } else {
      return 255;
    }
  }

  return 0;
}


int cosim_cj_t::cosim_judge_stage(int hartid, int dut_waddr, reg_t dut_wdata, bool fc) {
  processor_t* p = get_core(hartid);
  state_t* s = p->get_state();

  if (fc) {
    if (!f_check_board.clear(dut_waddr, freg(f64(dut_wdata)))) {
      if (f_check_board.get_ignore(dut_waddr)) {
        s->FPR.write(dut_waddr, freg(f64(dut_wdata)));
        f_check_board.clear(dut_waddr);
        return 0;
      } else {
        printf("\x1b[31m[error] WDATA \x1b[33mSIM %016lx\x1b[31m, DUT \x1b[36m%016lx \x1b[0m\n", 
          dump(f_check_board.get_data(dut_waddr)), dump(freg(f64(dut_wdata))));
        printf("\x1b[31m[error] %016lx@%08x float check board check %d error \x1b[0m\n", f_check_board.get_pc(dut_waddr), f_check_board.get_insn(dut_waddr), dut_waddr);
        for (int i = 0; i < 8; i++) {
          printf("%s = 0x%016lx %s = 0x%016lx %s = 0x%016lx %s = 0x%016lx\n", 
            freg_name[4*i+0], dump(s->FPR[4*i+0]), freg_name[4*i+1], dump(s->FPR[4*i+1]),
            freg_name[4*i+2], dump(s->FPR[4*i+2]), freg_name[4*i+3], dump(s->FPR[4*i+3]));
        }
        for (int i = 0; i < 8; i++) {
          printf("%s = 0x%016lx %s = 0x%016lx %s = 0x%016lx %s = 0x%016lx\n", 
            reg_name[4*i+0], dump(s->XPR[4*i+0]), reg_name[4*i+1], dump(s->XPR[4*i+1]),
            reg_name[4*i+2], dump(s->XPR[4*i+2]), reg_name[4*i+3], dump(s->XPR[4*i+3]));
        } 
        if (blind) {
          s->FPR.write(dut_waddr, freg(f64(dut_wdata)));
          f_check_board.clear(dut_waddr);
          return 0;
        } else {
          return 255;
        }
      }

    }
  } else {
    if (!check_board.clear(dut_waddr, dut_wdata)) {
      if (check_board.get_ignore(dut_waddr)) {
        s->XPR.write(dut_waddr, dut_wdata);
        check_board.clear(dut_waddr);
        return 0;
      } else if ((check_board.get_insn(dut_waddr) & 0x7f) == 0x73) {
        // printf("\x1b[31m[warn] %016lx@%08lx CSR UNMATCH \x1b[33mSIM %016lx\x1b[31m, DUT \x1b[36m%016lx \x1b[0m\n", 
        //   check_board.get_pc(dut_waddr), check_board.get_insn(dut_waddr), dump(check_board.get_data(dut_waddr)), dump(dut_wdata));
        s->XPR.write(dut_waddr, dut_wdata);
        check_board.clear(dut_waddr);
        return 0;
      } else {
        printf("\x1b[31m[error] WDATA \x1b[33mSIM %016lx\x1b[31m, DUT \x1b[36m%016lx \x1b[0m\n", 
          dump(check_board.get_data(dut_waddr)), dump(dut_wdata));
        printf("\x1b[31m[error] %016lx@%08x check board clear %d error \x1b[0m\n", check_board.get_pc(dut_waddr), check_board.get_insn(dut_waddr), dut_waddr);
        for (int i = 0; i < 8; i++) {
          printf("%s = 0x%016lx %s = 0x%016lx %s = 0x%016lx %s = 0x%016lx\n", 
            freg_name[4*i+0], dump(s->FPR[4*i+0]), freg_name[4*i+1], dump(s->FPR[4*i+1]),
            freg_name[4*i+2], dump(s->FPR[4*i+2]), freg_name[4*i+3], dump(s->FPR[4*i+3]));
        }
        for (int i = 0; i < 8; i++) {
          printf("%s = 0x%016lx %s = 0x%016lx %s = 0x%016lx %s = 0x%016lx\n", 
            reg_name[4*i+0], dump(s->XPR[4*i+0]), reg_name[4*i+1], dump(s->XPR[4*i+1]),
            reg_name[4*i+2], dump(s->XPR[4*i+2]), reg_name[4*i+3], dump(s->XPR[4*i+3]));
        } 
        if (blind) {
          s->XPR.write(dut_waddr, dut_wdata);
          check_board.clear(dut_waddr);
          return 0;
        } else {
          return 255;
        }
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
  masker_inst_t insn(in, rv64, pc);
  decode_inst_opcode(&insn);
  
  uint64_t new_inst;
  // Mutate
  if (hint_insn(in) != not_hint) {
    new_inst = in;
  } else if (start_randomize && !in_fuzz_handler_range(pc)) {
    new_inst = insn.mutation(cj_debug);
  } else {
    new_inst = in;
  }

  insn.record_to_history(new_inst);
  return new_inst;
}

uint64_t cosim_cj_t::cosim_randomizer_data(unsigned int read_select) {
  return magic->load(read_select);
}

void cosim_cj_t::update_tohost_info() {
  // tohost_addr = 0x80001000
  switch (tohost_data & 0xff) {
    // case 0x02:
    //   fuzz_start_addr = (int64_t)tohost_data >> 8;
    //   printf("[CJ] fuzz_start_addr: %016lx(%016lx)\n", tohost_data, fuzz_start_addr);
    //   break;
    // case 0x12:
    //   fuzz_end_addr = (int64_t)tohost_data >> 8;
    //   printf("[CJ] fuzz_end_addr: %016lx(%016lx)\n", tohost_data, fuzz_end_addr);     
    //   break;
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
  
  if (bus.load(addr, len, bytes)) {
    mmio_access = true;
    return true;
  } else {
    return false;
  }
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

// magic device call back function
uint64_t cosim_cj_t::get_random_text_address(std::default_random_engine &random) {
  if (text_label.size() == 0) {
    return 0x20220718;
  }
  std::uniform_int_distribution<uint64_t> rand_label(0, text_label.size() - 1);
  auto select = text_label[rand_label(random)];
  if (cj_debug) printf("[CJ] generated random text label: %s(%016lx)\n", addr2symbol[select].c_str(), select);

  // processor_t* p = get_core(0);
  // state_t* s = p->get_state();
  // printf("pc 0x%lx  mepc 0x%lx  sepc 0x%lx\n", s->pc, s->mepc->read(), s->sepc->read());

  return select;
}

uint64_t cosim_cj_t::get_random_data_address(std::default_random_engine &random) {
  if (data_label.size() == 0) {
    return 0x20220723;
  }
  std::uniform_int_distribution<uint64_t> rand_page(0, data_label.size() - 1);
  auto select = data_label[rand_page(random)];
  if (cj_debug) printf("[CJ] generated random data page:%016lx\n", select);
  return select;
}

uint64_t cosim_cj_t::get_exception_return_address(std::default_random_engine &random, int smode) {
  processor_t* p = get_core(0);
  state_t* s = p->get_state();
  reg_t epc = smode ? s->sepc->read() : s->mepc->read();

  if (in_fuzz_loop_range(epc)) {  // step to the next inst
    reg_t epc_pa = va_enable ? (epc - 0x1000) + fuzz_loop_entry_addr : epc;
    int step = debug_mmu->test_insn_length(epc_pa);
    if (cj_debug) printf("[CJ] %cepc %016lx in fuzz range, stepping %d bytes\n", smode ? 's' : 'm', epc, step);
    return epc + step;
  } else { // load a randomly selected target
    if (cj_debug) printf("[CJ] %cepc %016lx out of fuzz range\n", smode ? 's' : 'm', epc);
    return magic->load(MAGIC_RDM_TEXT_ADDR);
  }
}

void cosim_cj_t::record_rd_mutation_stats(unsigned int matched_reg_count) {
  matched_reg_count_stat[matched_reg_count]++;
}


// Magic device randomization function
reg_t magic_t::rdm_dword(int width, int sgned) { // 1 for signed, 0 for unsigned, -1 for random
  std::uniform_int_distribution<reg_t> rand(0, (reg_t)(-1));
  reg_t mask = width == 64 ? -1 : (1LL << width)-1;
  reg_t masked_val = rand(random) & mask;
  bool sgn_ext = (sgned == -1) ? rand2(random) : sgned;
  if ((masked_val >> (width - 1)) & sgn_ext)
    masked_val |= ~mask;
  return  masked_val;
}

reg_t magic_t::rdm_float(int type, int sgn, int botE, int botS) {   // 0 for 0, 1 for INF, 2 for qNAN, 3 for sNAN, 4 for normal, 5 for tiny, -1 for random
  int topT = botE - 1;
  std::uniform_int_distribution<reg_t> randType(0, 5);
  std::uniform_int_distribution<reg_t> randE(1, (1 << (botS - botE)) - 2);
  std::uniform_int_distribution<reg_t> randT(1, (1 << botE)-1);
  std::uniform_int_distribution<reg_t> randS(0, (1 << topT)-1);

  reg_t base = botS == 63 ? 0 : (-1LL) << (botS + 1);
  reg_t sgn_bit = (sgn == -1 ? rand2(random) : (reg_t)sgn) << botS;
  type = (type == -1) ? randType(random) : type;
  switch (type) {
    case 0: return base | sgn_bit | 0;
    case 1: return base | sgn_bit | (255 << botE) | 0;
    case 2: return base | sgn_bit | (255 << botE) | (randS(random) |  (1 << topT));
    case 3: return base | sgn_bit | (255 << botE) | (randT(random) & ~(1 << topT));
    case 4: return base | sgn_bit | (randE(random) << botE) | (rand2(random) << topT) | randS(random);
    case 5: return base | sgn_bit |   (0 << botE) | randT(random);
    default: return 0;
  }
}

reg_t magic_t::rdm_address(int r, int w, int x) {  // 1 for yes, 0 for no, -1 for random
  if (x)
    return simulator->get_random_text_address(random);
  else
    return simulator->get_random_data_address(random);
}

reg_t magic_t::rdm_epc_next(int smode) {
  return simulator->get_exception_return_address(random, smode);
}

const std::vector<magic_type*> magic_generator_type({
  &magic_int,           /* MAGIC_RANDOM         */
  &magic_int,           /* MAGIC_RDM_WORD       */
  &magic_float,         /* MAGIC_RDM_FLOAT      */
  &magic_float,         /* MAGIC_RDM_DOUBLE     */
  &magic_text_address,  /* MAGIC_RDM_TEXT_ADDR  */
  &magic_data_address,  /* MAGIC_RDM_DATA_ADDR  */
  &magic_text_address,  /* MAGIC_MEPC_NEXT      */
  &magic_text_address   /* MAGIC_SEPC_NEXT      */
});