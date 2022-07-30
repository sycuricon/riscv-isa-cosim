#ifndef _COSIM_MASKER_ENUM_H
#define _COSIM_MASKER_ENUM_H

static const char* rv_xlen_name[] = {"rv16", "rv32", "rv64", "rv128"};

static const char* rv_opcode_name[] = {"rv_op_illegal", "rv_op_lui", "rv_op_auipc", "rv_op_jal", "rv_op_jalr", "rv_op_beq", "rv_op_bne", "rv_op_blt", "rv_op_bge", "rv_op_bltu", "rv_op_bgeu", "rv_op_lb", "rv_op_lh", "rv_op_lw", "rv_op_lbu", "rv_op_lhu", "rv_op_sb", "rv_op_sh", "rv_op_sw", "rv_op_addi", "rv_op_slti", "rv_op_sltiu", "rv_op_xori", "rv_op_ori", "rv_op_andi", "rv_op_slli", "rv_op_srli", "rv_op_srai", "rv_op_add", "rv_op_sub", "rv_op_sll", "rv_op_slt", "rv_op_sltu", "rv_op_xor", "rv_op_srl", "rv_op_sra", "rv_op_or", "rv_op_and", "rv_op_fence", "rv_op_fence_i", "rv_op_lwu", "rv_op_ld", "rv_op_sd", "rv_op_addiw", "rv_op_slliw", "rv_op_srliw", "rv_op_sraiw", "rv_op_addw", "rv_op_subw", "rv_op_sllw", "rv_op_srlw", "rv_op_sraw", "rv_op_ldu", "rv_op_lq", "rv_op_sq", "rv_op_addid", "rv_op_sllid", "rv_op_srlid", "rv_op_sraid", "rv_op_addd", "rv_op_subd", "rv_op_slld", "rv_op_srld", "rv_op_srad", "rv_op_mul", "rv_op_mulh", "rv_op_mulhsu", "rv_op_mulhu", "rv_op_div", "rv_op_divu", "rv_op_rem", "rv_op_remu", "rv_op_mulw", "rv_op_divw", "rv_op_divuw", "rv_op_remw", "rv_op_remuw", "rv_op_muld", "rv_op_divd", "rv_op_divud", "rv_op_remd", "rv_op_remud", "rv_op_lr_w", "rv_op_sc_w", "rv_op_amoswap_w", "rv_op_amoadd_w", "rv_op_amoxor_w", "rv_op_amoor_w", "rv_op_amoand_w", "rv_op_amomin_w", "rv_op_amomax_w", "rv_op_amominu_w", "rv_op_amomaxu_w", "rv_op_lr_d", "rv_op_sc_d", "rv_op_amoswap_d", "rv_op_amoadd_d", "rv_op_amoxor_d", "rv_op_amoor_d", "rv_op_amoand_d", "rv_op_amomin_d", "rv_op_amomax_d", "rv_op_amominu_d", "rv_op_amomaxu_d", "rv_op_lr_q", "rv_op_sc_q", "rv_op_amoswap_q", "rv_op_amoadd_q", "rv_op_amoxor_q", "rv_op_amoor_q", "rv_op_amoand_q", "rv_op_amomin_q", "rv_op_amomax_q", "rv_op_amominu_q", "rv_op_amomaxu_q", "rv_op_ecall", "rv_op_ebreak", "rv_op_sret", "rv_op_hret", "rv_op_mret", "rv_op_dret", "rv_op_sfence_vma", "rv_op_wfi", "rv_op_csrrw", "rv_op_csrrs", "rv_op_csrrc", "rv_op_csrrwi", "rv_op_csrrsi", "rv_op_csrrci", "rv_op_flw", "rv_op_fsw", "rv_op_fmadd_s", "rv_op_fmsub_s", "rv_op_fnmsub_s", "rv_op_fnmadd_s", "rv_op_fadd_s", "rv_op_fsub_s", "rv_op_fmul_s", "rv_op_fdiv_s", "rv_op_fsgnj_s", "rv_op_fsgnjn_s", "rv_op_fsgnjx_s", "rv_op_fmin_s", "rv_op_fmax_s", "rv_op_fsqrt_s", "rv_op_fle_s", "rv_op_flt_s", "rv_op_feq_s", "rv_op_fcvt_w_s", "rv_op_fcvt_wu_s", "rv_op_fcvt_s_w", "rv_op_fcvt_s_wu", "rv_op_fmv_x_w", "rv_op_fclass_s", "rv_op_fmv_w_x", "rv_op_fcvt_l_s", "rv_op_fcvt_lu_s", "rv_op_fcvt_s_l", "rv_op_fcvt_s_lu", "rv_op_fld", "rv_op_fsd", "rv_op_fmadd_d", "rv_op_fmsub_d", "rv_op_fnmsub_d", "rv_op_fnmadd_d", "rv_op_fadd_d", "rv_op_fsub_d", "rv_op_fmul_d", "rv_op_fdiv_d", "rv_op_fsgnj_d", "rv_op_fsgnjn_d", "rv_op_fsgnjx_d", "rv_op_fmin_d", "rv_op_fmax_d", "rv_op_fcvt_s_d", "rv_op_fcvt_d_s", "rv_op_fsqrt_d", "rv_op_fle_d", "rv_op_flt_d", "rv_op_feq_d", "rv_op_fcvt_w_d", "rv_op_fcvt_wu_d", "rv_op_fcvt_d_w", "rv_op_fcvt_d_wu", "rv_op_fclass_d", "rv_op_fcvt_l_d", "rv_op_fcvt_lu_d", "rv_op_fmv_x_d", "rv_op_fcvt_d_l", "rv_op_fcvt_d_lu", "rv_op_fmv_d_x", "rv_op_flq", "rv_op_fsq", "rv_op_fmadd_q", "rv_op_fmsub_q", "rv_op_fnmsub_q", "rv_op_fnmadd_q", "rv_op_fadd_q", "rv_op_fsub_q", "rv_op_fmul_q", "rv_op_fdiv_q", "rv_op_fsgnj_q", "rv_op_fsgnjn_q", "rv_op_fsgnjx_q", "rv_op_fmin_q", "rv_op_fmax_q", "rv_op_fcvt_s_q", "rv_op_fcvt_q_s", "rv_op_fcvt_d_q", "rv_op_fcvt_q_d", "rv_op_fsqrt_q", "rv_op_fle_q", "rv_op_flt_q", "rv_op_feq_q", "rv_op_fcvt_w_q", "rv_op_fcvt_wu_q", "rv_op_fcvt_q_w", "rv_op_fcvt_q_wu", "rv_op_fclass_q", "rv_op_fcvt_l_q", "rv_op_fcvt_lu_q", "rv_op_fcvt_q_l", "rv_op_fcvt_q_lu", "rv_op_fmv_x_q", "rv_op_fmv_q_x", "rv_op_c_addi4spn", "rv_op_c_fld", "rv_op_c_lw", "rv_op_c_flw", "rv_op_c_fsd", "rv_op_c_sw", "rv_op_c_fsw", "rv_op_c_nop", "rv_op_c_addi", "rv_op_c_jal", "rv_op_c_li", "rv_op_c_addi16sp", "rv_op_c_lui", "rv_op_c_srli", "rv_op_c_srai", "rv_op_c_andi", "rv_op_c_sub", "rv_op_c_xor", "rv_op_c_or", "rv_op_c_and", "rv_op_c_subw", "rv_op_c_addw", "rv_op_c_j", "rv_op_c_beqz", "rv_op_c_bnez", "rv_op_c_slli", "rv_op_c_fldsp", "rv_op_c_lwsp", "rv_op_c_flwsp", "rv_op_c_jr", "rv_op_c_mv", "rv_op_c_ebreak", "rv_op_c_jalr", "rv_op_c_add", "rv_op_c_fsdsp", "rv_op_c_swsp", "rv_op_c_fswsp", "rv_op_c_ld", "rv_op_c_sd", "rv_op_c_addiw", "rv_op_c_ldsp", "rv_op_c_sdsp", "rv_op_c_lq", "rv_op_c_sq", "rv_op_c_lqsp", "rv_op_c_sqsp"};

