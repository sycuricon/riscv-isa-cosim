#include "masker.h"
#include "cj.h"
#include "masker_insn_fields.h"

#include <queue>

std::default_random_engine masker_inst_t::random;
std::uniform_int_distribution<uint64_t> masker_inst_t::rand2(0, 1);
std::unordered_map<rdm_entry, uint64_t, rdm_entry::HashFunction> masker_inst_t::history;
circular_queue<int, 16> masker_inst_t::rd_in_pipeline;
magic_type *masker_inst_t::type[32];


void decode_inst_opcode(masker_inst_t* dec) {
  rv_xlen xlen = dec->xlen;
  rv_inst inst = dec->inst;
  rv_opcode op = rv_op_illegal;
  switch (((inst >> 0) & 0b11)) {
    case 0: /* rv16_c0 */
      switch (((inst >> 13) & 0b111)) {
        case 0: op = ((inst >> 2) & 0b111) == 0 ? rv_op_illegal : rv_op_c_addi4spn; break;
        case 1: op = xlen == rv128 ? rv_op_c_lq : rv_op_c_fld; break;
        case 2: op = rv_op_c_lw; break;
        case 3: op = xlen == rv32 ? rv_op_c_flw : rv_op_c_ld ; break;
          /* 4 is reserved */
        case 5: op = xlen == rv128 ? rv_op_c_sq : rv_op_c_fsd; break;
        case 6: op = rv_op_c_sw; break;
        case 7: op = xlen == rv32 ? rv_op_c_fsw : rv_op_c_sd; break;
      } break; /* rv16_c0 */
    case 1: /* rv16_c1 */
      switch (((inst >> 13) & 0b111)) {
        case 0: op = ((inst >> 7) & 0b11111) == 0 ? rv_op_c_nop : rv_op_c_addi; break;
        case 1: op = xlen == rv32 ? rv_op_c_jal : rv_op_c_addiw; break;
        case 2: op = rv_op_c_li; break;
        case 3: op = ((inst >> 7) & 0b11111) == 2 ? rv_op_c_addi16sp : rv_op_c_lui; break;
        case 4:
          switch (((inst >> 10) & 0b11)) {
            case 0: op = rv_op_c_srli; break;
            case 1: op = rv_op_c_srai; break;
            case 2: op = rv_op_c_andi; break;
            case 3:
              switch (((inst >> 10) & 0b100) | ((inst >> 5) & 0b011)) {
                case 0: op = rv_op_c_sub; break;
                case 1: op = rv_op_c_xor; break;
                case 2: op = rv_op_c_or; break;
                case 3: op = rv_op_c_and; break;
                case 4: op = rv_op_c_subw; break;
                case 5: op = rv_op_c_addw; break;
              } break;
          } break;
        case 5: op = rv_op_c_j; break;
        case 6: op = rv_op_c_beqz; break;
        case 7: op = rv_op_c_bnez; break;
      } break; /* rv16_c1 */
    case 2: /* rv16_c2*/
      switch (((inst >> 13) & 0b111)) {
        case 0: op = rv_op_c_slli; break;
        case 1: op = xlen == rv128 ? rv_op_c_lqsp : rv_op_c_fldsp; break;
        case 2: op = rv_op_c_lwsp; break;
        case 3: op = xlen == rv32 ? rv_op_c_flwsp : rv_op_c_ldsp; break;
        case 4:
          switch (((inst >> 12) & 0b1)) {
            case 0: op = ((inst >> 2) & 0b11111) == 0 ? rv_op_c_jr : rv_op_c_mv; break;
            case 1: op = ((inst >> 7) & 0b11111) == 0 ? rv_op_c_ebreak :
                         ((inst >> 2) & 0b11111) == 0 ? rv_op_c_jalr : rv_op_c_add; break;
          } break;
        case 5: op = xlen == rv128 ? rv_op_c_sqsp : rv_op_c_fsdsp; break;
        case 6: op = rv_op_c_swsp; break;
        case 7: op = xlen == rv32 ? rv_op_c_fswsp : rv_op_c_sdsp; break;
      } break; /* rv16_c2*/
    case 3: /* rv32,rv64 */
      switch (((inst >> 2) & 0b11111)) {
        case 0: /* LOAD funct3 */
          switch (((inst >> 12) & 0b111)) {
            case 0: op = rv_op_lb; break;
            case 1: op = rv_op_lh; break;
            case 2: op = rv_op_lw; break;
            case 3: op = rv_op_ld; break;
            case 4: op = rv_op_lbu; break;
            case 5: op = rv_op_lhu; break;
            case 6: op = rv_op_lwu; break;
            case 7: op = rv_op_ldu; break;
          } break;
        case 1: /* LOAD-FP funct3 */
          switch (((inst >> 12) & 0b111)) {
            case 2: op = rv_op_flw; break;
            case 3: op = rv_op_fld; break;
            case 4: op = rv_op_flq; break;
          } break;
        case 3: /* MISC-MEM funct3 */
          switch (((inst >> 12) & 0b111)) {
            case 0: op = rv_op_fence; break;
            case 1: op = rv_op_fence_i; break;
            case 2: op = rv_op_lq; break;
          } break;
        case 4: /* OP-IMM funct3, funct7 */
          switch (((inst >> 12) & 0b111)) {
            case 0: op = rv_op_addi; break;
            case 1: op = ((inst >> (25+xlen-rv32)) & ((1UL << (7-(xlen-rv32)))-1)) == 0 ? rv_op_slli : rv_op_illegal; break;
            case 2: op = rv_op_slti; break;
            case 3: op = rv_op_sltiu; break;
            case 4: op = rv_op_xori; break;
            case 5: op = ((inst >> (25+xlen-rv32)) & ((1UL << (7-(xlen-rv32)))-1)) == 0 ? rv_op_srli :
                         ((inst >> (25+xlen-rv32)) & ((1UL << (7-(xlen-rv32)))-1)) == (1UL << (5-(xlen-rv32))) ?
                           rv_op_srai : rv_op_illegal; break;
            case 6: op = rv_op_ori; break;
            case 7: op = rv_op_andi; break;
          } break;
        case 5: /* AUIPC */
          op = rv_op_auipc; break;
        case 6: /* OP-IMM-32 funct3, funct7 */
          switch (((inst >> 12) & 0b111)) {
            case 0: op = rv_op_addiw; break;
            case 1: op = ((inst >> 25) & 0b1111111) == 0 ? rv_op_slliw : rv_op_illegal; break;
            case 5: op = ((inst >> 25) & 0b1111111) == 0 ? rv_op_srliw :
                         ((inst >> 25) & 0b1111111) == 32 ? rv_op_sraiw : rv_op_illegal; break;
          } break;
        case 8: /* STORE funct3 */
          switch (((inst >> 12) & 0b111)) {
            case 0: op = rv_op_sb; break;
            case 1: op = rv_op_sh; break;
            case 2: op = rv_op_sw; break;
            case 3: op = rv_op_sd; break;
            case 4: op = rv_op_sq; break;
          } break;
        case 9: /* STORE-FP funct3 */
          switch (((inst >> 12) & 0b111)) {
            case 2: op = rv_op_fsw; break;
            case 3: op = rv_op_fsd; break;
            case 4: op = rv_op_fsq; break;
          } break;
        case 11: /* AMO funct3, funct5 */
          switch (((inst >> 24) & 0b11111000) | ((inst >> 12) & 0b00000111)) {
            case 2: op = rv_op_amoadd_w; break;
            case 3: op = rv_op_amoadd_d; break;
            case 4: op = rv_op_amoadd_q; break;
            case 10: op = rv_op_amoswap_w; break;
            case 11: op = rv_op_amoswap_d; break;
            case 12: op = rv_op_amoswap_q; break;
            case 18: op = ((inst >> 20) & 0b11111) == 0 ? rv_op_lr_w : rv_op_illegal; break;
            case 19: op = ((inst >> 20) & 0b11111) == 0 ? rv_op_lr_d : rv_op_illegal; break;
            case 20: op = ((inst >> 20) & 0b11111) == 0 ? rv_op_lr_q : rv_op_illegal; break;
            case 26: op = rv_op_sc_w; break;
            case 27: op = rv_op_sc_d; break;
            case 28: op = rv_op_sc_q; break;
            case 34: op = rv_op_amoxor_w; break;
            case 35: op = rv_op_amoxor_d; break;
            case 36: op = rv_op_amoxor_q; break;
            case 66: op = rv_op_amoor_w; break;
            case 67: op = rv_op_amoor_d; break;
            case 68: op = rv_op_amoor_q; break;
            case 98: op = rv_op_amoand_w; break;
            case 99: op = rv_op_amoand_d; break;
            case 100: op = rv_op_amoand_q; break;
            case 130: op = rv_op_amomin_w; break;
            case 131: op = rv_op_amomin_d; break;
            case 132: op = rv_op_amomin_q; break;
            case 162: op = rv_op_amomax_w; break;
            case 163: op = rv_op_amomax_d; break;
            case 164: op = rv_op_amomax_q; break;
            case 194: op = rv_op_amominu_w; break;
            case 195: op = rv_op_amominu_d; break;
            case 196: op = rv_op_amominu_q; break;
            case 226: op = rv_op_amomaxu_w; break;
            case 227: op = rv_op_amomaxu_d; break;
            case 228: op = rv_op_amomaxu_q; break;
          } break;
        case 12: /* OP funct3, funct7 */
          switch (((inst >> 22) & 0b1111111000) | ((inst >> 12) & 0b0000000111)) {
            case 0: op = rv_op_add; break;
            case 1: op = rv_op_sll; break;
            case 2: op = rv_op_slt; break;
            case 3: op = rv_op_sltu; break;
            case 4: op = rv_op_xor; break;
            case 5: op = rv_op_srl; break;
            case 6: op = rv_op_or; break;
            case 7: op = rv_op_and; break;
            case 8: op = rv_op_mul; break;
            case 9: op = rv_op_mulh; break;
            case 10: op = rv_op_mulhsu; break;
            case 11: op = rv_op_mulhu; break;
            case 12: op = rv_op_div; break;
            case 13: op = rv_op_divu; break;
            case 14: op = rv_op_rem; break;
            case 15: op = rv_op_remu; break;
            case 256: op = rv_op_sub; break;
            case 261: op = rv_op_sra; break;
          } break;
        case 13: /* LUI */
          op = rv_op_lui; break;
        case 14: /* OP-32 funct3, funct7 */
          switch (((inst >> 22) & 0b1111111000) | ((inst >> 12) & 0b0000000111)) {
            case 0: op = rv_op_addw; break;
            case 1: op = rv_op_sllw; break;
            case 5: op = rv_op_srlw; break;
            case 8: op = rv_op_mulw; break;
            case 12: op = rv_op_divw; break;
            case 13: op = rv_op_divuw; break;
            case 14: op = rv_op_remw; break;
            case 15: op = rv_op_remuw; break;
            case 256: op = rv_op_subw; break;
            case 261: op = rv_op_sraw; break;
          } break;
        case 16: /* MADD funct2 */
          op = ((inst >> 25) & 0b11) == 0 ? rv_op_fmadd_s :
               ((inst >> 25) & 0b11) == 1 ? rv_op_fmadd_d :
               ((inst >> 25) & 0b11) == 3 ? rv_op_fmadd_q : rv_op_illegal; break;
        case 17: /* MSUB funct2 */
          op = ((inst >> 25) & 0b11) == 0 ? rv_op_fmsub_s :
               ((inst >> 25) & 0b11) == 1 ? rv_op_fmsub_d :
               ((inst >> 25) & 0b11) == 3 ? rv_op_fmsub_q : rv_op_illegal; break;
        case 18: /* NMSUB funct2 */
          op = ((inst >> 25) & 0b11) == 0 ? rv_op_fnmsub_s :
               ((inst >> 25) & 0b11) == 1 ? rv_op_fnmsub_d :
               ((inst >> 25) & 0b11) == 3 ? rv_op_fnmsub_q : rv_op_illegal; break;
        case 19: /* NMADD funct2 */
          op = ((inst >> 25) & 0b11) == 0 ? rv_op_fnmadd_s :
               ((inst >> 25) & 0b11) == 1 ? rv_op_fnmadd_d :
               ((inst >> 25) & 0b11) == 3 ? rv_op_fnmadd_q : rv_op_illegal; break;
        case 20: /* OP-FP funct7, funct3, rs2 */
          switch (((inst >> 25) & 0b1111111)) {
            case 0: op = rv_op_fadd_s; break;
            case 1: op = rv_op_fadd_d; break;
            case 3: op = rv_op_fadd_q; break;
            case 4: op = rv_op_fsub_s; break;
            case 5: op = rv_op_fsub_d; break;
            case 7: op = rv_op_fsub_q; break;
            case 8: op = rv_op_fmul_s; break;
            case 9: op = rv_op_fmul_d; break;
            case 11: op = rv_op_fmul_q; break;
            case 12: op = rv_op_fdiv_s; break;
            case 13: op = rv_op_fdiv_d; break;
            case 15: op = rv_op_fdiv_q; break;
            case 16: op = ((inst >> 12) & 0b111) == 0 ? rv_op_fsgnj_s :
                          ((inst >> 12) & 0b111) == 1 ? rv_op_fsgnjn_s :
                          ((inst >> 12) & 0b111) == 2 ? rv_op_fsgnjx_s : rv_op_illegal; break;
            case 17: op = ((inst >> 12) & 0b111) == 0 ? rv_op_fsgnj_d :
                          ((inst >> 12) & 0b111) == 1 ? rv_op_fsgnjn_d :
                          ((inst >> 12) & 0b111) == 2 ? rv_op_fsgnjx_d : rv_op_illegal; break;
            case 19: op = ((inst >> 12) & 0b111) == 0 ? rv_op_fsgnj_q :
                          ((inst >> 12) & 0b111) == 1 ? rv_op_fsgnjn_q :
                          ((inst >> 12) & 0b111) == 2 ? rv_op_fsgnjx_q : rv_op_illegal; break;
            case 20: op = ((inst >> 12) & 0b111) == 0 ? rv_op_fmin_s :
                          ((inst >> 12) & 0b111) == 1 ? rv_op_fmax_s : rv_op_illegal; break;
            case 21: op = ((inst >> 12) & 0b111) == 0 ? rv_op_fmin_d :
                          ((inst >> 12) & 0b111) == 1 ? rv_op_fmax_d : rv_op_illegal; break;
            case 23: op = ((inst >> 12) & 0b111) == 0 ? rv_op_fmin_q :
                          ((inst >> 12) & 0b111) == 1 ? rv_op_fmax_q : rv_op_illegal; break;
            case 32: op = ((inst >> 20) & 0b11111) == 0 ? rv_op_fcvt_s_d :
                          ((inst >> 20) & 0b11111) == 3 ? rv_op_fcvt_s_q : rv_op_illegal; break;
            case 33: op = ((inst >> 20) & 0b11111) == 0 ? rv_op_fcvt_d_s :
                          ((inst >> 20) & 0b11111) == 3 ? rv_op_fcvt_d_q : rv_op_illegal; break;
            case 35: op = ((inst >> 20) & 0b11111) == 0 ? rv_op_fcvt_q_s :
                          ((inst >> 20) & 0b11111) == 1 ? rv_op_fcvt_q_d : rv_op_illegal; break;
            case 44: op = ((inst >> 20) & 0b11111) == 0 ? rv_op_fsqrt_s : rv_op_illegal; break;
            case 45: op = ((inst >> 20) & 0b11111) == 0 ? rv_op_fsqrt_d : rv_op_illegal; break;
            case 47: op = ((inst >> 20) & 0b11111) == 0 ? rv_op_fsqrt_q : rv_op_illegal; break;
            case 80: op = ((inst >> 12) & 0b111) == 0 ? rv_op_fle_s :
                          ((inst >> 12) & 0b111) == 1 ? rv_op_flt_s :
                          ((inst >> 12) & 0b111) == 2 ? rv_op_feq_s : rv_op_illegal; break;
            case 81: op = ((inst >> 12) & 0b111) == 0 ? rv_op_fle_d :
                          ((inst >> 12) & 0b111) == 1 ? rv_op_flt_d :
                          ((inst >> 12) & 0b111) == 2 ? rv_op_feq_d : rv_op_illegal; break;
            case 83: op = ((inst >> 12) & 0b111) == 0 ? rv_op_fle_q :
                          ((inst >> 12) & 0b111) == 1 ? rv_op_flt_q :
                          ((inst >> 12) & 0b111) == 2 ? rv_op_feq_q : rv_op_illegal; break;
            case 96:
              switch (((inst >> 20) & 0b11111)) {
                case 0: op = rv_op_fcvt_w_s; break;
                case 1: op = rv_op_fcvt_wu_s; break;
                case 2: op = rv_op_fcvt_l_s; break;
                case 3: op = rv_op_fcvt_lu_s; break;
              } break;
            case 97:
              switch (((inst >> 20) & 0b11111)) {
                case 0: op = rv_op_fcvt_w_d; break;
                case 1: op = rv_op_fcvt_wu_d; break;
                case 2: op = rv_op_fcvt_l_d; break;
                case 3: op = rv_op_fcvt_lu_d; break;
              } break;
            case 99:
              switch (((inst >> 20) & 0b11111)) {
                case 0: op = rv_op_fcvt_w_q; break;
                case 1: op = rv_op_fcvt_wu_q; break;
                case 2: op = rv_op_fcvt_l_q; break;
                case 3: op = rv_op_fcvt_lu_q; break;
              } break;
            case 104:
              switch (((inst >> 20) & 0b11111)) {
                case 0: op = rv_op_fcvt_s_w; break;
                case 1: op = rv_op_fcvt_s_wu; break;
                case 2: op = rv_op_fcvt_s_l; break;
                case 3: op = rv_op_fcvt_s_lu; break;
              } break;
            case 105:
              switch (((inst >> 20) & 0b11111)) {
                case 0: op = rv_op_fcvt_d_w; break;
                case 1: op = rv_op_fcvt_d_wu; break;
                case 2: op = rv_op_fcvt_d_l; break;
                case 3: op = rv_op_fcvt_d_lu; break;
              } break;
            case 107:
              switch (((inst >> 20) & 0b11111)) {
                case 0: op = rv_op_fcvt_q_w; break;
                case 1: op = rv_op_fcvt_q_wu; break;
                case 2: op = rv_op_fcvt_q_l; break;
                case 3: op = rv_op_fcvt_q_lu; break;
              } break;
            case 112: op = (((inst >> 17) & 0b11111000) | ((inst >> 12) & 0b00000111)) == 0 ? rv_op_fmv_x_w :
                           (((inst >> 17) & 0b11111000) | ((inst >> 12) & 0b00000111)) == 1 ? rv_op_fclass_s : rv_op_illegal; break;
            case 113: op = (((inst >> 17) & 0b11111000) | ((inst >> 12) & 0b00000111)) == 0 ? rv_op_fmv_x_d :
                           (((inst >> 17) & 0b11111000) | ((inst >> 12) & 0b00000111)) == 1 ? rv_op_fclass_d : rv_op_illegal; break;
            case 115: op = (((inst >> 17) & 0b11111000) | ((inst >> 12) & 0b00000111)) == 0 ? rv_op_fmv_x_q :
                           (((inst >> 17) & 0b11111000) | ((inst >> 12) & 0b00000111)) == 1 ? rv_op_fclass_q : rv_op_illegal; break;
            case 120: op = (((inst >> 17) & 0b11111000) | ((inst >> 12) & 0b00000111)) == 0 ? rv_op_fmv_w_x : rv_op_illegal; break;
            case 121: op = (((inst >> 17) & 0b11111000) | ((inst >> 12) & 0b00000111)) == 0 ? rv_op_fmv_d_x : rv_op_illegal; break;
            case 123: op = (((inst >> 17) & 0b11111000) | ((inst >> 12) & 0b00000111)) == 0 ? rv_op_fmv_q_x : rv_op_illegal; break;
          } break;
        case 22: /* reserved for OP-IMM-64 */
          switch (((inst >> 12) & 0b111)) {
            case 0: op = rv_op_addid; break;
            case 1: op = ((inst >> 26) & 0b111111) == 0 ? rv_op_sllid : rv_op_illegal; break;
            case 5: op = ((inst >> 26) & 0b111111) == 0 ? rv_op_srlid :
                         ((inst >> 26) & 0b111111) == 16 ? rv_op_sraid : rv_op_illegal; break;
          } break;
        case 24: /* BRANCH funct3 */
          switch (((inst >> 12) & 0b111)) {
            case 0: op = rv_op_beq; break;
            case 1: op = rv_op_bne; break;
            case 4: op = rv_op_blt; break;
            case 5: op = rv_op_bge; break;
            case 6: op = rv_op_bltu; break;
            case 7: op = rv_op_bgeu; break;
          } break;
        case 25: /* JALR */
          op = ((inst >> 12) & 0b111) == 0 ? rv_op_jalr : rv_op_illegal; break;
        case 27: /* JAL */
          op = rv_op_jal; break;
        case 28: /* SYSTEM funct3, funct12 */
          switch (((inst >> 12) & 0b111)) {
            case 0:
              switch (((inst >> 20) & 0b111111100000) | ((inst >> 7) & 0b000000011111)) {
                case 0: op = ((inst >> 15) & 0b1111111111) == 0 ? rv_op_ecall :
                             ((inst >> 15) & 0b1111111111) == 32 ? rv_op_ebreak : rv_op_illegal; break;
                case 256: op = ((inst >> 15) & 0b1111111111) == 64 ? rv_op_sret :
                               ((inst >> 15) & 0b1111111111) == 160 ? rv_op_wfi : rv_op_illegal; break;
                case 288: op = rv_op_sfence_vma; break;
                case 512: op = ((inst >> 15) & 0b1111111111) == 64 ? rv_op_hret : rv_op_illegal; break;
                case 768: op = ((inst >> 15) & 0b1111111111) == 64 ? rv_op_mret : rv_op_illegal; break;
                case 1952: op = ((inst >> 15) & 0b1111111111) == 576 ? rv_op_dret : rv_op_illegal; break;
              } break;
            case 1: op = rv_op_csrrw; break;
            case 2: op = rv_op_csrrs; break;
            case 3: op = rv_op_csrrc; break;
            case 5: op = rv_op_csrrwi; break;
            case 6: op = rv_op_csrrsi; break;
            case 7: op = rv_op_csrrci; break;
          } break;
        case 30: /* reserved for OP-64 */
          switch (((inst >> 22) & 0b1111111000) | ((inst >> 12) & 0b0000000111)) {
            case 0: op = rv_op_addd; break;
            case 1: op = rv_op_slld; break;
            case 5: op = rv_op_srld; break;
            case 8: op = rv_op_muld; break;
            case 12: op = rv_op_divd; break;
            case 13: op = rv_op_divud; break;
            case 14: op = rv_op_remd; break;
            case 15: op = rv_op_remud; break;
            case 256: op = rv_op_subd; break;
            case 261: op = rv_op_srad; break;
          } break;
      } break; /* rv32,rv64 */
  }
  
  dec->op = op;
}

