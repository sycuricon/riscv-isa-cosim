#include "sim.h"
#include <iomanip>
uint64_t sim_t::reg_info_mem::mem_base;

void sim_t::dump_memlist(std::list<std::pair<reg_t,char*>>& mem_list,std::ofstream& mem_file){
    reg_t last_page=0;
    for(auto p=mem_list.begin();p!=mem_list.end();p++){
        // if(p->first-4096 != last_page){
        mem_file << "@" << std::setw(16) << std::setfill('0') << std::hex << (p->first/8) << std::endl;
        // }
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

sim_t::reg_info_mem sim_t::find_useless_page(std::list<std::pair<reg_t,char*>>::iterator begin,std::list<std::pair<reg_t,char*>>& mem_list,std::list<char*>& free_list){
    std::list<std::pair<reg_t,char*>>::iterator end=mem_list.end();
    while(begin!=end){
        auto tmp=begin;
        ++tmp;
        if(tmp==end){
            if(begin->first==0x7ffff000){
                throw std::exception();
            }
            break;
        }
        if(tmp->first-4096==begin->first){
            ++begin;
        }else{
            break;
        }
    }
    if(begin==end){
        throw std::exception();
    }
    
    char* page=(char*)std::calloc(4096,1);
    free_list.push_back(page);
    std::list<std::pair<reg_t,char*>>::iterator p;
    auto q=begin;
    ++q;
    if(q!=end){
        p=mem_list.insert(q,std::make_pair(begin->first+4096,page));
    }else{
        mem_list.push_back(std::make_pair(begin->first+4096,page));
        p=++begin;
    }
        
    reg_info_mem ret(p);
    std::printf("get free page %016llx\n",ret.base);

    return ret;
}

#define LUI(regindex,imm) ((((imm)&0xfffff000)+((((imm)&800)!=0)<<12))|(((regindex)&0x1f)<<7)|0b0110111)
#define ADDI(regindex,imm) ((((imm)&0xfff)<<20)|(((regindex)&0x1f)<<15)|((0b000)<<12)|(((regindex)&0x1f)<<7)|0b0010011)
#define SLLI(regindex,imm) ((((imm)&0x3f)<<20)|(((regindex)&0x1f)<<15)|((0b001)<<12)|(((regindex)&0x1f)<<7)|0b0010011)
#define SRLI(regindex,imm) ((((imm)&0x3f)<<20)|(((regindex)&0x1f)<<15)|((0b101)<<12)|(((regindex)&0x1f)<<7)|0b0010011)
#define J(regindex) ((((regindex)&0x1f)<<15)|((0&0x1f)<<7)|0b1100111)
#define LOAD(regindex) ((((regindex)&0x1f)<<15)|((0b011)<<12)|(((regindex)&0x1f)<<7)|0b0000011)
#define CSRW(regindex,csrindex) ((((csrindex)&0xfff)<<20)|(((regindex)&0x1f)<<15)|((0b001)<<12)|((0&0x1f)<<7)|0b1110011)
#define STORE(addr_reg,value_reg) ((((value_reg)&0x1f)<<20)|(((addr_reg)&0x1f)<<15)|(0b011<<12)|0b0100011)
bool sim_t::reg_info_mem::write_li(int regindex,uint64_t imm){
    if(inst_remain()<5||data_remain()<1){
        return false;
    }
    uint64_t addr=get_data_addr();
    write_data(imm);
    write_inst(LUI(regindex,addr));
    write_inst(SLLI(regindex,32));
    write_inst(SRLI(regindex,32));
    write_inst(ADDI(regindex,addr));
    write_inst(LOAD(regindex));
    return true;
}

bool sim_t::reg_info_mem::write_jal_abs(uint64_t abs){
    if(inst_remain()<6||data_remain()<1){
        return false;
    }
    int regindex=1;
    write_li(regindex,abs);
    write_inst(J(regindex));
    return true;
}

bool sim_t::reg_info_mem::write_mv_csr(int csrindex,uint64_t imm){
    if(inst_remain()<6||data_remain()<1){
        return false;
    }
    int regindex=1;
    write_li(regindex,imm);
    write_inst(CSRW(regindex,csrindex));
    return true;
}

bool sim_t::reg_info_mem::write_store(uint64_t addr,uint64_t imm){
    if(inst_remain()<11||data_remain()<2){
        return false;
    }
    int value_reg=1;
    int addr_reg=2;
    write_li(value_reg,imm);
    write_li(addr_reg,addr);
    write_inst(STORE(addr_reg,value_reg));
    return true;
}

void sim_t::dump_regfile(int regindex,uint64_t imm,reg_info_mem& p,std::list<std::pair<reg_t,char*>>& mem_list,std::list<char*>& free_list){
    p.write_li(regindex,imm);
    if(p.inst_remain()<12||p.data_remain()<2){
        reg_info_mem q=find_useless_page(p.page,mem_list,free_list);
        p.write_jal_abs(q.base);
        p=q;
    }
}

void sim_t::dump_csrfile(int csrindex,uint64_t imm,reg_info_mem& p,std::list<std::pair<reg_t,char*>>& mem_list,std::list<char*>& free_list){
    p.write_mv_csr(csrindex,imm);
    if(p.inst_remain()<12||p.data_remain()<2){
        reg_info_mem q=find_useless_page(p.page,mem_list,free_list);
        p.write_jal_abs(q.base);
        p=q;
    }
}

void sim_t::add_reg_info(std::list<std::pair<reg_t,char*>>& mem_list,std::list<char*>& free_list){
    char* page=(char*)std::malloc(4096);
    free_list.push_back(page);
    std::memcpy(page,mem_list.begin()->second,4096);
    mem_list.begin()->second=page;
    reg_info_mem p(mem_list.begin());
    std::printf("copy the first page\n");

    uint32_t tmp_data[6]={p.inst_page[0],p.inst_page[1],p.inst_page[2],p.inst_page[3],p.inst_page[4],p.inst_page[5]};
    reg_info_mem q=find_useless_page(mem_list.begin(),mem_list,free_list);
    p.inst_page[0]=LUI(1,q.base);
    p.inst_page[2]=SLLI(1,32);
    p.inst_page[3]=SRLI(1,32);
    p.inst_page[1]=ADDI(1,q.base);
    p.inst_page[4]=J(1);
    std::printf("jump from first page to reg info recovery page\n");
    q.write_store(p.base,((uint64_t)tmp_data[1]<<32)|(tmp_data[0]));
    q.write_store(p.base+8,((uint64_t)tmp_data[3]<<32)|(tmp_data[2]));
    q.write_store(p.base+16,((uint64_t)tmp_data[5]<<32)|(tmp_data[4]));
    std::printf("recover the first four inst of the first page\n");

    processor_t *core = get_core(0);
    int csr_index[]={
        // CSR_FFLAGS,
        // CSR_FCSR,
        // CSR_FRM,
        CSR_SSTATUS,
        CSR_SIE,
        CSR_STVEC,
        CSR_SCOUNTEREN,
        CSR_SSCRATCH,
        CSR_SATP,
        CSR_MSTATUS,
        CSR_MISA,
        CSR_MEDELEG,
        CSR_MIDELEG,
        CSR_MIE,
        CSR_MEPC,
        CSR_MTVEC,
        CSR_MCOUNTEREN,
        CSR_PMPCFG0,
        CSR_PMPCFG1,
        CSR_PMPCFG2,
        CSR_PMPCFG3,
        // CSR_PMPCFG4,
        // CSR_PMPCFG5,
        // CSR_PMPCFG6,
        // CSR_PMPCFG7,
        // CSR_PMPCFG8,
        // CSR_PMPCFG9,
        // CSR_PMPCFG10, 
        // CSR_PMPCFG11, 
        // CSR_PMPCFG12, 
        // CSR_PMPCFG13, 
        // CSR_PMPCFG14, 
        // CSR_PMPCFG15, 
        CSR_PMPADDR0, 
        CSR_PMPADDR1, 
        CSR_PMPADDR2, 
        CSR_PMPADDR3, 
        CSR_PMPADDR4, 
        CSR_PMPADDR5, 
        CSR_PMPADDR6, 
        CSR_PMPADDR7, 
        CSR_PMPADDR8, 
        CSR_PMPADDR9, 
        CSR_PMPADDR10,
        CSR_PMPADDR11,
        CSR_PMPADDR12,
        CSR_PMPADDR13,
        CSR_PMPADDR14,
        CSR_PMPADDR15,
        // CSR_PMPADDR16,
        // CSR_PMPADDR17,
        // CSR_PMPADDR18,
        // CSR_PMPADDR19,
        // CSR_PMPADDR20,
        // CSR_PMPADDR21,
        // CSR_PMPADDR22,
        // CSR_PMPADDR23,
        // CSR_PMPADDR24,
        // CSR_PMPADDR25,
        // CSR_PMPADDR26,
        // CSR_PMPADDR27,
        // CSR_PMPADDR28,
        // CSR_PMPADDR29,
        // CSR_PMPADDR30,
        // CSR_PMPADDR31,
        // CSR_PMPADDR32,
        // CSR_PMPADDR33,
        // CSR_PMPADDR34,
        // CSR_PMPADDR35,
        // CSR_PMPADDR36,
        // CSR_PMPADDR37,
        // CSR_PMPADDR38,
        // CSR_PMPADDR39,
        // CSR_PMPADDR40,
        // CSR_PMPADDR41,
        // CSR_PMPADDR42,
        // CSR_PMPADDR43,
        // CSR_PMPADDR44,
        // CSR_PMPADDR45,
        // CSR_PMPADDR46,
        // CSR_PMPADDR47,
        // CSR_PMPADDR48,
        // CSR_PMPADDR49,
        // CSR_PMPADDR50,
        // CSR_PMPADDR51,
        // CSR_PMPADDR52,
        // CSR_PMPADDR53,
        // CSR_PMPADDR54,
        // CSR_PMPADDR55,
        // CSR_PMPADDR56,
        // CSR_PMPADDR57,
        // CSR_PMPADDR58,
        // CSR_PMPADDR59,
        // CSR_PMPADDR60,
        // CSR_PMPADDR61,
        // CSR_PMPADDR62,
        // CSR_PMPADDR63,
    };
    for(size_t i=0;i<(sizeof(csr_index)/sizeof(csr_index[0]));i++){
        if(core->get_state()->csrmap.find(csr_index[i])==core->get_state()->csrmap.end()){
            std::printf("no csr %d\n",csr_index[i]);
            continue;
        }
        std::printf("dump csr %d:%016llx\n",csr_index[i],(core->get_state()->csrmap[csr_index[i]])->read());
        dump_csrfile(csr_index[i],(core->get_state()->csrmap[csr_index[i]])->read(),q,mem_list,free_list);
    }
    
    reg_t mstatus=core->get_state()->mstatus->read();
    reg_t priv=core->get_state()->prv;
    mstatus=((mstatus&~((uint64_t)0b11<<11))|((priv&0b11)<<11));
    mstatus|=(mstatus&(0b1<<3))<<4;
    dump_csrfile(CSR_MSTATUS,mstatus,q,mem_list,free_list);
    std::printf("set the MPP to priv,set the MIE to PMIE\n");
    for(int i=1;i<32;i++){
        std::printf("dump reg %d:%016llx\n",i,core->get_state()->XPR[i]);
        dump_regfile(i,core->get_state()->XPR[i],q,mem_list,free_list);
    }
    q.write_inst(0x30200073);
    std::printf("mret\n");

}