static const char* rv_format_name[] = {"rv_format_r", "rv_format_r_fence", "rv_format_r_float", "rv_format_r_float_rm", "rv_format_r_float_rs2", "rv_format_r_float_rm_rs2", "rv_format_r_amo", "rv_format_r4_rm", "rv_format_u", "rv_format_j", "rv_format_i", "rv_format_i_shamt", "rv_format_i_csr", "rv_format_s", "rv_format_b", "rv_format_cls_w", "rv_format_cls_d", "rv_format_cls_q", "rv_format_ciw_addi4spn", "rv_format_ci", "rv_format_ci_addi16sp", "rv_format_ci_lui", "rv_format_ci_shamt", "rv_format_ci_lsp_w", "rv_format_ci_lsp_d", "rv_format_ci_lsp_q", "rv_format_cb", "rv_format_cb_shamt", "rv_format_cb_andi", "rv_format_ca", "rv_format_cj", "rv_format_css_w", "rv_format_css_d", "rv_format_css_q", "rv_format_cr"};

static const char* rv_field_name[] = {"rv_field_funct3", "rv_field_funct7", "rv_field_funct5", "rv_field_funct2", "rv_field_rd", "rv_field_rs1", "rv_field_rs2", "rv_field_rs3", "rv_field_fm", "rv_field_pred", "rv_field_succ", "rv_field_rm", "rv_field_aq", "rv_field_rl", "rv_field_imm_u", "rv_field_imm_j", "rv_field_imm_i", "rv_field_shamt_imm", "rv_field_shamt_funct", "rv_field_csr", "rv_field_imm_s", "rv_field_imm_b", "rv_field_c_funct3", "rv_field_c_funct1", "rv_field_c_funct_rs1", "rv_field_c_funct_rs2", "rv_field_c_rs1", "rv_field_c_rs2", "rv_field_c_rs1p", "rv_field_c_rs2p", "rv_field_cls_uimm6", "rv_field_cls_uimm7", "rv_field_cls_uimm8", "rv_field_ciw_uimm9", "rv_field_cb_imm8", "rv_field_cj_imm11", "rv_field_ci_imm5", "rv_field_ci_imm9", "rv_field_ci_imm17", "rv_field_ci_uimm5", "rv_field_ci_uimm7", "rv_field_ci_uimm8", "rv_field_ci_uimm9", "rv_field_css_uimm7", "rv_field_css_uimm8", "rv_field_css_uimm9"};