rv_format decode_inst_format(rv_opcode op) {
  switch (op) {
    case rv_op_ecall: case rv_op_ebreak: case rv_op_wfi: case rv_op_sfence_vma:
    case rv_op_sret: case rv_op_hret: case rv_op_mret: case rv_op_dret:
    case rv_op_add: case rv_op_sub: case rv_op_xor: case rv_op_or: case rv_op_and:
    case rv_op_sll: case rv_op_slt: case rv_op_sltu: case rv_op_sra: case rv_op_srl:
    case rv_op_addw: case rv_op_subw: case rv_op_sllw: case rv_op_srlw: case rv_op_sraw:
    case rv_op_addd: case rv_op_subd: case rv_op_slld: case rv_op_srld: case rv_op_srad:
    case rv_op_mul: case rv_op_mulh: case rv_op_mulhsu: case rv_op_mulhu:
    case rv_op_div: case rv_op_divu: case rv_op_rem: case rv_op_remu:
    case rv_op_mulw: case rv_op_divw: case rv_op_divuw: case rv_op_remw: case rv_op_remuw:
    case rv_op_muld: case rv_op_divd: case rv_op_divud: case rv_op_remd: case rv_op_remud:
      return rv_format_r;
    case rv_op_fence:
      return rv_format_r_fence;
    case rv_op_fsgnj_s: case rv_op_fsgnjn_s: case rv_op_fsgnjx_s: case rv_op_fmin_s: case rv_op_fmax_s:
    case rv_op_fle_s: case rv_op_flt_s: case rv_op_feq_s:
    case rv_op_fsgnj_d: case rv_op_fsgnjn_d: case rv_op_fsgnjx_d: case rv_op_fmin_d: case rv_op_fmax_d:
    case rv_op_fle_d: case rv_op_flt_d: case rv_op_feq_d:
    case rv_op_fsgnj_q: case rv_op_fsgnjn_q: case rv_op_fsgnjx_q: case rv_op_fmin_q: case rv_op_fmax_q:
    case rv_op_fle_q: case rv_op_flt_q: case rv_op_feq_q:
      return rv_format_r_float;
    case rv_op_fadd_s: case rv_op_fsub_s: case rv_op_fmul_s: case rv_op_fdiv_s:
    case rv_op_fadd_d: case rv_op_fsub_d: case rv_op_fmul_d: case rv_op_fdiv_d:
    case rv_op_fadd_q: case rv_op_fsub_q: case rv_op_fmul_q: case rv_op_fdiv_q:
      return rv_format_r_float_rm;
    case rv_op_fmv_x_w: case rv_op_fmv_w_x: case rv_op_fclass_s:
    case rv_op_fmv_x_d: case rv_op_fmv_d_x: case rv_op_fclass_d:
    case rv_op_fmv_x_q: case rv_op_fmv_q_x: case rv_op_fclass_q:
      return rv_format_r_float_rs2;
    case rv_op_fsqrt_s: case rv_op_fsqrt_d: case rv_op_fsqrt_q:
    case rv_op_fcvt_w_s: case rv_op_fcvt_wu_s: case rv_op_fcvt_s_w: case rv_op_fcvt_s_wu:
    case rv_op_fcvt_l_s: case rv_op_fcvt_lu_s: case rv_op_fcvt_s_l: case rv_op_fcvt_s_lu:
    case rv_op_fcvt_s_d: case rv_op_fcvt_d_s:
    case rv_op_fcvt_w_d: case rv_op_fcvt_wu_d: case rv_op_fcvt_d_w: case rv_op_fcvt_d_wu:
    case rv_op_fcvt_l_d: case rv_op_fcvt_lu_d: case rv_op_fcvt_d_l: case rv_op_fcvt_d_lu:
    case rv_op_fcvt_s_q: case rv_op_fcvt_q_s: case rv_op_fcvt_d_q: case rv_op_fcvt_q_d:
    case rv_op_fcvt_w_q: case rv_op_fcvt_wu_q: case rv_op_fcvt_q_w: case rv_op_fcvt_q_wu:
    case rv_op_fcvt_l_q: case rv_op_fcvt_lu_q: case rv_op_fcvt_q_l: case rv_op_fcvt_q_lu:
      return rv_format_r_float_rm_rs2;
    case rv_op_lr_w: case rv_op_lr_d: case rv_op_lr_q:
    case rv_op_sc_w: case rv_op_sc_d: case rv_op_sc_q:
    case rv_op_amoswap_w: case rv_op_amoadd_w: case rv_op_amoxor_w: case rv_op_amoor_w: case rv_op_amoand_w:
    case rv_op_amomin_w: case rv_op_amomax_w: case rv_op_amominu_w: case rv_op_amomaxu_w:
    case rv_op_amoswap_d: case rv_op_amoadd_d: case rv_op_amoxor_d: case rv_op_amoor_d: case rv_op_amoand_d:
    case rv_op_amomin_d: case rv_op_amomax_d: case rv_op_amominu_d: case rv_op_amomaxu_d:
    case rv_op_amoswap_q: case rv_op_amoadd_q: case rv_op_amoxor_q: case rv_op_amoor_q: case rv_op_amoand_q:
    case rv_op_amomin_q: case rv_op_amomax_q: case rv_op_amominu_q: case rv_op_amomaxu_q:
      return rv_format_r_amo;
    case rv_op_fmadd_s: case rv_op_fmsub_s: case rv_op_fnmsub_s: case rv_op_fnmadd_s:
    case rv_op_fmadd_d: case rv_op_fmsub_d: case rv_op_fnmsub_d: case rv_op_fnmadd_d:
    case rv_op_fmadd_q: case rv_op_fmsub_q: case rv_op_fnmsub_q: case rv_op_fnmadd_q:
      return rv_format_r4_rm;
    case rv_op_lui: case rv_op_auipc:
      return rv_format_u;
    case rv_op_jal:
      return rv_format_j;
    case rv_op_lb: case rv_op_lh: case rv_op_lw: case rv_op_lbu: case rv_op_lhu:
    case rv_op_lwu: case rv_op_ld: case rv_op_ldu: case rv_op_lq:
    case rv_op_flw: case rv_op_fld: case rv_op_flq:
    case rv_op_addi: case rv_op_slti: case rv_op_sltiu: case rv_op_xori: case rv_op_ori: case rv_op_andi:
    case rv_op_jalr: case rv_op_fence_i:
    case rv_op_addiw: case rv_op_addid:
      return rv_format_i;
    case rv_op_slli: case rv_op_slliw: case rv_op_sllid:
    case rv_op_srli: case rv_op_srliw: case rv_op_srlid:
    case rv_op_srai: case rv_op_sraiw: case rv_op_sraid:
      return rv_format_i_shamt;
    case rv_op_csrrw: case rv_op_csrrs: case rv_op_csrrc:
    case rv_op_csrrwi: case rv_op_csrrsi: case rv_op_csrrci:
      return rv_format_i_csr;
    case rv_op_sb: case rv_op_sh: case rv_op_sw: case rv_op_sd: case rv_op_sq:
    case rv_op_fsw: case rv_op_fsd: case rv_op_fsq:
      return rv_format_s;
    case rv_op_beq: case rv_op_bne: case rv_op_blt: case rv_op_bge: case rv_op_bltu: case rv_op_bgeu:
      return rv_format_b;

    case rv_op_c_lw: case rv_op_c_flw: case rv_op_c_sw: case rv_op_c_fsw:
      return rv_format_cls_w;
    case rv_op_c_ld: case rv_op_c_fld: case rv_op_c_sd: case rv_op_c_fsd:
      return rv_format_cls_d;
    case rv_op_c_lq: case rv_op_c_sq:
      return rv_format_cls_q;
    case rv_op_c_addi4spn:
      return rv_format_ciw_addi4spn;
    case rv_op_c_nop: case rv_op_c_addi: case rv_op_c_addiw: case rv_op_c_li:
      return rv_format_ci;
    case rv_op_c_addi16sp:
      return rv_format_ci_addi16sp;
    case rv_op_c_lui:
      return rv_format_ci_lui;
    case rv_op_c_slli:
      return rv_format_ci_shamt;
    case rv_op_c_srli: case rv_op_c_srai:
      return rv_format_cb_shamt;
    case rv_op_c_andi:
      return rv_format_cb_andi;
    case rv_op_c_sub: case rv_op_c_xor: case rv_op_c_or: case rv_op_c_and: case rv_op_c_subw: case rv_op_c_addw:
      return rv_format_ca;
    case rv_op_c_jal: case rv_op_c_j:
      return rv_format_cj;
    case rv_op_c_beqz: case rv_op_c_bnez:
      return rv_format_cb;
    case rv_op_c_lwsp: case rv_op_c_flwsp:
      return rv_format_ci_lsp_w;
    case rv_op_c_ldsp: case rv_op_c_fldsp:
      return rv_format_ci_lsp_d;
    case rv_op_c_lqsp:
      return rv_format_ci_lsp_q;
    case rv_op_c_swsp: case rv_op_c_fswsp:
      return rv_format_css_w;
    case rv_op_c_sdsp: case rv_op_c_fsdsp:
      return rv_format_css_d;
    case rv_op_c_sqsp:
      return rv_format_css_q;
    case rv_op_c_jr: case rv_op_c_jalr: case rv_op_c_add: case rv_op_c_mv: case rv_op_c_ebreak:
      return rv_format_cr;
    default:
      return rv_format_r;
  }
}

