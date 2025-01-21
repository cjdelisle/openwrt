

typedef unsigned long int uint32;       /* 32-bit unsigned integer      */

typedef signed short sint15;            /* 16-bit signed integer        */
typedef signed short int int16;                         /* 16-bit signed integer        */
typedef unsigned short uint16;          /* 16-bit unsigned integer      */

typedef signed char sint7;              /* 8-bit signed integer         */
typedef unsigned char uint8;            /* 8-bit unsigned integer       */

#ifndef VPint
#define VPint			*(volatile unsigned long int *)
#endif

static inline uint32 regRead32(uint32 reg)
{
	return VPint(reg);
}

#define spiFlashBoot	 (VPint(CR_AHB_HWCONF) & (1<<4))

#define isEN7526c	(((VPint(0xbfb00064)&0xffff0000))==0x00080000)
#define isEN751221 	((((VPint(0xbfb00064)&0xffff0000))==0x00070000) || isEN7526c)

#define EFUSE_VERIFY_DATA0 (0xBFBF8214)
#define EFUSE_VERIFY_DATA1	(0xBFBF8218)
#define EFUSE_PKG_MASK_7516	(0xC0000)
#define EFUSE_PKG_MASK          (0x3F)
#define EFUSE_PKG_SEL_MASK      (0x3)
#define EFUSE_REMARK_BIT        (1<<6)

#define EFUSE_PKG_REMARK_SHITF 7

#define EFUSE_EN7526F   (0x0)
#define EFUSE_EN7521F   (0x10)
#define EFUSE_EN7521S   (0x20)
#define EFUSE_EN7512    (0x4)
#define EFUSE_EN7513    (0x5)
#define EFUSE_EN7513G   (0x6)
#define EFUSE_EN7516G	(0x80000)
#define EFUSE_EN7521G   (0x12)
#define EFUSE_EN7526D   (0x1)
#define EFUSE_EN7526G   (0x2)

#define EFUSE_EN7561G	(0xC0000)

#define EFUSE_EN7586    (0xA)
#define EFUSE_EN7586    (0xA)


#define EFUSE_REMARK_BIT_7516	(1 << 0)
#define EFUSE_PKG_REMARK_SHITF_7516	2
#define EFUSE_PKG_MASK		(0x3F)
#define EFUSE_REMARK_BIT	(1 << 6)
#define EFUSE_PKG_REMARK_SHITF	7



#define EFUSE_EN7527H		(0x0)
#define EFUSE_EN7527G		(0x0)


#define isEN7526F (isEN751221 && ( (regRead32(EFUSE_VERIFY_DATA0)&EFUSE_REMARK_BIT)? \
                        (((regRead32(EFUSE_VERIFY_DATA0)>>EFUSE_PKG_REMARK_SHITF)& EFUSE_PKG_MASK)== EFUSE_EN7526F): \
                        ((regRead32(EFUSE_VERIFY_DATA0)&EFUSE_PKG_MASK)==EFUSE_EN7526F)))
#define isEN7521F (isEN751221 && ( (regRead32(EFUSE_VERIFY_DATA0)&EFUSE_REMARK_BIT)? \
                        (((regRead32(EFUSE_VERIFY_DATA0)>>EFUSE_PKG_REMARK_SHITF)& EFUSE_PKG_MASK)== EFUSE_EN7521F): \
                        ((regRead32(EFUSE_VERIFY_DATA0)&EFUSE_PKG_MASK)==EFUSE_EN7521F)))
#define isEN7521S (isEN751221 && ( (regRead32(EFUSE_VERIFY_DATA0)&EFUSE_REMARK_BIT)? \
                        (((regRead32(EFUSE_VERIFY_DATA0)>>EFUSE_PKG_REMARK_SHITF)& EFUSE_PKG_MASK)== EFUSE_EN7521S): \
                        ((regRead32(EFUSE_VERIFY_DATA0)&EFUSE_PKG_MASK)==EFUSE_EN7521S)))
#define isEN7512 (isEN751221 && ( (regRead32(EFUSE_VERIFY_DATA0)&EFUSE_REMARK_BIT)? \
                        (((regRead32(EFUSE_VERIFY_DATA0)>>EFUSE_PKG_REMARK_SHITF)& EFUSE_PKG_MASK)== EFUSE_EN7512): \
                        ((regRead32(EFUSE_VERIFY_DATA0)&EFUSE_PKG_MASK)==EFUSE_EN7512)))
