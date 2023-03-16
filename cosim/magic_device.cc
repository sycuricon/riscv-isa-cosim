#include "magic_device.h"

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
    return get_simulator()->get_random_text_address(random);
  else
    return get_simulator()->get_random_data_address(random);
}

reg_t magic_t::rdm_epc_next(int smode) {
  return get_simulator()->get_exception_return_address(random, smode);
}

extern const std::vector<magic_type*> magic_generator_type({
  &magic_int,           /* MAGIC_RANDOM         */
  &magic_int,           /* MAGIC_RDM_WORD       */
  &magic_float,         /* MAGIC_RDM_FLOAT      */
  &magic_float,         /* MAGIC_RDM_DOUBLE     */
  &magic_text_address,  /* MAGIC_RDM_TEXT_ADDR  */
  &magic_data_address,  /* MAGIC_RDM_DATA_ADDR  */
  &magic_text_address,  /* MAGIC_MEPC_NEXT      */
  &magic_text_address   /* MAGIC_SEPC_NEXT      */
});