void decode_inst_oprand(masker_inst_t* dec) {
  rv_inst inst = dec->inst;
  switch (decode_inst_format(dec->op)) {
    case rv_format_r: dec->args.assign({&rd, &funct3, &rs1, &rs2, &funct7}); break;
    case rv_format_r_fence: dec->args.assign({&rd, &funct3, &rs1, &succ, &pred, &fm}); break;
    case rv_format_r_float: case rv_format_r_float_rs2:
      dec->args.assign({&rd, &funct3, &rs1, &rs2, &funct2, &funct5}); break;
    case rv_format_r_float_rm: case rv_format_r_float_rm_rs2:
      dec->args.assign({&rd, &rm, &rs1, &rs2, &funct2, &funct5}); break;
    case rv_format_r_amo: dec->args.assign({&rd, &funct3, &rs1, &rs2, &rl, &aq, &funct5}); break;
    case rv_format_r4_rm: dec->args.assign({&rd, &rm, &rs1, &rs2, &funct2, &rs3}); break;
    case rv_format_u: dec->args.assign({&rd, &imm_u}); break;
    case rv_format_j: dec->args.assign({&rd, &imm_j}); break;
    case rv_format_i: dec->args.assign({&rd, &funct3, &rs1, &imm_i}); break;
    case rv_format_i_shamt:
      switch (dec->op) {
        case rv_op_slli: case rv_op_srli: case rv_op_srai:
          shamt_imm.set_xlen(dec->xlen); shamt_funct.set_xlen(dec->xlen); break;
        case rv_op_slliw: case rv_op_srliw: case rv_op_sraiw:
          shamt_imm.set_xlen(rv32); shamt_funct.set_xlen(rv32); break;
        case rv_op_sllid: case rv_op_srlid: case rv_op_sraid:
          shamt_imm.set_xlen(rv64); shamt_funct.set_xlen(rv64); break; 
        default: exit(10025);
        }
      dec->args.assign({&rd, &funct3, &rs1, &shamt_imm, &shamt_funct}); break;
    case rv_format_i_csr: dec->args.assign({&rd, &funct3, &rs1, &csr}); break;
    case rv_format_s: dec->args.assign({&funct3, &rs1, &rs2, &imm_s}); break;
    case rv_format_b: dec->args.assign({&funct3, &rs1, &rs2, &imm_b}); break;

    case rv_format_cls_w: dec->args.assign({&c_rs2p, &c_rs1p, &cls_uimm6, &c_funct3}); break;
    case rv_format_cls_d: dec->args.assign({&c_rs2p, &c_rs1p, &cls_uimm7, &c_funct3}); break;
    case rv_format_cls_q: dec->args.assign({&c_rs2p, &c_rs1p, &cls_uimm8, &c_funct3}); break;
    case rv_format_ciw_addi4spn: dec->args.assign({&c_rs2p, &ciw_uimm9, &c_funct3}); break;
    case rv_format_ci: dec->args.assign({&ci_imm5, &c_rs1, &c_funct3}); break;
    case rv_format_ci_addi16sp: dec->args.assign({&ci_imm9, &c_rs1, &c_funct3}); break;
    case rv_format_ci_lui: dec->args.assign({&ci_imm17, &c_rs1, &c_funct3}); break;
    case rv_format_ci_shamt: dec->args.assign({&ci_uimm5, &c_rs1, &c_funct3}); break;
    case rv_format_cb_shamt: dec->args.assign({&ci_uimm5, &c_rs1p, &c_funct_rs1, &c_funct3}); break;
    case rv_format_cb_andi: dec->args.assign({&ci_imm5, &c_rs1p, &c_funct_rs1, &c_funct3}); break;
    case rv_format_ca: dec->args.assign({&c_rs2p, &c_funct_rs2, &c_rs1p, &c_funct_rs1, &c_funct1, &c_funct3}); break;
    case rv_format_cj: dec->args.assign({&cj_imm11, &c_funct3}); break;
    case rv_format_cb: dec->args.assign({&cb_imm8, &c_rs1p, &c_funct3}); break;
    case rv_format_ci_lsp_w: dec->args.assign({&ci_uimm7, &c_rs1, &c_funct3}); break;
    case rv_format_ci_lsp_d: dec->args.assign({&ci_uimm8, &c_rs1, &c_funct3}); break;
    case rv_format_ci_lsp_q: dec->args.assign({&ci_uimm9, &c_rs1, &c_funct3}); break;
    case rv_format_css_w: dec->args.assign({&c_rs2, &css_uimm7, &c_funct3}); break;
    case rv_format_css_d: dec->args.assign({&c_rs2, &css_uimm8, &c_funct3}); break;
    case rv_format_css_q: dec->args.assign({&c_rs2, &css_uimm9, &c_funct3}); break;
    case rv_format_cr: dec->args.assign({&c_rs2, &c_rs1, &c_funct1, &c_funct3}); break;
    default:
      exit(10010);
  };

  dec->decode();
}