#define isEN7526D (isEN751221 && ( (regRead32(EFUSE_VERIFY_DATA0)&EFUSE_REMARK_BIT)? \
                        (((regRead32(EFUSE_VERIFY_DATA0)>>EFUSE_PKG_REMARK_SHITF)& EFUSE_PKG_MASK)== EFUSE_EN7526D): \
                        ((regRead32(EFUSE_VERIFY_DATA0)&EFUSE_PKG_MASK)==EFUSE_EN7526D)))
#define isEN7513 (isEN751221 && ( (regRead32(EFUSE_VERIFY_DATA0)&EFUSE_REMARK_BIT)? \
                        (((regRead32(EFUSE_VERIFY_DATA0)>>EFUSE_PKG_REMARK_SHITF)& EFUSE_PKG_MASK)== EFUSE_EN7513): \
                        ((regRead32(EFUSE_VERIFY_DATA0)&EFUSE_PKG_MASK)==EFUSE_EN7513)))
#define isEN7526G (isEN751221 && ( (regRead32(EFUSE_VERIFY_DATA0)&EFUSE_REMARK_BIT)? \
                        (((regRead32(EFUSE_VERIFY_DATA0)>>EFUSE_PKG_REMARK_SHITF)& EFUSE_PKG_MASK)== EFUSE_EN7526G): \
                        ((regRead32(EFUSE_VERIFY_DATA0)&EFUSE_PKG_MASK)==EFUSE_EN7526G)))
#define isEN7521G (isEN751221 && ( (regRead32(EFUSE_VERIFY_DATA0)&EFUSE_REMARK_BIT)? \
                        (((regRead32(EFUSE_VERIFY_DATA0)>>EFUSE_PKG_REMARK_SHITF)& EFUSE_PKG_MASK)== EFUSE_EN7521G): \
                        ((regRead32(EFUSE_VERIFY_DATA0)&EFUSE_PKG_MASK)==EFUSE_EN7521G)))
#define isEN7513G (isEN751221 && ( (regRead32(EFUSE_VERIFY_DATA0)&EFUSE_REMARK_BIT)? \
                        (((regRead32(EFUSE_VERIFY_DATA0)>>EFUSE_PKG_REMARK_SHITF)& EFUSE_PKG_MASK)== EFUSE_EN7513G): \
                        ((regRead32(EFUSE_VERIFY_DATA0)&EFUSE_PKG_MASK)==EFUSE_EN7513G)))
#define isEN7586 (isEN751221 && ( (regRead32(EFUSE_VERIFY_DATA0)&EFUSE_REMARK_BIT)? \
                        (((regRead32(EFUSE_VERIFY_DATA0)>>EFUSE_PKG_REMARK_SHITF)& EFUSE_PKG_MASK)== EFUSE_EN7586): \
                        ((regRead32(EFUSE_VERIFY_DATA0)&EFUSE_PKG_MASK)==EFUSE_EN7586)))
#define isLQFP	 (isEN751221 && ( (regRead32(EFUSE_VERIFY_DATA0)&EFUSE_REMARK_BIT)? \
							(((regRead32(EFUSE_VERIFY_DATA0)>>EFUSE_PKG_REMARK_SHITF)& EFUSE_PKG_SEL_MASK)== 0): \
							((regRead32(EFUSE_VERIFY_DATA0)&EFUSE_PKG_SEL_MASK)==0)))


#define isRT63365 		(((VPint(0xbfb00064) & 0xffff0000)) == 0x00040000)
#define isMT751020		(((VPint(0xbfb00064) & 0xffff0000)) == 0x00050000)
#define isMT7505		(((VPint(0xbfb00064) & 0xffff0000)) == 0x00060000)
#define isEN7526c		(((VPint(0xbfb00064) & 0xffff0000)) == 0x00080000)
#define isEN751221		((((VPint(0xbfb00064) & 0xffff0000)) == 0x00070000) || isEN7526c)
#define isEN7528		(((VPint(0xbfb00064) & 0xffff0000)) == 0x000B0000)
#ifdef CONFIG_ECONET_EN7528
#define isEN751627		((((VPint(0xbfb00064) & 0xffff0000)) == 0x00090000) || isEN7528)
#else
#define isEN751627		(((VPint(0xbfb00064) & 0xffff0000)) == 0x00090000)
#endif
#define isEN7580 		(((VPint(0xbfb00064) & 0xffff0000)) == 0x000A0000)

