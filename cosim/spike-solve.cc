#include "cj.h"
#include <stdio.h>
#include <stdlib.h>


int main(int argc, const char *argv[]) {
    config_t cfg;
    cfg.verbose = true;
    cfg.isa = "rv64gc_zicsr_zicntr";
    cfg.boot_addr = 0x80000000;
    cfg.elffiles = std::vector<std::string>{
        argv[1]};
    cfg.mem_layout = std::vector<mem_cfg_t>{
        mem_cfg_t(0x80000000UL, 0x80000000UL),
    };

    cfg.logfile = fopen("/dev/null", "wt");
    cosim_cj_t *simulator = new cosim_cj_t(cfg);
    processor_t *core = simulator->get_core(0);

    int cnt = atoi(argv[2]);

    for (int i = 0; i < cnt; i++) {
        simulator->cosim_commit_stage(0, 0, 0, false);
    }

    FILE *dump_file = fopen(argv[3], "wt");
    const char *reg_name[32] = {
        "ZERO", "RA", "SP", "GP", "TP", "T0", "T1", "T2", "S0", "S1", "A0", "A1",
        "A2", "A3", "A4", "A5", "A6", "A7", "S2", "S3", "S4", "S5", "S6", "S7",
        "S8", "S9", "S10", "S11", "T3", "T4", "T5", "T6"};
    for (int i = 0; i < 32; i++) {
        fprintf(dump_file, "%s %016lx\n", reg_name[i], core->get_state()->XPR[i]);
    }

    const char *freg_name[32] = {
        "FT0", "FT1", "FT2", "FT3", "FT4", "FT5", "FT6", "FT7",
        "FS0", "FS1", "FA0", "FA1", "FA2", "FA3", "FA4", "FA5",
        "FA6", "FA7", "FS2", "FS3", "FS4", "FS5", "FS6", "FS7",
        "FS8", "FS9", "FS10", "FS11", "FT8", "FT9", "FT10", "FT11"};
    for (int i = 0; i < 32; i++) {
        fprintf(dump_file, "%s %016lx\n", freg_name[i], core->get_state()->FPR[i].v[0]);
    }
}
