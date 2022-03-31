#include "cj.h"
#include "platform.h"
#include "dts.h"
#include "libfdt.h"
#include "../VERSION"

#include <cstdio>

cosim_cj_t::cosim_cj_t(cfg_t* cfg, reg_t start_pc, reg_t mem_size)
{
  fprintf(stderr, "Starship Commit & Judge General Co-simulation Framework with Spike " SPIKE_VERSION "\n\n");

  const char* varch = DEFAULT_VARCH;
  bool halted = false;
  bool real_time_clint = false;
  std::vector<std::pair<reg_t, abstract_device_t*>> plugin_devices;
  std::vector<std::string> htif_args;
  std::vector<int> hartids;
  std::vector<std::pair<reg_t, mem_t*>> mems(1, std::make_pair(reg_t(DRAM_BASE), new mem_t(mem_size)));
  debug_module_config_t dm_config = {
          .progbufsize = 2,
          .max_sba_data_width = 0,
          .require_authentication = false,
          .abstract_rti = 0,
          .support_hasel = true,
          .support_abstract_csr_access = true,
          .support_haltgroups = true,
          .support_impebreak = true
  };
  const char *log_path = nullptr;
  const char* dtb_file = NULL;
  bool dtb_enabled = true;
  FILE *cmd_file = NULL;

#ifdef HAVE_BOOST_ASIO
    boost::asio::io_service *io_service_ptr = NULL; // needed for socket command interface option -s
    boost::asio::ip::tcp::acceptor *acceptor_ptr = NULL;
#endif

  sim_t s(cfg, varch, halted, real_time_clint,
    start_pc, mems, plugin_devices, htif_args,
    hartids, dm_config, log_path, dtb_enabled, dtb_file,
#ifdef HAVE_BOOST_ASIO
    io_service_ptr, acceptor_ptr,
#endif
    cmd_file);

}

cosim_cj_t::cosim_cj_t(config_t& cfg) {
  isa_parser_t isa(cfg.isa(), cfg.priv());
  FILE* logfile = NULL;
  std::ostream sout_(nullptr);

  fprintf(stderr, "Starship Commit & Judge General Co-simulation Framework with Spike " SPIKE_VERSION "\n\n");

  if (cfg.logfile()) {
    logfile = fopen(cfg.logfile(), "w");
  }
  sout_.rdbuf(std::cerr.rdbuf());

  // create memory and debug mmu
  bus.add_device(cfg.mem_base(), new mem_t(cfg.mem_size() << 20));
  debug_mmu = new mmu_t(this, NULL);

  // create processor
  for (size_t i = 0; i < cfg.nprocs(); i++) {
    procs.push_back(new processor_t(isa, cfg.varch(), this, i, false, logfile, sout_));
    if (!procs[i]) {
      std::cerr << "processor [" << i << "] create failed!" << std::endl ;
      exit(1);
    }
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
}


cosim_cj_t::~cosim_cj_t() {
  for (size_t i = 0; i < procs.size(); i++)
    delete procs[i];
  delete debug_mmu;
}


__attribute__((weak)) int main() {
  printf("Hello v0.2\n");
  config_t cfg;
  cosim_cj_t cj(cfg);
  printf("Hello v0.2\n");



  return 0;
}