/* Support old xDSL chips */
#define isTC3162L2P2		((((unsigned char)(VPint(0xbfb0008c)>>12)&0xff)!=0)&&(((VPint(0xbfb00064)&0xffffffff))==0x00000000)?1:0)
#define isTC3162L3P3		((((unsigned char)(VPint(0xbfb0008c)>>12)&0xff)==7)&&(((VPint(0xbfb00064)&0xffffffff))==0x00000000)?1:0)
#define isTC3162L4P4		((((unsigned char)(VPint(0xbfb0008c)>>12)&0xff)==8)&&(((VPint(0xbfb00064)&0xffffffff))==0x00000000)?1:0)
#define isTC3162L5P5E2		((((unsigned char)(VPint(0xbfb0008c)>>12)&0xff)==0xa)&&(((VPint(0xbfb00064)&0xffffffff))==0x00000000)?1:0)
#define isTC3162L5P5E3		((((unsigned char)(VPint(0xbfb0008c)>>12)&0xff)==0xb)&&(((VPint(0xbfb00064)&0xffffffff))==0x00000000)?1:0)
#define isTC3162L5P5		(isTC3162L5P5E2 || isTC3162L5P5E3)
#define isTC3162U		((((unsigned char)(VPint(0xbfb0008c)>>12)&0xff)==0x10)&&(((VPint(0xbfb00064)&0xffffffff))==0x00000000)?1:0)
#define isRT63260		((((unsigned char)(VPint(0xbfb0008c)>>12)&0xff)==0x20)&&(((VPint(0xbfb00064)&0xffffffff))==0x00000000)?1:0)
#define isTC3169		(((VPint(0xbfb00064)&0xffff0000))==0x00000000)
#define isTC3182		(((VPint(0xbfb00064)&0xffff0000))==0x00010000)
#define isRT65168		(((VPint(0xbfb00064)&0xffff0000))==0x00020000)
#define isRT63165		(((VPint(0xbfb00064)&0xffff0000))==0x00030000)

#define PDIDR			(VPint(0xBFB0005C)&0xFFFF)
#define isEN751221FPGA		((VPint(0xBFB0008C) & (1 << 29)) ? 0 : 1) //used for 7512/7521
#define isGenernalFPGA		((VPint(0xBFB0008C) & (1 << 31)) ? 1 : 0) //used for 63365/751020
#define isGenernalFPGA_2	(((VPint(CR_AHB_SSTR) & 0x1) == 0) ? 1 : 0) //used for EN7526c and later version
#if defined(CONFIG_ECONET_EN7516) || \
    defined(CONFIG_ECONET_EN7527) || \
    defined(CONFIG_ECONET_EN7528)
#define isFPGA			0 // GET_IS_FPGA
#define SYNC_TYPE4()		__asm__ volatile ("sync 0x4")
#else
#define isFPGA			0 //(isEN7526c ? isGenernalFPGA_2 : (isEN751221 ? isEN751221FPGA : isGenernalFPGA))
#endif

#define isEN751627QFP		(((VPint(0xbfa20174) & 0x8000) == 0x8000) ? 1 : 0)


#define isEN7516G		(isEN751627 && \
				( (VPint(EFUSE_VERIFY_DATA0) & EFUSE_REMARK_BIT_7516)? \
				(((VPint(EFUSE_VERIFY_DATA0) >> EFUSE_PKG_REMARK_SHITF_7516) & EFUSE_PKG_MASK_7516) == EFUSE_EN7516G): \
				 ((VPint(EFUSE_VERIFY_DATA0) & EFUSE_PKG_MASK_7516) == EFUSE_EN7516G)))

#define isEN7561G		(isEN751627 && !isEN751627QFP && \
				( (VPint(EFUSE_VERIFY_DATA0) & EFUSE_REMARK_BIT_7516)? \
				(((VPint(EFUSE_VERIFY_DATA0) >> EFUSE_PKG_REMARK_SHITF_7516) & EFUSE_PKG_MASK_7516) == EFUSE_EN7561G): \
				 ((VPint(EFUSE_VERIFY_DATA0) & EFUSE_PKG_MASK_7516) == EFUSE_EN7516G)))

#define isEN7527H		(isEN751627 && isEN751627QFP && \
				( (VPint(EFUSE_VERIFY_DATA0) & EFUSE_REMARK_BIT_7516)? \
				(((VPint(EFUSE_VERIFY_DATA0) >> EFUSE_PKG_REMARK_SHITF_7516) & EFUSE_PKG_MASK_7516) == EFUSE_EN7527H): \
				 ((VPint(EFUSE_VERIFY_DATA0) & EFUSE_PKG_MASK_7516) == EFUSE_EN7527H)))