rv_inst masker_inst_t::bare_op() {
  rv_inst op;
  if ((inst & OP_MASK) == OP_32) {
    op = inst & 0x7f;
  } else {
    op = inst & OP_MASK;
  }
  return op;
}

rv_inst masker_inst_t::mutation(bool debug) {
  if (history.find(rdm_entry(pc, inst)) != history.end())  // already mutated
    return replay_mutation(debug);
  
  decode_inst_oprand(this);

  static std::vector<rv_inst> amo_list = {0x2, 0x3, 0x1, 0x0, 0x4, 0xc, 0x8, 0x10, 0x14, 0x18, 0x1c};
  rv_inst bop = bare_op();
  int type;

  if (debug)
    printf("\e[1;35m[CJ] insn mutation:  %s @ %08lx\n", rv_opcode_name[op], inst);
  for (auto arg : args) {
    if (debug) {
      printf("\e[1;35m[CJ] insn mutation:  \t%s @ %08lx ->", rv_field_name[arg->name], arg->value);
    }

    switch (arg->name) {
      // functions
      case rv_field_funct3:
        // b type
        switch (bop) {
          case 0x63:  // 1100011  branch
            arg->value = randBits(3);
            break;
          case 0x03:  // 0000011  load
            arg->value = randInt(0, 7);   // 7 is illegal
            break;
          case 0x23:  // 0100011  store
            arg->value = randInt(0, 4);  // 4 is illegal
            break;
          case 0x13:  // 0010011  alu_i
            arg->value = randBits(3);
            break;
          case 0x33:  // 0110011  alu_r
            arg->value = randBits(3);
            break;
          case 0x1b:  // 0011011  alu_iw
            arg->value = randBits(3);
            break;
          case 0x3b:  // 0111011  alu_rw
            arg->value = randBits(3);
            break;
          case 0x53:  // 1010011  f
            arg->value = randBits(2);
            break;

        }
        break;

      case rv_field_funct7:
        switch (bop) {
          case 0x2f:  // 0101111 amo
            arg->value = amo_list[randInt(0, amo_list.size()-1)];
        }
        break;

      case rv_field_funct5:
      case rv_field_funct2:
      case rv_field_c_funct3:
      case rv_field_c_funct1:
        // do nothing
        break;

      // registers
      case rv_field_rd:
        if (randBits(3) == 0) {  // WAW
          arg->value = random_rd_in_pipeline();
        } else {
          arg->value = randBits(5);
        }
        break;

      case rv_field_rs1:
      case rv_field_rs2:
      case rv_field_rs3:
      case rv_field_c_rs1:
      case rv_field_c_rs2:
        type = randBits(3);
        if (type == 0) {  // Rand
          arg->value = randBits(5);
        } else if (type <= 2) { // RAW
          arg->value = random_rd_in_pipeline();
        } else {  // rd_with_type
          magic_type *t;
          switch (op) {
            case rv_op_lb: case rv_op_lh: case rv_op_lw: case rv_op_lbu: case rv_op_lhu:case rv_op_lwu: case rv_op_ld: case rv_op_ldu: case rv_op_lq: 
            case rv_op_lr_w: case rv_op_lr_d: case rv_op_lr_q:
            case rv_op_flw: case rv_op_fld: case rv_op_flq:
            case rv_op_c_lw: case rv_op_c_flw: case rv_op_c_ld: case rv_op_c_fld: case rv_op_c_lq:
            case rv_op_c_lwsp: case rv_op_c_flwsp: case rv_op_c_ldsp: case rv_op_c_fldsp: case rv_op_c_lqsp:
              t = &magic_r_address; break;
            case rv_op_sd: case rv_op_sq: case rv_op_sb: case rv_op_sh: case rv_op_sw: 
            case rv_op_sc_w: case rv_op_sc_d: case rv_op_sc_q:
            case rv_op_fsw: case rv_op_fsd: case rv_op_fsq:
            case rv_op_c_sw:case rv_op_c_fsw: case rv_op_c_sd: case rv_op_c_fsd: case rv_op_c_sq:
            case rv_op_c_swsp: case rv_op_c_fswsp: case rv_op_c_sdsp: case rv_op_c_fsdsp: case rv_op_c_sqsp: 
            case rv_op_amoswap_w: case rv_op_amoadd_w: case rv_op_amoxor_w: case rv_op_amoor_w: case rv_op_amoand_w: case rv_op_amomin_w: case rv_op_amomax_w: case rv_op_amominu_w: case rv_op_amomaxu_w:
            case rv_op_amoswap_d: case rv_op_amoadd_d: case rv_op_amoxor_d: case rv_op_amoor_d: case rv_op_amoand_d:case rv_op_amomin_d: case rv_op_amomax_d: case rv_op_amominu_d: case rv_op_amomaxu_d:
            case rv_op_amoswap_q: case rv_op_amoadd_q: case rv_op_amoxor_q: case rv_op_amoor_q: case rv_op_amoand_q:case rv_op_amomin_q: case rv_op_amomax_q: case rv_op_amominu_q: case rv_op_amomaxu_q:
              t = &magic_w_address; break;
            case rv_op_jalr: case rv_op_c_jalr:
              t = &magic_x_address;
            default:
              t = &magic_void;
          }

          arg->value = rd_with_type(t);
        }
        break;

      case rv_field_c_rs1p:
      case rv_field_c_rs2p:
        arg->value = randBits(3);
        break;

      // fence
      case rv_field_fm:
        if (rand2(random)) { // legal
          if (rand2(random)) // flip
            arg->value ^= 0x8;
        }
        else           // illegal
          arg->value = randBits(4);
        break;

      // fence RWIO
      case rv_field_pred:
      case rv_field_succ:
        arg->value ^= randBits(4);
        break;

      // FCVT
      case rv_field_rm:
        switch(op) {
          default:
          if (rand2(random))
            arg->value = randBits(3);
        }
        break;

      // AMO
      case rv_field_aq:
      case rv_field_rl:
        if (rand2(random))
          arg->value = !arg->value;
        break;
      
      case rv_field_imm_u:
        arg->value = randBits(20) << 12;
        break;
      case rv_field_imm_j:
        arg->value = simulator->get_random_text_address(random) - pc;
        break;
      case rv_field_imm_i:
        if (op == rv_op_jalr) {
          arg->value = 0;
        } else {
          arg->value = randBits(12);
        }
        break;
      case rv_field_imm_s:
          arg->value = randBits(12);
        break;
      
      // imm_b is used for branch instruction
      // so should be given a valid exec addr
      case rv_field_imm_b:
        arg->value = simulator->get_random_text_address(random) - pc;
        break;

      case rv_field_cls_uimm6:
        arg->value = randBits(5) << 2;
        break;
      case rv_field_cls_uimm7:
        arg->value = randBits(5) << 3;
        break;
      case rv_field_cls_uimm8:
        arg->value = randBits(5) << 4;
        break;
      case rv_field_ciw_uimm9:
        arg->value = randBits(8) << 2;
        break;
      case rv_field_cb_imm8:
        arg->value = randBits(8) << 1;
        break;
      case rv_field_cj_imm11:
        arg->value = randBits(11) << 1;
        break;
      case rv_field_ci_imm5:
        arg->value = randBits(6);
        break;
      case rv_field_ci_imm9:
        arg->value = randBits(6) << 4;
        break;
      case rv_field_ci_imm17:
        arg->value = randBits(6) << 12;
        break;
      case rv_field_ci_uimm5:
        arg->value = randBits(6);
        break;
      case rv_field_ci_uimm7:
        arg->value = randBits(6) << 2;
        break;
      case rv_field_ci_uimm8:
        arg->value = randBits(6) << 3;
        break;
      case rv_field_ci_uimm9:
        arg->value = randBits(6) << 4;
        break;
      case rv_field_css_uimm7:
        arg->value = randBits(6) << 2;
        break;
      case rv_field_css_uimm8:
        arg->value = randBits(6) << 3;
        break;
      case rv_field_css_uimm9:
        arg->value = randBits(6) << 4;
        break;

      case rv_field_csr:
        // do nothing
        break;

      case rv_field_shamt_imm:
        arg->value = randBits(5);
        break;
      case rv_field_shamt_funct:
        arg->value = randBits(1) << 5;
        break;

      default:
        break;
    }

    if (debug) {
      printf(" %08lx\e[0m\n", arg->value);
    }
  }

  return encode(debug);
}

