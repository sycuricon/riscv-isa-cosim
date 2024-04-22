#include "swap_mem.h"

swap_mem_manager swap_mem;

void swap_mem_manager::register_swap_mem(uint64_t block_begin, size_t block_len, char* mem_block, int swap_idx){
  if(swap_manager.size() <= swap_idx ){
    swap_manager.resize(swap_idx + 1);
  }
  swap_manager[swap_idx].push_back(swap_mem(block_begin, block_len, mem_block));
  this->swap_idx = this->swap_idx < swap_idx ? swap_idx : this->swap_idx;
}

void swap_mem_manager::do_mem_swap(){
  swap_idx --;
  if(swap_idx < 0){
    throw std::invalid_argument(std::string("the program is not halted at the last swap mem"));
  }
  std::vector<swap_mem>& swap_mem_vector = swap_manager[swap_idx];
  for(auto p=swap_mem_vector.begin();p!=swap_mem_vector.end();p++){
    mem_manager->write(p->block_begin, p->block_len, p->mem_block);
  }
}