#define isEN7527G		(isEN751627 && !isEN751627QFP && \
				( (VPint(EFUSE_VERIFY_DATA0) & EFUSE_REMARK_BIT_7516)? \
				(((VPint(EFUSE_VERIFY_DATA0) >> EFUSE_PKG_REMARK_SHITF_7516) & EFUSE_PKG_MASK_7516) == EFUSE_EN7527G): \
				 ((VPint(EFUSE_VERIFY_DATA0) & EFUSE_PKG_MASK_7516) == EFUSE_EN7527G)))

#define isEN7528HU		(isEN7528 && (GET_PACKAGE_ID == 0x0))
#define isEN7528DU		(isEN7528 && (GET_PACKAGE_ID == 0x1))
#define isEN7561DU		(isEN7528 && (GET_PACKAGE_ID == 0x2))
#define isEN7526FH_EN7528DU	(isEN7528 && (GET_PACKAGE_ID == 0x3))
#define isEN7521G_EN7528DU	(isEN7528 && (GET_PACKAGE_ID == 0x7))

#define EFUSE_DDR3_BIT		(1 << 23)
#define EFUSE_DDR3_REMARK_BIT	(1 << 24)
#define EFUSE_IS_DDR3		( (VPint(EFUSE_VERIFY_DATA0) & EFUSE_REMARK_BIT)? \
				 ((VPint(EFUSE_VERIFY_DATA0) & EFUSE_DDR3_REMARK_BIT)): \
				 ((VPint(EFUSE_VERIFY_DATA0) & EFUSE_DDR3_BIT)))

#define REG_SAVE_INFO		0xBFB00284
#define GET_REG_SAVE_INFO_POINT	((volatile SYS_GLOBAL_PARM_T *)REG_SAVE_INFO)

typedef union {
	struct {
		uint32 packageID		:  4;
		uint32 isDDR4			:  1;
		uint32 isSecureHwTrapEn		:  1; /* detect secure HW trap */
		uint32 isSecureModeEn		:  1; /* RSA key has been written or not */
		uint32 isFlashBoot 		:  1;
		uint32 isCtrlEcc		:  1;
		uint32 isFpga			:  1;
		uint32 sys_clk			: 10; /* bus clock can support up to 1024MHz */
		uint32 dram_size		: 12; /* DRAM size can support up to 2048MB */
	} raw ;
	uint32 word;
} SYS_GLOBAL_PARM_T ;

#define GET_IS_DDR4			(GET_REG_SAVE_INFO_POINT->raw.isDDR4)
#define GET_DRAM_SIZE			(GET_REG_SAVE_INFO_POINT->raw.dram_size)
#define GET_SYS_CLK			(GET_REG_SAVE_INFO_POINT->raw.sys_clk)
#define GET_IS_FPGA			(GET_REG_SAVE_INFO_POINT->raw.isFpga)
#define GET_IS_SPI_ECC			(GET_REG_SAVE_INFO_POINT->raw.isCtrlEcc)
#define GET_PACKAGE_ID			(GET_REG_SAVE_INFO_POINT->raw.packageID)
#define GET_IS_SECURE_MODE		(GET_REG_SAVE_INFO_POINT->raw.isSecureModeEn)
#define GET_IS_SECURE_HWTRAP		(GET_REG_SAVE_INFO_POINT->raw.isSecureHwTrapEn)

#define SYS_HCLK		(GET_SYS_CLK)
#define SAR_CLK			((SYS_HCLK)/(4.0))		//more accurate if 4.0 not 4

/* define CPU timer clock, FPGA is 50Mhz, ASIC is 200Mhz */
#define	CPUTMR_CLK		(isFPGA ? (50*1000000) : (200*1000000))

#define isMT7530		(((VPint(0xbfb58000 + 0x7ffc) & 0xffff0000)) == 0x75300000)

#define DSPRAM_BASE		0x9c000000

#define WAN2LAN_CH_ID	(1<<31)

#define IS_SPIFLASH			((~(VPint(0xBFA10114))) & 0x2)
#define IS_NANDFLASH			   (VPint(0xBFA10114)   & 0x2)
#ifdef TCSUPPORT_SPI_CONTROLLER_ECC
#define isSpiControllerECC		(GET_IS_SPI_ECC)
#else
#define isSpiControllerECC		(0)
#endif
#define isSpiNandAndCtrlECC		(IS_NANDFLASH && isSpiControllerECC)
