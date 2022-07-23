// //
// // Created by phantom on 6/6/22.
// //

// #include "masker.h"
// #include "stdio.h"
// #include <iostream>

// int main() {

//   masker_inst_t tester;
//   uint16_t inst = 0;
//   tester.inst = 0;
//   tester.xlen = rv64;
//   while (true) {
//     if (inst == 0)
//       printf(">>>>>>>>>>>> inst = %08x\n", inst);
//     tester.inst = inst;
//     tester.op = decode_inst_opcode(tester.xlen, tester.inst);
//     decode_inst_oprand(&tester);
//     rv_inst enc = tester.encode();

//     if (enc != tester.inst && tester.op != rv_op_illegal) {
//       printf("GG: %lx -> %lx\n",  tester.inst, enc);
//       std::cout << tester.op << std::endl;
//       tester.encode(true);
//       return 1;
//     }
//     else {
//       inst++;
//     }
//   }
// }