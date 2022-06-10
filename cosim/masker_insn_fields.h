struct field_funct3_t : field_t {
  field_funct3_t() { name = rv_field_funct3; }
  void decode(rv_inst inst) { value = (inst << 49) >> 61; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = ((rv_inst)value << 61) >> 49;
    return inst | adjust;
  }
} funct3;

struct field_funct7_t : field_t {
  field_funct7_t() { name = rv_field_funct7; }
  void decode(rv_inst inst) { value = (inst << 32) >> 57; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = ((rv_inst)value << 57) >> 32;
    return inst | adjust;
  }
} funct7;

struct field_funct5_t : field_t {
  field_funct5_t() { name = rv_field_funct5; }
  void decode(rv_inst inst) { value = (inst << 32) >> 59; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = ((rv_inst)value << 59) >> 32;
    return inst | adjust;
  }
} funct5;

struct field_funct2_t : field_t {
  field_funct2_t() { name = rv_field_funct2; }
  void decode(rv_inst inst) { value = (inst << 37) >> 62; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = ((rv_inst)value << 62) >> 37;
    return inst | adjust;
  }
} funct2;

struct field_rd_t : field_t {
  field_rd_t() { name = rv_field_rd; }
  void decode(rv_inst inst) { value = (inst << 52) >> 59; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = ((rv_inst)value << 59) >> 52;
    return inst | adjust;
  }
} rd;

struct field_rs1_t : field_t {
  field_rs1_t() { name = rv_field_rs1; }
  void decode(rv_inst inst) { value = (inst << 44) >> 59; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = ((rv_inst)value << 59) >> 44;
    return inst | adjust;
  }
} rs1;

struct field_rs2_t : field_t {
  field_rs2_t() { name = rv_field_rs2; }
  void decode(rv_inst inst) { value = (inst << 39) >> 59; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = ((rv_inst)value << 59) >> 39;
    return inst | adjust;
  }
} rs2;

struct field_rs3_t : field_t {
  field_rs3_t() { name = rv_field_rs3; }
  void decode(rv_inst inst) { value = (inst << 32) >> 59; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = ((rv_inst)value << 59) >> 32;
    return inst | adjust;
  }
} rs3;

struct field_fm_t : field_t {
  field_fm_t() { name = rv_field_fm; }
  void decode(rv_inst inst) { value = (inst << 32) >> 60; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = ((rv_inst)value << 60) >> 32;
    return inst | adjust;
  }
} fm;

struct field_pred_t : field_t {
  field_pred_t() { name = rv_field_pred; }
  void decode(rv_inst inst) { value = (inst << 36) >> 60; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = ((rv_inst)value << 60) >> 36;
    return inst | adjust;
  }
} pred;

struct field_succ_t : field_t {
  field_succ_t() { name = rv_field_succ; }
  void decode(rv_inst inst) { value = (inst << 40) >> 60; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = ((rv_inst)value << 60) >> 40;
    return inst | adjust;
  }
} succ;

struct field_rm_t : field_t {
  field_rm_t() { name = rv_field_rm; }
  void decode(rv_inst inst) { value = (inst << 49) >> 61; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = ((rv_inst)value << 61) >> 49;
    return inst | adjust;
  }
} rm;

struct field_aq_t : field_t {
  field_aq_t() { name = rv_field_aq; }
  void decode(rv_inst inst) { value = (inst << 37) >> 63; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = ((rv_inst)value << 63) >> 37;
    return inst | adjust;
  }
} aq;

struct field_rl_t : field_t {
  field_rl_t() { name = rv_field_rl; }
  void decode(rv_inst inst) { value = (inst << 38) >> 63; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = ((rv_inst)value << 63) >> 38;
    return inst | adjust;
  }
} rl;

struct field_imm_u_t : field_t {
  field_imm_u_t() { name = rv_field_imm_u; }
  void decode(rv_inst inst) { value = (((int64_t)inst << 32) >> 44) << 12; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = (((rv_inst)value >> 12) << 44) >> 32;
    return inst | adjust;
  }
} imm_u;

struct field_imm_j_t : field_t {
  field_imm_j_t() { name = rv_field_imm_j; }
  void decode(rv_inst inst) {
    value = (((int64_t)inst << 32) >> 63) << 20 |
            ((inst << 33) >> 54) << 1 |
            ((inst << 43) >> 63) << 11 |
            ((inst << 44) >> 56) << 12; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = (((rv_inst)value >> 20) << 63) >> 32 |
            (((rv_inst)value >> 1) << 54) >> 33 |
            (((rv_inst)value >> 11) << 63) >> 43 |
            (((rv_inst)value >> 12) << 56) >> 44;
    return inst | adjust;
  }
} imm_j;

