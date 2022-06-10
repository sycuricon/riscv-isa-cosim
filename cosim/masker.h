#ifndef _COSIM_MASKER_H
#define _COSIM_MASKER_H

#include <cstdlib>
#include <cstdint>
#include <vector>
#include <iostream>

// opcode mask
#define OP_MASK       (((1UL << 2) - 1) << 0)
#define OP_16_C0      0UL
#define OP_16_C1      1UL
#define OP_16_C2      2UL
#define OP_32         3UL

// field mask
#define OPCODE_MASK   (((1UL << 5) - 1) << 2)
#define FUNCT3_MASK   (((1UL << 3) - 1) << 12)
#define FUNCT7_MASK   (((1UL << 7) - 1) << 25)
#define FUNCT2_MASK   (((1UL << 2) - 1) << 25)
#define FUNCT5_MASK   (((1UL << 5) - 1) << 27)
#define FUNCT12_MASK  (((1UL << 12) - 1) << 20)
#define RS1_MASK      (((1UL << 5) - 1) << 15)
#define RS2_MASK      (((1UL << 5) - 1) << 20)
#define RD_MASK       (((1UL << 5) - 1) << 7)

typedef uint64_t rv_inst;

#include "masker_enum.h"

class masker_inst_t;

rv_format decode_inst_format(rv_opcode op);
void decode_inst_opcode(masker_inst_t* dec);
void decode_inst_oprand(masker_inst_t* dec);


class field_t {
public:
  rv_field  name;
  int64_t   value;
  rv_xlen   xlen;

  void set_xlen(rv_xlen len) { xlen = len;}
  rv_xlen get_xlen() const { return xlen; }
  virtual void decode(rv_inst inst) = 0;
  virtual rv_inst encode(rv_inst inst) = 0;
};

class masker_inst_t {
public:
  uint64_t  pc;
  rv_xlen   xlen;
  rv_inst   inst;
  rv_opcode op;
  std::vector<field_t*> args;

  masker_inst_t(rv_inst inst, rv_xlen xlen, uint64_t pc) :
    pc(pc), xlen(xlen), inst(inst) {}

  void decode() {
    for (auto arg : args)
      arg->decode(inst);
  }

  rv_inst encode(bool debug=false) {
    rv_inst new_inst;
    if ((inst & OP_MASK) == OP_32)
      new_inst = inst & 0x7f;
    else
      new_inst = inst & OP_MASK;

    for (auto arg : args)
      new_inst = arg->encode(new_inst);

    if (debug) {
      masker_inst_t tmp(new_inst, rv64, pc);
      decode_inst_opcode(&tmp);
      printf("\e[1;35m[CJ] insn mutation:  %016lx @ %08lx -> %08lx [%s] \e[0m\n", pc, inst, new_inst, rv_opcode_name[tmp.op]);
    }
      

    return new_inst;
  }

  void mutation(bool debug=false) {
    if (debug)
      printf("\e[1;35m[CJ] insn mutation:  %s @ %08lx\n", rv_opcode_name[op], inst);
    for (auto arg : args) {
      if (debug) {
        printf("\e[1;35m[CJ] insn mutation:  \t%s @ %08lx ->", rv_field_name[arg->name], arg->value);
      }
      arg->value ++;
      if (debug) {
        printf(" %08lx\e[0m\n", arg->value);
      }
    }
  }

};

#endif
