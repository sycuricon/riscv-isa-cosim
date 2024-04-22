#ifndef __SWAP_MEM_H__
#define __SWAP_MEM_H__
#include <cstdint>
#include <cstdlib>
#include <vector>
#include "memif.h"

class swap_mem_manager{
    private:
        struct swap_mem{
            uint64_t block_begin;
            size_t block_len;
            char* mem_block;
            swap_mem(uint64_t begin, size_t len, char* block):\
            block_begin(begin), block_len(len), mem_block(block){}
            ~swap_mem(){delete [] mem_block;}
        };

        std::vector<std::vector<swap_mem>> swap_manager;
        int swap_idx;
        memif_t* mem_manager;
    
    public:
        swap_mem_manager():swap_idx(0), mem_manager(nullptr){}
        void register_memif(memif_t* mem){mem_manager = mem;}
        void register_swap_mem(uint64_t block_begin, size_t block_len, char* mem_block, int swap_idx);
        void do_mem_swap();
};

extern swap_mem_manager swap_mem;

#endif