struct field_imm_i_t : field_t {
  field_imm_i_t() { name = rv_field_imm_i; }
  void decode(rv_inst inst) { value = ((int64_t)inst << 32) >> 52; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = ((rv_inst)value << 52) >> 32;
    return inst | adjust;
  }
} imm_i;

struct field_shamt_imm_t : field_t {
  field_shamt_imm_t() { name = rv_field_shamt_imm; }
  void decode(rv_inst inst) { value = (inst << (39-(xlen-rv32))) >> (59-(xlen-rv32)); }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = ((rv_inst)value << (59-(xlen-rv32))) >> (39-(xlen-rv32));
    return inst | adjust;
  }
} shamt_imm;

struct field_shamt_funct_t : field_t {
  field_shamt_funct_t() { name = rv_field_shamt_funct; }
  void decode(rv_inst inst) { value = (inst << 32) >> (57+(xlen-rv32)); }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = ((rv_inst)value << (57+(xlen-rv32))) >> 32;
    return inst | adjust;
  }
} shamt_funct;

struct field_csr_t : field_t {
  field_csr_t() { name = rv_field_csr; }
  void decode(rv_inst inst) { value = (inst << 32) >> 52; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = ((rv_inst)value << 52) >> 32;
    return inst | adjust;
  }
} csr;

struct field_imm_s_t : field_t {
  field_imm_s_t() { name = rv_field_imm_s; }
  void decode(rv_inst inst) {
    value = (((int64_t)inst << 32) >> 57) << 5 |
            (inst << 52) >> 59; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = (((rv_inst)value >> 5) << 57) >> 32 |
                     ((rv_inst)value << 59) >> 52;
    return inst | adjust;
  }
} imm_s;

struct field_imm_b_t : field_t {
  field_imm_b_t() { name = rv_field_imm_b; }
  void decode(rv_inst inst) {
    value = (((int64_t)inst << 32) >> 63) << 12 |
            ((inst << 33) >> 58) << 5 |
            ((inst << 52) >> 60) << 1 |
            ((inst << 56) >> 63) << 11; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = (((rv_inst)value >> 12) << 63) >> 32 |
                     (((rv_inst)value >> 5) << 58) >> 33 |
                     (((rv_inst)value >> 1) << 60) >> 52 |
                     (((rv_inst)value >> 11) << 63) >> 56;
    return inst | adjust;
  }
} imm_b;

struct field_c_funct3_t : field_t {
  field_c_funct3_t() { name = rv_field_c_funct3; }
  void decode(rv_inst inst) { value = (inst << 48) >> 60; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = ((rv_inst)value << 60) >> 48;
    return inst | adjust;
  }
} c_funct3;

struct field_c_funct1_t : field_t {
  field_c_funct1_t() { name = rv_field_c_funct1; }
  void decode(rv_inst inst) { value = (inst << 51) >> 63; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = ((rv_inst)value << 63) >> 51;
    return inst | adjust;
  }
} c_funct1;

struct field_c_funct_rs1_t : field_t {
  field_c_funct_rs1_t() { name = rv_field_c_funct_rs1; }
  void decode(rv_inst inst) { value = (inst << 52) >> 62; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = ((rv_inst)value << 62) >> 52;
    return inst | adjust;
  }
} c_funct_rs1;

struct field_c_funct_rs2_t : field_t {
  field_c_funct_rs2_t() { name = rv_field_c_funct_rs2; }
  void decode(rv_inst inst) { value = (inst << 57) >> 62; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = ((rv_inst)value << 62) >> 57;
    return inst | adjust;
  }
} c_funct_rs2;

struct field_c_rs1_t : field_t {
  field_c_rs1_t() { name = rv_field_c_rs1; }
  void decode(rv_inst inst) { value = (inst << 52) >> 59; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = ((rv_inst)value << 59) >> 52;
    return inst | adjust;
  }
} c_rs1;

struct field_c_rs2_t : field_t {
  field_c_rs2_t() { name = rv_field_c_rs2; }
  void decode(rv_inst inst) { value = (inst << 57) >> 59; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = ((rv_inst)value << 59) >> 57;
    return inst | adjust;
  }
} c_rs2;

struct field_c_rs1p_t : field_t {
  field_c_rs1p_t() { name = rv_field_c_rs1p; }
  void decode(rv_inst inst) { value = (inst << 54) >> 61; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = ((rv_inst)value << 61) >> 54;
    return inst | adjust;
  }
} c_rs1p;

struct field_c_rs2p_t : field_t {
  field_c_rs2p_t() { name = rv_field_c_rs2p; }
  void decode(rv_inst inst) { value = (inst << 59) >> 61; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = ((rv_inst)value << 61) >> 59;
    return inst | adjust;
  }
} c_rs2p;