rv_inst masker_inst_t::encode(bool debug) {
    rv_inst new_inst;
    // if (history.find(rdm_entry(pc, inst)) != history.end())
    //   return replay_mutation(debug);

    if ((inst & OP_MASK) == OP_32)
      new_inst = inst & 0x7f;
    else
      new_inst = inst & OP_MASK;

    for (auto arg : args)
      new_inst = arg->encode(new_inst);

    if (debug) {
      masker_inst_t tmp(new_inst, rv64, pc);
      decode_inst_opcode(&tmp);
      printf("\e[1;35m[CJ] Insn mutation:  %016lx @ %08lx -> %08lx [%s] \e[0m\n", pc, inst, new_inst, rv_opcode_name[tmp.op]);
    }

    // inst = new_inst;
    return new_inst;
  }

rv_inst masker_inst_t::replay_mutation(bool debug) {
  rv_inst new_inst;
  if (history.find(rdm_entry(pc, inst)) != history.end()) {
    new_inst = history[rdm_entry(pc, inst)];
  } else {
    // do not directly use the return 0 for the non-exsist key
    if (debug) printf("\e[1;33m[CJ] Non-exsist key, maybe zero maybe something wrong \e[0m\n");
    new_inst = 0x0;
  }

  if (debug) {
    masker_inst_t tmp(new_inst, rv64, pc);
    decode_inst_opcode(&tmp);
    printf("\e[1;33m[CJ] Replay insn mutation:  %016lx @ %08lx -> %08lx [%s] \e[0m\n", pc, inst, new_inst, rv_opcode_name[tmp.op]);
  }
  return new_inst;
}

