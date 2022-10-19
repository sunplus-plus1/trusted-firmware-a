#ifndef __Q645_SP_PM_H_
#define __Q645_SP_PM_H_


#define PMC_SAVE_DATA_BASE          (0x150000)
#define WARMBOOT_CODE_ADDR          (0x120000)
#define WARMBOOT_XBOOT_LOAD_ADDR    (0x100000 - 0x20)


#define RGST_SECURE_REG       RF_GRP(502,0)
#define SECGRP1_MAIN_REG      RF_GRP(83,0)
#define SECGRP1_PAI_REG       RF_GRP(84,0)
#define SECGRP1_PAII_REG  	  RF_GRP(85,0)

#define SP_WDG_CTRL			  RF_GRP(12,10)
#define SP_WDG_CNT			  RF_GRP(12,11)
#define SP_WDG_ENABLE         RF_GRP(4,18)

typedef struct{
	uint32_t reg_Sec_Main[32];
	uint32_t reg_Sec_PAI[32];
	uint32_t reg_Sec_PAII[32];
	uint32_t reg_Sec_Group[32];
}reg_data;

struct xboot_hdr {
	unsigned int  magic;
	unsigned int version;
	unsigned int length;       // exclude header
	unsigned int checksum;     // exclude header
	unsigned int img_flag;
	unsigned int reserved[3];
};


#endif