struct field_cls_uimm6_t : field_t {
  field_cls_uimm6_t() { name = rv_field_cls_uimm6; }
  void decode(rv_inst inst) {
    value = ((inst << 51) >> 61) << 3 |
            ((inst << 57) >> 63) << 2 |
            ((inst << 58) >> 63) << 6; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = (((rv_inst)value >> 3) << 61) >> 51 |
                     (((rv_inst)value >> 2) << 63) >> 57 |
                     (((rv_inst)value >> 6) << 63) >> 58;
    return inst | adjust;
  }
} cls_uimm6;

struct field_cls_uimm7_t : field_t {
  field_cls_uimm7_t() { name = rv_field_cls_uimm7; }
  void decode(rv_inst inst) {
    value = ((inst << 51) >> 61) << 3 |
            ((inst << 57) >> 62) << 6; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = (((rv_inst)value >> 3) << 61) >> 51 |
                     (((rv_inst)value >> 6) << 62) >> 57;
    return inst | adjust;
  }
} cls_uimm7;

struct field_cls_uimm8_t : field_t {
  field_cls_uimm8_t() { name = rv_field_cls_uimm8; }
  void decode(rv_inst inst) {
    value = ((inst << 51) >> 62) << 4 |
            ((inst << 53) >> 63) << 8 |
            ((inst << 57) >> 62) << 6; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = (((rv_inst)value >> 4) << 62) >> 51 |
                     (((rv_inst)value >> 8) << 63) >> 53 |
                     (((rv_inst)value >> 6) << 62) >> 57;
    return inst | adjust;
  }
} cls_uimm8;

struct field_ciw_uimm9_t : field_t {
  field_ciw_uimm9_t() { name = rv_field_ciw_uimm9; }
  void decode(rv_inst inst) {
    value = ((inst << 51) >> 62) << 4 |
            ((inst << 53) >> 60) << 6 |
            ((inst << 57) >> 63) << 2 |
            ((inst << 58) >> 63) << 3; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = (((rv_inst)value >> 4) << 62) >> 51 |
                     (((rv_inst)value >> 6) << 60) >> 53 |
                     (((rv_inst)value >> 2) << 63) >> 57 |
                     (((rv_inst)value >> 3) << 63) >> 58;
    return inst | adjust;
  }
} ciw_uimm9;

struct field_ci_imm5_t : field_t {
  field_ci_imm5_t() { name = rv_field_ci_imm5; }
  void decode(rv_inst inst) {
    value = (((int64_t)inst << 51) >> 63) << 5 |
            (inst << 57) >> 59; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = (((rv_inst)value >> 5) << 63) >> 51 |
                     ((rv_inst)value << 59) >> 57;
    return inst | adjust;
  }
} ci_imm5;

struct field_ci_imm9_t : field_t {
  field_ci_imm9_t() { name = rv_field_ci_imm9; }
  void decode(rv_inst inst) {
    value = (((int64_t)inst << 51) >> 63) << 9 |
            ((inst << 57) >> 63) << 4 |
            ((inst << 58) >> 63) << 6 |
            ((inst << 59) >> 62) << 7 |
            ((inst << 61) >> 63) << 5; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = (((rv_inst)value >> 9) << 63) >> 51 |
                     (((rv_inst)value >> 4) << 63) >> 57 |
                     (((rv_inst)value >> 6) << 63) >> 58 |
                     (((rv_inst)value >> 7) << 62) >> 59 |
                     (((rv_inst)value >> 5) << 63) >> 61;
    return inst | adjust;
  }
} ci_imm9;

struct field_ci_imm17_t : field_t {
  field_ci_imm17_t() { name = rv_field_ci_imm17; }
  void decode(rv_inst inst) {
    value = (((int64_t)inst << 51) >> 63) << 17 |
            ((inst << 57) >> 59) << 12; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = (((rv_inst)value >> 17) << 63) >> 51 |
                     (((rv_inst)value >> 12) << 59) >> 57;
    return inst | adjust;
  }
} ci_imm17;

struct field_ci_uimm5_t : field_t {
  field_ci_uimm5_t() { name = rv_field_ci_uimm5; }
  void decode(rv_inst inst) {
    value = ((inst << 51) >> 63) << 5 |
            (inst << 57) >> 59; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = (((rv_inst)value >> 5) << 63) >> 51 |
                     ((rv_inst)value << 59) >> 57;
    return inst | adjust;
  }
} ci_uimm5;