uint64_t masker_inst_t::randInt(uint64_t a, uint64_t b) {
  std::uniform_int_distribution<uint64_t> r(a, b);
  return r(random);
}

uint64_t masker_inst_t::randBits(uint64_t w) {
  std::uniform_int_distribution<uint64_t> r(0, w == 64 ? -1LL : (1LL << w) - 1);
  return r(random);
}

int masker_inst_t::random_rd_in_pipeline() {
  return rd_in_pipeline[randInt(0, rd_in_pipeline.size())];
}

int masker_inst_t::rd_with_type(magic_type *t) {
  std::vector<int> buf;
  for (int i = 31; i > 0; i--) {
    if (type[i]->is_child_of(t)) {
      buf.push_back(i);
    }
  }
  int n = buf.size();
  if (t != &magic_void)
    simulator->record_rd_mutation_stats(n);

  if (n > 0) {
    std::uniform_int_distribution<uint64_t> r(0, n-1);
    return buf[r(random)];
  }
  else {
    return randBits(5);
  }
}

void masker_inst_t::record_to_history(rv_inst new_inst) {
  history[rdm_entry(pc, inst)] = new_inst;
  
  int rd = (new_inst << 52) >> 59;
  int rs1 = (new_inst << 44) >> 59;
  int64_t imm = ((int64_t)new_inst << 32) >> 52;

  // save type
  if (rd != 0) {
    if (hint_insn(new_inst) == magic_ops && 0 <= imm) {
     size_t index = imm / 8;
      if (index < magic_generator_type.size())
        type[rd] = magic_generator_type[index];
      else
        type[rd] = &magic_void;
    } else if (op == rv_op_jal || op == rv_op_jalr) {  // jump
      type[rd] = &magic_text_address;
    } else {
      type[rd] = &magic_void;
    }
  }

  // save rd
  rd_in_pipeline.push(rd);
  rd_in_pipeline.pop();
}