typedef enum {
  rv16, rv32, rv64, rv128
} rv_xlen;

typedef enum {
  rv_op_illegal,
  rv_op_lui,
  rv_op_auipc,
  rv_op_jal,
  rv_op_jalr,
  rv_op_beq, rv_op_bne, rv_op_blt, rv_op_bge, rv_op_bltu, rv_op_bgeu,
  rv_op_lb, rv_op_lh, rv_op_lw, rv_op_lbu, rv_op_lhu,
  rv_op_sb, rv_op_sh, rv_op_sw,
  rv_op_addi, rv_op_slti, rv_op_sltiu, rv_op_xori, rv_op_ori, rv_op_andi, rv_op_slli, rv_op_srli, rv_op_srai,
  rv_op_add, rv_op_sub, rv_op_sll, rv_op_slt, rv_op_sltu, rv_op_xor, rv_op_srl, rv_op_sra, rv_op_or, rv_op_and,
  rv_op_fence, rv_op_fence_i,
  rv_op_lwu, rv_op_ld, rv_op_sd,
  rv_op_addiw, rv_op_slliw, rv_op_srliw, rv_op_sraiw,
  rv_op_addw, rv_op_subw, rv_op_sllw, rv_op_srlw, rv_op_sraw,
  rv_op_ldu, rv_op_lq, rv_op_sq,
  rv_op_addid, rv_op_sllid, rv_op_srlid, rv_op_sraid,
  rv_op_addd, rv_op_subd, rv_op_slld, rv_op_srld, rv_op_srad,
  rv_op_mul, rv_op_mulh, rv_op_mulhsu, rv_op_mulhu,
  rv_op_div, rv_op_divu, rv_op_rem, rv_op_remu,
  rv_op_mulw, rv_op_divw, rv_op_divuw, rv_op_remw, rv_op_remuw,
  rv_op_muld, rv_op_divd, rv_op_divud, rv_op_remd, rv_op_remud,
  rv_op_lr_w, rv_op_sc_w,
  rv_op_amoswap_w, rv_op_amoadd_w, rv_op_amoxor_w, rv_op_amoor_w, rv_op_amoand_w,
  rv_op_amomin_w, rv_op_amomax_w, rv_op_amominu_w, rv_op_amomaxu_w,
  rv_op_lr_d, rv_op_sc_d,
  rv_op_amoswap_d, rv_op_amoadd_d, rv_op_amoxor_d, rv_op_amoor_d, rv_op_amoand_d,
  rv_op_amomin_d, rv_op_amomax_d, rv_op_amominu_d, rv_op_amomaxu_d,
  rv_op_lr_q, rv_op_sc_q,
  rv_op_amoswap_q, rv_op_amoadd_q, rv_op_amoxor_q, rv_op_amoor_q, rv_op_amoand_q,
  rv_op_amomin_q, rv_op_amomax_q, rv_op_amominu_q, rv_op_amomaxu_q,
  rv_op_ecall, rv_op_ebreak,
  rv_op_sret, rv_op_hret, rv_op_mret, rv_op_dret,
  rv_op_sfence_vma, rv_op_wfi,
  rv_op_csrrw, rv_op_csrrs, rv_op_csrrc, rv_op_csrrwi, rv_op_csrrsi, rv_op_csrrci,
  rv_op_flw, rv_op_fsw,
  rv_op_fmadd_s, rv_op_fmsub_s, rv_op_fnmsub_s, rv_op_fnmadd_s,
  rv_op_fadd_s, rv_op_fsub_s, rv_op_fmul_s, rv_op_fdiv_s,
  rv_op_fsgnj_s, rv_op_fsgnjn_s, rv_op_fsgnjx_s, rv_op_fmin_s, rv_op_fmax_s, rv_op_fsqrt_s,
  rv_op_fle_s, rv_op_flt_s, rv_op_feq_s,
  rv_op_fcvt_w_s, rv_op_fcvt_wu_s, rv_op_fcvt_s_w, rv_op_fcvt_s_wu,
  rv_op_fmv_x_w, rv_op_fclass_s, rv_op_fmv_w_x,
  rv_op_fcvt_l_s, rv_op_fcvt_lu_s, rv_op_fcvt_s_l, rv_op_fcvt_s_lu,
  rv_op_fld, rv_op_fsd,
  rv_op_fmadd_d, rv_op_fmsub_d, rv_op_fnmsub_d, rv_op_fnmadd_d,
  rv_op_fadd_d, rv_op_fsub_d, rv_op_fmul_d, rv_op_fdiv_d,
  rv_op_fsgnj_d, rv_op_fsgnjn_d, rv_op_fsgnjx_d, rv_op_fmin_d, rv_op_fmax_d,
  rv_op_fcvt_s_d, rv_op_fcvt_d_s, rv_op_fsqrt_d,
  rv_op_fle_d, rv_op_flt_d, rv_op_feq_d,
  rv_op_fcvt_w_d, rv_op_fcvt_wu_d, rv_op_fcvt_d_w, rv_op_fcvt_d_wu,
  rv_op_fclass_d, rv_op_fcvt_l_d, rv_op_fcvt_lu_d,
  rv_op_fmv_x_d, rv_op_fcvt_d_l, rv_op_fcvt_d_lu, rv_op_fmv_d_x,
  rv_op_flq, rv_op_fsq,
  rv_op_fmadd_q, rv_op_fmsub_q, rv_op_fnmsub_q, rv_op_fnmadd_q,
  rv_op_fadd_q, rv_op_fsub_q, rv_op_fmul_q, rv_op_fdiv_q,
  rv_op_fsgnj_q, rv_op_fsgnjn_q, rv_op_fsgnjx_q, rv_op_fmin_q, rv_op_fmax_q,
  rv_op_fcvt_s_q, rv_op_fcvt_q_s, rv_op_fcvt_d_q, rv_op_fcvt_q_d, rv_op_fsqrt_q,
  rv_op_fle_q, rv_op_flt_q, rv_op_feq_q,
  rv_op_fcvt_w_q, rv_op_fcvt_wu_q, rv_op_fcvt_q_w, rv_op_fcvt_q_wu, rv_op_fclass_q,
  rv_op_fcvt_l_q, rv_op_fcvt_lu_q, rv_op_fcvt_q_l, rv_op_fcvt_q_lu,
  rv_op_fmv_x_q, rv_op_fmv_q_x,
  rv_op_c_addi4spn, rv_op_c_fld, rv_op_c_lw, rv_op_c_flw, rv_op_c_fsd, rv_op_c_sw, rv_op_c_fsw,
  rv_op_c_nop, rv_op_c_addi, rv_op_c_jal, rv_op_c_li, rv_op_c_addi16sp, rv_op_c_lui,
  rv_op_c_srli, rv_op_c_srai, rv_op_c_andi, rv_op_c_sub, rv_op_c_xor, rv_op_c_or, rv_op_c_and,
  rv_op_c_subw, rv_op_c_addw, rv_op_c_j, rv_op_c_beqz, rv_op_c_bnez, rv_op_c_slli, rv_op_c_fldsp,
  rv_op_c_lwsp, rv_op_c_flwsp, rv_op_c_jr, rv_op_c_mv, rv_op_c_ebreak, rv_op_c_jalr,
  rv_op_c_add, rv_op_c_fsdsp, rv_op_c_swsp, rv_op_c_fswsp, rv_op_c_ld, rv_op_c_sd,
  rv_op_c_addiw, rv_op_c_ldsp, rv_op_c_sdsp, rv_op_c_lq, rv_op_c_sq, rv_op_c_lqsp, rv_op_c_sqsp
} rv_opcode;

