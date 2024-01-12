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

static int text_indent = 0;
#define INC_INDENT text_indent++
#define DEC_INDENT text_indent--
#define DO_INDENT(fout) for(int i=0;i<text_indent;i++){(fout)<<'\t';}
#define DUMP_HEAD(fout,name) \
    do{\
        DO_INDENT(fout);\
        (fout) << '\"' << (name) << '\"' <<':'<<std::endl;\
        DO_INDENT(fout);\
        (fout) << '{' << std::endl;\
        INC_INDENT;\
    }while(0)

#define DUMP_TAIL(fout) \
    do{\
        DEC_INDENT;\
        DO_INDENT(fout);\
        (fout) << "}";\
    }while(0)

#define DUMP_FIELD(fout,name,val,offset,len) \
    do{\
        DO_INDENT(fout);\
        (fout) << '\"' << name << '\"' << ':' <<'\"';\
        reg_t mask = (len) == 64 ? ~(reg_t)0 : (((reg_t)1 << (len)) - 1) << offset;\
        reg_t field_value = (((val) & mask) >> offset);\
        if(len >= 4){\
            (fout) << std::setw((len)/4) << std::setfill('0') << std::hex << field_value;\
        }else{\
            for(int mask=1<<(len-1);mask;mask>>=1){\
                (fout) << std::setw(1) << std::dec << ((mask & field_value) ? 1 : 0);\
            }\
        }\
        (fout) << '\"';\
    }while(0)

#define DUMP_FIELD_NOT_FINAL(fout,name,val,offset,len) DUMP_FIELD(fout,name,val,offset,len); (fout) << "," << std::endl
#define DUMP_FIELD_FINAL(fout,name,val,offset,len) DUMP_FIELD(fout,name,val,offset,len); (fout) << std::endl
#define DUMP_VAL(name,val) std::cout << (name) << ':' << std::setw(16) << std::setfill('0') << std::hex << (val) << std::endl

#define DUMP_REGISTER(fout,name,val,name_array,offset_array,len_array,array_size)\
    do{\
        DUMP_VAL(name,val);\
        DUMP_HEAD(fout,name);\
        for(int i=0;i<(array_size);i++){\
            DUMP_FIELD(fout,(name_array)[i],(val),(offset_array)[i],(len_array)[i]);\
            if(i!=array_size-1){\
                (fout)<<',';\
            }\
            (fout)<<std::endl;\
        }\
        DUMP_TAIL(fout);\
    }while(0)

static void dump_tvec(std::ofstream& fout,reg_t value,const char* reg_name){
    DUMP_VAL(reg_name,value);
    DUMP_HEAD(fout,reg_name);
    DUMP_FIELD_NOT_FINAL(fout,"BASE",value&(~(reg_t)0b11),0,64);
    DUMP_FIELD_FINAL(fout,"MODE",value,0,2);
    DUMP_TAIL(fout);
}

static void dump_counteren(std::ofstream& fout,reg_t value,const char* reg_name){
    static const char* name[]={"CY","TM","IR","HPM3","HPM4","HPM5","HPM6","HPM7",\
        "HPM8","HPM9","HPM10","HPM11","HPM12","HPM13","HPM14","HPM15",\
        "HPM16","HPM17","HPM18","HPM19","HPM20","HPM21","HPM22","HPM23",
        "HPM24","HPM25","HPM26","HPM27","HPM28","HPM29","HPM30","HPM31"};
    static int offset[]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,\
        21,22,23,24,25,26,27,28,29,30,31};
    static int len[]={1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
    DUMP_REGISTER(fout,reg_name,value,name,offset,len,sizeof(name)/sizeof(name[0]));
}

static void dump_reg(std::ofstream& fout,reg_t value,const char* reg_name){
    DUMP_VAL(reg_name,value);
    DUMP_FIELD(fout,reg_name,value,0,64);
}

static void dump_pmpaddr(std::ofstream& fout,reg_t value,const char* reg_name){
    DUMP_VAL(reg_name,value);
    DUMP_FIELD(fout,reg_name,value<<2,0,56);
}

static void dump_priv(std::ofstream& fout,reg_t value,const char* reg_name){
    DUMP_VAL(reg_name,value);
    DUMP_FIELD(fout,reg_name,value,0,2);
}

static void dump_satp(std::ofstream& fout,reg_t value,const char* reg_name){
    DUMP_VAL(reg_name,value);
    DUMP_HEAD(fout,reg_name);
    DUMP_FIELD_NOT_FINAL(fout,"PPN",value<<12,0,56);
    DUMP_FIELD_NOT_FINAL(fout,"ASID",value,44,16);
    DUMP_FIELD_FINAL(fout,"MODE",value,60,4);
    DUMP_TAIL(fout);
}