void masker_inst_t::reset_mutation_history() {
  history.clear();

  rd_in_pipeline.clear();

  type[0] = &magic_zero;
  for (int i = 1; i <= 31; i++)
    type[i] = &magic_void;

  for (int i = 0; i < 5; i++) {
    rd_in_pipeline.push(0);
  }
}

void masker_inst_t::fence_mutation() {
  history.clear();
}

magic_type::magic_type(const std::string &name, std::vector<magic_type*> parents) :
  name(name),
  parents(parents)
{
  for (auto &p : parents)
    p->children.push_back(this);
}

bool magic_type::is_child_of(magic_type *b) {
  std::queue<magic_type*> q;
  q.push(this);
  while (!q.empty()) {
    auto a = q.front(); q.pop();
    if (a == b) return true;
    for (auto &p : a->parents)
      q.push(p);
  }
  return false;
}

magic_type magic_void("void");
magic_type magic_int("int", {&magic_void});
magic_type magic_float("float", {&magic_void});
magic_type magic_zero("zero", {&magic_int, &magic_float});
magic_type magic_r_address("address", {&magic_void});
magic_type magic_w_address("address", {&magic_void});
magic_type magic_x_address("address", {&magic_void});
magic_type magic_text_address("text address", {&magic_r_address, &magic_x_address});
magic_type magic_data_address("data address", {&magic_r_address, &magic_w_address});