typedef enum {
  rv_format_r,
  rv_format_r_fence,
  rv_format_r_float,
  rv_format_r_float_rm,
  rv_format_r_float_rs2,
  rv_format_r_float_rm_rs2,
  rv_format_r_amo,
  rv_format_r4_rm,
  rv_format_u,
  rv_format_j,
  rv_format_i,
  rv_format_i_shamt,
  rv_format_i_csr,
  rv_format_s,
  rv_format_b,

  rv_format_cls_w,
  rv_format_cls_d,
  rv_format_cls_q,
  rv_format_ciw_addi4spn,
  rv_format_ci,
  rv_format_ci_addi16sp,
  rv_format_ci_lui,
  rv_format_ci_shamt,
  rv_format_ci_lsp_w,
  rv_format_ci_lsp_d,
  rv_format_ci_lsp_q,
  rv_format_cb,
  rv_format_cb_shamt,
  rv_format_cb_andi,
  rv_format_ca,
  rv_format_cj,
  rv_format_css_w,
  rv_format_css_d,
  rv_format_css_q,
  rv_format_cr
} rv_format;


typedef enum {
  rv_field_funct3,
  rv_field_funct7,
  rv_field_funct5,
  rv_field_funct2,
  rv_field_rd,
  rv_field_rs1,
  rv_field_rs2,
  rv_field_rs3,
  rv_field_fm,
  rv_field_pred,
  rv_field_succ,
  rv_field_rm,
  rv_field_aq,
  rv_field_rl,
  rv_field_imm_u,
  rv_field_imm_j,
  rv_field_imm_i,
  rv_field_shamt_imm,
  rv_field_shamt_funct,
  rv_field_csr,
  rv_field_imm_s,
  rv_field_imm_b,

  rv_field_c_funct3,
  rv_field_c_funct1,
  rv_field_c_funct_rs1,
  rv_field_c_funct_rs2,
  rv_field_c_rs1,
  rv_field_c_rs2,
  rv_field_c_rs1p,
  rv_field_c_rs2p,
  rv_field_cls_uimm6,
  rv_field_cls_uimm7,
  rv_field_cls_uimm8,
  rv_field_ciw_uimm9,
  rv_field_cb_imm8,
  rv_field_cj_imm11,
  rv_field_ci_imm5,
  rv_field_ci_imm9,
  rv_field_ci_imm17,
  rv_field_ci_uimm5,
  rv_field_ci_uimm7,
  rv_field_ci_uimm8,
  rv_field_ci_uimm9,
  rv_field_css_uimm7,
  rv_field_css_uimm8,
  rv_field_css_uimm9
} rv_field;


typedef enum {
  not_hint, magic_ops, rdm_ops
} rv_hint;

#endif
