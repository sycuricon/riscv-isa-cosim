#ifndef _COSIM_MASKER_H
#define _COSIM_MASKER_H

#include <cstdlib>
#include <cstdint>
#include <vector>
#include <random>
#include <iostream>
#include <unordered_map>
#include <queue>

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
#include "magic_type.h"

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

template<class T, int capacity>
class circular_queue {
public:
  size_t size() {
    int t = tail - head;
    if (t < 0) t += capacity;
    return t;
  }
  void push(const T &v) {
    arr[tail++] = v;
    if (tail >= capacity) tail -= capacity;
  }
  T& front() {
    return arr[head];
  }
  void pop() {
    head++;
    if (head >= capacity) head -= capacity;
  }
  T& operator[](size_t i) {
    int t = i + head;
    if (i >= capacity) i -= capacity;
    return arr[i];
  } 
  void clear() {
    head = tail = 0;
  }
private:
  int head, tail;
  T arr[capacity];
};

struct rdm_entry {
  uint64_t pc;
  uint64_t insn;

  rdm_entry() : pc(0xdeadbeef), insn(0xdeadbeef) { }
  rdm_entry(uint64_t pc, uint64_t insn) {
    this->pc = pc;
    if ((insn & 0x3) == 0x3) this->insn = insn & 0xffffffff;
    else this->insn = insn & 0xffff;
  }

  bool operator==(const rdm_entry& other) const {
    if (this->pc == other.pc && this->insn == other.insn) return true;
    else return false;
  }

  struct HashFunction {
    size_t operator()(const rdm_entry& entry) const {
      static_assert(sizeof(size_t) >= 8, "size of size_t is less than 8");
      return ((entry.pc & 0xffffffff) << 32) | (entry.insn & 0xffffffff);
    }
  };
};

class masker_inst_t {
public:
  uint64_t  pc;
  rv_xlen   xlen;
  rv_inst   inst;
  rv_opcode op;
  std::vector<field_t*> args;

  masker_inst_t(rv_inst inst, rv_xlen xlen, uint64_t pc) :
    pc(pc), xlen(xlen), inst(inst) {
      // printf(">>> %016lx  %016lx\n", pc, inst);
    }

  void decode() {
    for (auto arg : args)
      arg->decode(inst);
  }

  rv_inst bare_op();

  rv_inst encode(bool debug=false);
  rv_inst mutation(bool debug=false);
  rv_inst replay_mutation(bool debug=false);

  void record_to_history(rv_inst new_inst);
  static void reset_mutation_history();
  static void fence_mutation();

private:
  static std::default_random_engine random;
  static std::uniform_int_distribution<uint64_t> rand2;
  static uint64_t randInt(uint64_t a, uint64_t b);
  static uint64_t randBits(uint64_t w);

  static int random_rd_in_pipeline();
  static int rd_with_type(magic_type *t);

  static std::unordered_map<rdm_entry, uint64_t, rdm_entry::HashFunction> history;
  static circular_queue<int, 16> rd_in_pipeline;
  static magic_type *type[32];
};

inline rv_hint hint_insn(uint64_t insn) {
  /* hint insn: slti x0, x0, imm */
  if ((insn & 0xfffff) == 0x02013) 
    return rdm_ops;
  /* magic device: l? x?, imm(x0) */
  else if ((insn & 0xf807f) == 0x00003)
    return magic_ops;
  else
    return not_hint;
}

#endif