static void dump_misa(std::ofstream& fout,reg_t value,const char* reg_name){
    static const char* name[]={"A","B","C","D","E","F","H","I","J","M","N","P","Q","S","U","V","X","64"};
    static int offset[]={0,1,2,3,4,5,7,8,9,12,13,15,16,18,20,21,23,63};
    static int len[]={1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
    DUMP_REGISTER(fout,reg_name,value,name,offset,len,sizeof(name)/sizeof(name[0]));
}

static void dump_medeleg(std::ofstream& fout,reg_t value,const char* reg_name){
    static const char* name[]={"Iaddr_Misalign","Iaccess_Fault","Illegal_Inst","Breakpoint",\
        "Laddr_Misalign","Laccess_Fault","Saddr_Misalign","Saccess_Fault","Ecall_U","Ecall_S",\
        "Ecall_H","Ecall_M","IPage_Fault","LPage_Fault","SPage_Fault"};
    static int offset[]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,15};
    static int len[]={1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
    DUMP_REGISTER(fout,reg_name,value,name,offset,len,sizeof(name)/sizeof(name[0]));
}

static void dump_mideleg(std::ofstream& fout,reg_t value,const char* reg_name){
    static const char* name[]={"USI","SSI","HSI","MSI","UTI","STI","HTI",\
            "MTI","UEI","SEI","HEI","MEI"};
    static int offset[]={0,1,2,3,4,5,6,7,8,9,10,11};
    static int len[]={1,1,1,1,1,1,1,1,1,1,1,1};
    DUMP_REGISTER(fout,reg_name,value,name,offset,len,sizeof(name)/sizeof(name[0]));
}

static void dump_mie(std::ofstream& fout,reg_t value,const char* reg_name){
    static const char* name[]={"USIE","SSIE","HSIE","MSIE","UTIE","STIE","HTIE",\
            "MTIE","UEIE","SEIE","HEIE","MEIE"};
    static int offset[]={0,1,2,3,4,5,6,7,8,9,10,11};
    static int len[]={1,1,1,1,1,1,1,1,1,1,1,1};
    DUMP_REGISTER(fout,reg_name,value,name,offset,len,sizeof(name)/sizeof(name[0]));
}

static void dump_pmpcfg0(std::ofstream& fout,reg_t value,const char* reg_name){
    DUMP_VAL(reg_name,value);
    DUMP_HEAD(fout,reg_name);
    static const char* name[]={"R","W","X","A","L"};
    static int offset[]={0,1,2,3,7};
    static int len[]={1,1,1,2,1};
    static const char* sub_pmpcfg_name[]={"pmp0cfg","pmp1cfg","pmp2cfg","pmp3cfg",\
        "pmp4cfg","pmp5cfg","pmp6cfg","pmp7cfg"};
    for(int i=0;i<8;i++){
        DUMP_REGISTER(fout,sub_pmpcfg_name[i],((value)&0xff),name,offset,len,sizeof(name)/sizeof(name[0]));
        value>>=8;
        if(i!=7)fout<<',';
        fout<<std::endl;
    }
    DUMP_TAIL(fout);
}

static void dump_pmpcfg2(std::ofstream& fout,reg_t value,const char* reg_name){
    DUMP_VAL(reg_name,value);
    DUMP_HEAD(fout,reg_name);
    static const char* name[]={"R","W","X","A","L"};
    static int offset[]={0,1,2,3,7};
    static int len[]={1,1,1,2,1};
    static const char* sub_pmpcfg_name[]={"pmp8cfg","pmp9cfg","pmp10cfg","pmp11cfg",\
        "pmp12cfg","pmp13cfg","pmp14cfg","pmp15cfg"};
    for(int i=0;i<8;i++){
        DUMP_REGISTER(fout,sub_pmpcfg_name[i],((value)&0xff),name,offset,len,sizeof(name)/sizeof(name[0]));
        value>>=8;
        if(i!=7)fout<<',';
        fout<<std::endl;
    }
    DUMP_TAIL(fout);
}

static void dump_mstatus(std::ofstream& fout,reg_t value,const char* reg_name){
    static const char* name[]={"SIE","MIE","SPIE","UBE","MPIE","SPP","VS","MPP","FS","XS","MPRV",\
        "SUM","MXR","TVM","TW","TSR","UXL","SXL","SBE","MBE","SD"};
    static int offset[]={1,3,5,6,7,8,9,11,13,15,17,18,19,20,21,22,32,34,36,37,63};
    static int len[]={1,1,1,1,1,1,2,2,2,2,1,1,1,1,1,1,2,2,1,1,1};
    DUMP_REGISTER(fout,reg_name,value,name,offset,len,sizeof(name)/sizeof(name[0]));
}

void sim_t::dump_register(){
    std::ofstream fout("reginfo.json");
    fout << "{" << std::endl;
    INC_INDENT;
    DUMP_HEAD(fout,"csr");
    processor_t *core = get_core(0);
    int csr_index[]={
        CSR_STVEC,
        CSR_SCOUNTEREN,
        CSR_SSCRATCH,
        CSR_SATP,
        CSR_MISA,
        CSR_MEDELEG,
        CSR_MIDELEG,
        CSR_MIE,
        CSR_MTVEC,
        CSR_MCOUNTEREN,
        CSR_PMPCFG0,
        CSR_PMPCFG2,
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
        CSR_PMPADDR15
    };
    static const char* csr_name[]={
        "stvec",
        "scounteren",
        "sscratch",
        "satp",
        "misa",
        "medeleg",
        "mideleg",
        "mie",
        "mtvec",
        "mcounteren",
        "pmpcfg0",
        "pmpcfg2",
        "pmpaddr0",
        "pmpaddr1",
        "pmpaddr2",
        "pmpaddr3",
        "pmpaddr4",
        "pmpaddr5",
        "pmpaddr6",
        "pmpaddr7",
        "pmpaddr8",
        "pmpaddr9",
        "pmpaddr10",
        "pmpaddr11",
        "pmpaddr12",
        "pmpaddr13",
        "pmpaddr14",
        "pmpaddr15",
    };
    typedef void (*DumpFunc)(std::ofstream&,reg_t,const char*);
    DumpFunc dump_func[]={
        &dump_tvec,
        &dump_counteren,
        &dump_reg,
        &dump_satp,
        &dump_misa,
        &dump_medeleg,
        &dump_mideleg,
        &dump_mie,
        &dump_tvec,
        &dump_counteren,
        &dump_pmpcfg0,
        &dump_pmpcfg2,
        &dump_pmpaddr,
        &dump_pmpaddr,
        &dump_pmpaddr,
        &dump_pmpaddr,
        &dump_pmpaddr,
        &dump_pmpaddr,
        &dump_pmpaddr,
        &dump_pmpaddr,
        &dump_pmpaddr,
        &dump_pmpaddr,
        &dump_pmpaddr,
        &dump_pmpaddr,
        &dump_pmpaddr,
        &dump_pmpaddr,
        &dump_pmpaddr,
        &dump_pmpaddr
    };

    for(size_t i=0;i<(sizeof(csr_index)/sizeof(csr_index[0]));i++){
        if(core->get_state()->csrmap.find(csr_index[i])==core->get_state()->csrmap.end()){
            std::printf("no csr %d\n",csr_index[i]);
            continue;
        }
        reg_t csr_val=(core->get_state()->csrmap[csr_index[i]])->read();
        dump_func[i](fout,csr_val,csr_name[i]);
        if(i!=sizeof(csr_index)/sizeof(csr_index[0])-1){
            fout<<",";
        }
        fout<<std::endl;
    }
    DUMP_TAIL(fout);
    fout<<","<<std::endl;
    
    reg_t mstatus=core->get_state()->mstatus->read();
    reg_t priv=core->get_state()->prv;
    std::printf("dump mstatus:\t%016llx\n",mstatus);
    std::printf("set the MPP to priv,set the MIE to PMIE\n");
    reg_t pc=core->get_state()->pc;
    std::printf("dump pc:\t%016llx\n",pc);
    std::printf("set the PC to MEPC\n");

    DUMP_HEAD(fout,"target");
    dump_mstatus(fout,mstatus,"mstatus");
    fout<<","<<std::endl;
    dump_priv(fout,priv,"priv");
    fout<<","<<std::endl;
    dump_reg(fout,pc,"address");
    fout<<std::endl;
    DUMP_TAIL(fout);
    fout<<","<<std::endl;

    DUMP_HEAD(fout,"GPR");
    static const char* reg_name[]={
        "x0","x1","x2","x3","x4","x5","x6","x7","x8","x9","x10","x11","x12","x13","x14","x15",\
        "x16","x17","x18","x19","x20","x21","x22","x23","x24","x25","x26","x27","x28","x29",\
        "x30","x31"
    };
    for(int i=1;i<32;i++){
        reg_t reg=core->get_state()->XPR[i];
        dump_reg(fout,reg,reg_name[i]);
        if(i<=30)fout<< ",";
        fout<< std::endl;
    }
    DUMP_TAIL(fout);
    fout<< std::endl;
    DUMP_TAIL(fout);
    fout<< std::endl;

    fout.close();
}