struct field_cj_imm11_t : field_t {
  field_cj_imm11_t() { name = rv_field_cj_imm11; }
  void decode(rv_inst inst) {
    value = (((int64_t)inst << 51) >> 63) << 11 |
            ((inst << 52) >> 63) << 4 |
            ((inst << 53) >> 62) << 8 |
            ((inst << 55) >> 63) << 10 |
            ((inst << 56) >> 63) << 6 |
            ((inst << 57) >> 63) << 7 |
            ((inst << 58) >> 61) << 1 |
            ((inst << 61) >> 63) << 5; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = (((rv_inst)value >> 11) << 63) >> 51 |
                     (((rv_inst)value >> 4) << 63) >> 52 |
                     (((rv_inst)value >> 8) << 62) >> 53 |
                     (((rv_inst)value >> 10) << 63) >> 55 |
                     (((rv_inst)value >> 6) << 63) >> 56 |
                     (((rv_inst)value >> 7) << 63) >> 57 |
                     (((rv_inst)value >> 1) << 61) >> 58 |
                     (((rv_inst)value >> 5) << 63) >> 61;
    return inst | adjust;
  }
} cj_imm11;

struct field_cb_imm8_t : field_t {
  field_cb_imm8_t() { name = rv_field_cb_imm8; }
  void decode(rv_inst inst) {
    value = (((int64_t)inst << 51) >> 63) << 8 |
            ((inst << 52) >> 62) << 3 |
            ((inst << 57) >> 62) << 6 |
            ((inst << 59) >> 62) << 1 |
            ((inst << 61) >> 63) << 5; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = (((rv_inst)value >> 8) << 63) >> 51 |
                     (((rv_inst)value >> 3) << 62) >> 52 |
                     (((rv_inst)value >> 6) << 62) >> 57 |
                     (((rv_inst)value >> 1) << 62) >> 59 |
                     (((rv_inst)value >> 5) << 63) >> 61;
    return inst | adjust;
  }
} cb_imm8;

struct field_ci_uimm7_t : field_t {
  field_ci_uimm7_t() { name = rv_field_ci_uimm7; }
  void decode(rv_inst inst) {
    value = ((inst << 51) >> 63) << 5 |
            ((inst << 57) >> 61) << 2 |
            ((inst << 60) >> 62) << 6; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = (((rv_inst)value >> 5) << 63) >> 51 |
                     (((rv_inst)value >> 2) << 61) >> 57 |
                     (((rv_inst)value >> 6) << 62) >> 60;
    return inst | adjust;
  }
} ci_uimm7;

struct field_ci_uimm8_t : field_t {
  field_ci_uimm8_t() { name = rv_field_ci_uimm8; }
  void decode(rv_inst inst) {
    value = ((inst << 51) >> 63) << 5 |
            ((inst << 57) >> 62) << 3 |
            ((inst << 59) >> 61) << 6; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = (((rv_inst)value >> 5) << 63) >> 51 |
                     (((rv_inst)value >> 3) << 62) >> 57 |
                     (((rv_inst)value >> 6) << 61) >> 59;
    return inst | adjust;
  }
} ci_uimm8;

struct field_ci_uimm9_t : field_t {
  field_ci_uimm9_t() { name = rv_field_ci_uimm9; }
  void decode(rv_inst inst) {
    value = ((inst << 51) >> 63) << 5 |
            ((inst << 57) >> 63) << 4 |
            ((inst << 58) >> 60) << 6; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = (((rv_inst)value >> 5) << 63) >> 51 |
                     (((rv_inst)value >> 4) << 63) >> 57 |
                     (((rv_inst)value >> 6) << 60) >> 58;
    return inst | adjust;
  }
} ci_uimm9;

struct field_css_uimm7_t : field_t {
  field_css_uimm7_t() { name = rv_field_css_uimm7; }
  void decode(rv_inst inst) {
    value = ((inst << 51) >> 60) << 2 |
            ((inst << 55) >> 62) << 6; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = (((rv_inst)value >> 2) << 60) >> 51 |
                     (((rv_inst)value >> 6) << 62) >> 55;
    return inst | adjust;
  }
} css_uimm7;

struct field_css_uimm8_t : field_t {
  field_css_uimm8_t() { name = rv_field_css_uimm8; }
  void decode(rv_inst inst) {
    value = ((inst << 51) >> 61) << 3 |
            ((inst << 54) >> 61) << 6; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = (((rv_inst)value >> 3) << 61) >> 51 |
                     (((rv_inst)value >> 6) << 61) >> 54;
    return inst | adjust;
  }
} css_uimm8;

struct field_css_uimm9_t : field_t {
  field_css_uimm9_t() { name = rv_field_css_uimm9; }
  void decode(rv_inst inst) {
    value = ((inst << 51) >> 62) << 4 |
            ((inst << 53) >> 60) << 6; }
  rv_inst encode(rv_inst inst) {
    rv_inst adjust = (((rv_inst)value >> 4) << 62) >> 51 |
                     (((rv_inst)value >> 6) << 60) >> 53;
    return inst | adjust;
  }
} css_uimm9;