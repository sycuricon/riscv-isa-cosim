#include "sim.h"
#include <iomanip>
void sim_t::dump_memlist(std::list<std::pair<reg_t,char*>>& mem_list,std::ofstream& mem_file){
    int last_page=0;
    for(auto p=mem_list.begin();p!=mem_list.end();p++){
        if(p->first-4096 != last_page){
            mem_file << "@" << std::setw(16) << std::setfill('0') << std::hex << (p->first/8) << std::endl;
        }
        last_page = p->first;
        for(int i=0;i<4096;i+=8){
            char* c = &(p->second[i]);
            uint64_t data=0;
            for(int j=7;j>=0;j--){
                data<<=8;
                data+=(uint8_t)c[j];
            }
            mem_file << std::setw(16) << std::setfill('0') << std::hex << data << std::endl;
        }
    }
}

void sim_t::add_reg_info(std::list<std::pair<reg_t,char*>>& mem_list,std::list<char*>& free_list){

}