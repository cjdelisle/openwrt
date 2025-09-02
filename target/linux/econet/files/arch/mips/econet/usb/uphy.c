#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>


#define PHYS_TO_K1(physaddr)		KSEG1ADDR(physaddr)
#define sysRegRead(phys)		(*(volatile unsigned int *)PHYS_TO_K1(phys))
#define sysRegWrite(phys, val)		((*(volatile unsigned int *)PHYS_TO_K1(phys)) = (val))
#define VPint			*(volatile unsigned long int *)
#define CONFIG_ECONET_EN7512

///

#define RALINK_XHCI_UPHY_BASE		0xBFA80000
#define RALINK_USB_UPHY_P0_BASE		(RALINK_XHCI_UPHY_BASE + 0x0800)
#define RALINK_USB_UPHY_P1_BASE		(RALINK_XHCI_UPHY_BASE + 0x1000)
#define CR_AHB_BASE       	0xBFB00000
#define CR_AHB_HWCONF       (CR_AHB_BASE + 0x8C)


///

#define ADDR_SIFSLV_BASE		0xBFA80000
#define ADDR_SIFSLV_FMREG_BASE		(ADDR_SIFSLV_BASE + 0x0100)
#define ADDR_SIFSLV_PHYD_BASE		(ADDR_SIFSLV_BASE + 0x0900)
#define ADDR_SIFSLV_PHYD_B2_BASE	(ADDR_SIFSLV_BASE + 0x0a00)
#define ADDR_SIFSLV_PHYA_BASE		(ADDR_SIFSLV_BASE + 0x0b00)
#define ADDR_SIFSLV_PHYA_DA_BASE	(ADDR_SIFSLV_BASE + 0x0c00)

#define ADDR_U2_PHY_P0_BASE		RALINK_USB_UPHY_P0_BASE
#define ADDR_U2_PHY_P1_BASE		RALINK_USB_UPHY_P1_BASE

#define U2_SR_COEFF			28
#define REF_CK				20

#define REG_SIFSLV_FMREG_FMCR0		(ADDR_SIFSLV_FMREG_BASE + 0x00)
#define REG_SIFSLV_FMREG_FMCR1		(ADDR_SIFSLV_FMREG_BASE + 0x04)
#define REG_SIFSLV_FMREG_FMCR2		(ADDR_SIFSLV_FMREG_BASE + 0x08)
#define REG_SIFSLV_FMREG_FMMONR0	(ADDR_SIFSLV_FMREG_BASE + 0x0C)
#define REG_SIFSLV_FMREG_FMMONR1	(ADDR_SIFSLV_FMREG_BASE + 0x10)

/* SIFSLV_FMREG_FMCR0 */
#define RG_LOCKTH			(0xf << 28)
#define RG_MONCLK_SEL			(0x3 << 26)
#define RG_FM_MODE			(0x1 << 25)
#define RG_FREQDET_EN			(0x1 << 24)
#define RG_CYCLECNT			(0x00ffffff)

/* SIFSLV_FMREG_FMMONR1 */
#define RG_MONCLK_SEL_3			(0x1 << 9)
#define RG_FRCK_EN			(0x1 << 8)
#define USBPLL_LOCK			(0x1 << 1)
#define USB_FM_VLD			(0x1 << 0)

#define OFS_U2_PHY_AC0			0x00
#define OFS_U2_PHY_AC1			0x04
#define OFS_U2_PHY_AC2			0x08
#define OFS_U2_PHY_ACR0			0x10
#define OFS_U2_PHY_ACR1			0x14
#define OFS_U2_PHY_ACR2			0x18
#define OFS_U2_PHY_ACR3			0x1C
#define OFS_U2_PHY_ACR4			0x20
#define OFS_U2_PHY_AMON0		0x24
#define OFS_U2_PHY_DCR0			0x60
#define OFS_U2_PHY_DCR1			0x64
#define OFS_U2_PHY_DTM0			0x68
#define OFS_U2_PHY_DTM1			0x6C

/* U2_PHY_ACR0 */
#define RG_USB20_ICUSB_EN		(0x1 << 24)
#define RG_USB20_HSTX_SRCAL_EN		(0x1 << 23)
#define RG_USB20_HSTX_SRCTRL		(0x7 << 16)
#define RG_USB20_LS_CR			(0x7 << 12)
#define RG_USB20_FS_CR			(0x7 << 8)
#define RG_USB20_LS_SR			(0x7 << 4)
#define RG_USB20_FS_SR			(0x7 << 0)

static atomic_t uphy_init_instance = ATOMIC_INIT(0);

static inline u32
uphy_read32(u32 reg_addr)
{
	return sysRegRead(reg_addr);
}

static inline void
uphy_write32(u32 reg_addr, u32 reg_data)
{
	sysRegWrite(reg_addr, reg_data);
}

static void
uphy_write8(u32 reg_addr, u32 reg_data)
{
	u32 reg_sfl = (reg_addr % 4) * 8;
	u32 reg_a32 = reg_addr & 0xfffffffc;
	u32 reg_msk = 0xff << reg_sfl;
	u32 reg_tmp = uphy_read32(reg_a32);

	reg_tmp &= ~reg_msk;
	reg_tmp |= ((reg_data << reg_sfl) & reg_msk);

	uphy_write32(reg_a32, reg_tmp);
}

static void
u2_slew_rate_calibration(int port_id, u32 u2_phy_reg_base)
{
	u32 reg_val, i;
	u32 u4Tmp, u4FmOut = 0;

	/* enable HS TX SR calibration */
	reg_val = uphy_read32(u2_phy_reg_base + OFS_U2_PHY_ACR0);
	reg_val |= RG_USB20_HSTX_SRCAL_EN;
	uphy_write32(u2_phy_reg_base + OFS_U2_PHY_ACR0, reg_val);
	msleep(1);

	/* enable free run clock */
	reg_val = uphy_read32(REG_SIFSLV_FMREG_FMMONR1);
	reg_val |= RG_FRCK_EN;
	uphy_write32(REG_SIFSLV_FMREG_FMMONR1, reg_val);

	/* setting MONCLK_SEL 0x0/0x1 for port0/port1 */
	reg_val = uphy_read32(REG_SIFSLV_FMREG_FMCR0);
	reg_val &= ~RG_MONCLK_SEL;
	reg_val |= (port_id << 26);
	uphy_write32(REG_SIFSLV_FMREG_FMCR0, reg_val);

	/* setting cyclecnt = 400 */
	reg_val = uphy_read32(REG_SIFSLV_FMREG_FMCR0);
	reg_val &= ~RG_CYCLECNT;
	reg_val |= 0x400;
	uphy_write32(REG_SIFSLV_FMREG_FMCR0, reg_val);

	/* enable frequency meter */
	reg_val = uphy_read32(REG_SIFSLV_FMREG_FMCR0);
	reg_val |= RG_FREQDET_EN;
	uphy_write32(REG_SIFSLV_FMREG_FMCR0, reg_val);

	/* wait for FM detection done, set 10ms timeout */
	for (i = 0; i < 10; i++) {
		/* read FM_OUT */
		u4FmOut = uphy_read32(REG_SIFSLV_FMREG_FMMONR0);

		/* check if FM detection done */
		if (u4FmOut != 0)
			break;

		msleep(1);
	}

	/* disable frequency meter */
	reg_val = uphy_read32(REG_SIFSLV_FMREG_FMCR0);
	reg_val &= ~RG_FREQDET_EN;
	uphy_write32(REG_SIFSLV_FMREG_FMCR0, reg_val);

	/* disable free run clock */
	reg_val = uphy_read32(REG_SIFSLV_FMREG_FMMONR1);
	reg_val &= ~RG_FRCK_EN;
	uphy_write32(REG_SIFSLV_FMREG_FMMONR1, reg_val);

	/* disable HS TX SR calibration */
	reg_val = uphy_read32(u2_phy_reg_base + OFS_U2_PHY_ACR0);
	reg_val &= ~RG_USB20_HSTX_SRCAL_EN;
	uphy_write32(u2_phy_reg_base + OFS_U2_PHY_ACR0, reg_val);
	msleep(1);

	/* update RG_USB20_HSTX_SRCTRL */
	reg_val = uphy_read32(u2_phy_reg_base + OFS_U2_PHY_ACR0);
	reg_val &= ~RG_USB20_HSTX_SRCTRL;
	if (u4FmOut != 0) {
		/* set reg = (1024/FM_OUT) * 20 * 0.028 (round to the nearest digits) */
		u4Tmp = (((1024 * REF_CK * U2_SR_COEFF) / u4FmOut) + 500) / 1000;
		reg_val |= ((u4Tmp & 0x07) << 16);
		printk(KERN_INFO "U2PHY P%d set SRCTRL %s value: %d\n", port_id, "calibration", u4Tmp);
	} else {
		reg_val |= (0x4 << 16);
		printk(KERN_INFO "U2PHY P%d set SRCTRL %s value: %d\n", port_id, "default", 4);
	}
	uphy_write32(u2_phy_reg_base + OFS_U2_PHY_ACR0, reg_val);
}

static inline void
u2_phy_init(u32 u2_phy_reg_base)
{
	u32 reg_val;

	/* set SW PLL Stable mode to 1 for U2 LPM device remote wakeup */
	reg_val = uphy_read32(u2_phy_reg_base + OFS_U2_PHY_DCR1);
	reg_val &= ~(0x3 << 18);
	reg_val |=  (0x1 << 18);
	uphy_write32(u2_phy_reg_base + OFS_U2_PHY_DCR1, reg_val);
}

// This function is called with the crystal is 25Mhz rather than 20Mhz
// as determined by reading a hardware configuration register.
static inline void
uphy_setup_25mhz_xtal(void)
{
	// ADDR_SIFSLV_PHYA_DA_BASE = 0xBFA80C00 // SSUSB SIFSLV U3PHYA DA Register Summary
	// MACHINE IS BIG ENDIAN

	// BFA80C1C reg9 DA_SSUSB_PLL_FBKDIV
	// 6:0 RG_SSUSB_PLL_FBKDIV_U3
	uphy_write8(ADDR_SIFSLV_PHYA_DA_BASE + 0x1c, 0x18);
	// 14:8 RG_SSUSB_PLL_FBKDIV_PE1H
	uphy_write8(ADDR_SIFSLV_PHYA_DA_BASE + 0x1d, 0x18);
	// 22:16 RG_SSUSB_PLL_FBKDIV_PE1D (unused)
	// 30:24 RG_SSUSB_PLL_FBKDIV_PE2H
	uphy_write8(ADDR_SIFSLV_PHYA_DA_BASE + 0x1f, 0x18);

	// BFA80C24 reg12 DA_SSUSB_PLL_PCW_NCPO
    // 30:0 RG_SSUSB_PLL_PCW_NCPO_U3
	uphy_write32(ADDR_SIFSLV_PHYA_DA_BASE + 0x24, 0x18000000);

	// BFA80C28 reg13
	// 30:0 RG_SSUSB_PLL_PCW_NCPO_PE1H
	uphy_write32(ADDR_SIFSLV_PHYA_DA_BASE + 0x28, 0x18000000);

	// BFA80C30 reg15
	// 30:0 RG_SSUSB_PLL_PCW_NCPO_PE2H
	uphy_write32(ADDR_SIFSLV_PHYA_DA_BASE + 0x30, 0x18000000);

	// BFA80C38 reg19 DA_SSUSB_PLL_SSC_DELTA1
	// 31:16 RG_SSUSB_PLL_SSC_DELTA1_PE1H -> 0x004a
    // 15:0 RG_SSUSB_PLL_SSC_DELTA1_U3    -> 0x004a
	uphy_write32(ADDR_SIFSLV_PHYA_DA_BASE + 0x38, 0x004a004a);

	// BFA80C3C reg20
	// 31:16 RG_SSUSB_PLL_SSC_DELTA1_PE2H
    // 15:0 RG_SSUSB_PLL_SSC_DELTA1_PE1D -> 0x4a00
	uphy_write8(ADDR_SIFSLV_PHYA_DA_BASE + 0x3e, 0x4a); // reg20
	uphy_write8(ADDR_SIFSLV_PHYA_DA_BASE + 0x3f, 0x0);  // reg20

	// BFA80C40 reg21
	// 31:16 RG_SSUSB_PLL_SSC_DELTA_U3
    // 15:0 RG_SSUSB_PLL_SSC_DELTA1_PE2D -> 0x4800
	uphy_write8(ADDR_SIFSLV_PHYA_DA_BASE + 0x42, 0x48); // reg21
	uphy_write8(ADDR_SIFSLV_PHYA_DA_BASE + 0x43, 0x0);  // reg21

	// BFA80C44 reg23
	// 31:16 RG_SSUSB_PLL_SSC_DELTA_PE1D
	// 15:0 RG_SSUSB_PLL_SSC_DELTA_PE1H -> 0x4800
	uphy_write8(ADDR_SIFSLV_PHYA_DA_BASE + 0x44, 0x48); // reg23
	uphy_write8(ADDR_SIFSLV_PHYA_DA_BASE + 0x45, 0x0);  // reg23

	// BFA80C48 reg25
	// 31:16 RG_SSUSB_PLL_SSC_DELTA_PE2D
	// 15:0 RG_SSUSB_PLL_SSC_DELTA_PE2H -> 0x4800
	uphy_write8(ADDR_SIFSLV_PHYA_DA_BASE + 0x48, 0x48);
	uphy_write8(ADDR_SIFSLV_PHYA_DA_BASE + 0x49, 0x0);

	// ADDR_SIFSLV_PHYA_BASE = 0xBFA80B00 // SSUSB SIFSLV U3PHYA Register Summary

	// BFA80B24 reg9
	// 31:16 RG_SSUSB_PLL_DDS_DMY - DDS dummy registers
	// 15:0 RG_SSUSB_PLL_SSC_PRD - DDS SSC dither period control
	uphy_write8(ADDR_SIFSLV_PHYA_BASE + 0x24, 0x90);
	uphy_write8(ADDR_SIFSLV_PHYA_BASE + 0x25, 0x1);

	// BFA80B10 reg4
	// 31:1 RG_SSUSB_SYSPLL_PCW_NCPO - DDS NCPO PCW (default: /24)
	uphy_write32(ADDR_SIFSLV_PHYA_BASE + 0x10, 0x1c000000);

	// BFA80B08 reg2
	// 31 RG_SSUSB_SYSPLL_LF - Enable PLL low-frequency mode
	//   - 0: Disable
	//   - 1'b1: Enable
	// 30:24 RG_SSUSB_SYSPLL_FBDIV - Feedback divide ratio
	//   - 7'd0: /1
	//   - 7'd1: /2
	// 23:22 RG_SSUSB_SYSPLL_POSDIV - Post-divider ratio for single-end clock output
	//   - 7'd127: /128
	//   - 2'b00: VCO/1
	//   - 2'b01: VCO/2
	//   - 2'b1X: VCO/4
	// 21 RG_SSUSB_SYSPLL_VCO_DIV_SEL - Select VCO div2
	//   - 1'b0: /1
	//   - 1'b1: /2
	// 20 RG_SSUSB_SYSPLL_BLP - LPF Bandiwdth selection
	//   - 1'b0: 40MHz
	//   - 1'b1: 20MHz
	// 19 RG_SSUSB_SYSPLL_BP - Capacitance adjustment for Bandiwdth
	//   - 1'b0: When RG_PLL_BR=1'b0
	//   - 1'b1: When RG_PLL_BR=1'b1
	// 18 RG_SSUSB_SYSPLL_BR - Resistance adjustment for Bandwidth
	//   - 1'b0: BW = Fref/10
	//   - 1'b1: BW = Fref/20
	// 17 RG_SSUSB_SYSPLL_BC - OP current reduce
	//   - 1'b0: 10uA
	//   - 1'b1: 5uA
	// 16:14 RG_SSUSB_SYSPLL_DIVEN - Time domain cap multiplication ratio
	//   - 3'd0: x1
	//   - 3'd1: x2
	//   - 3'd6: x64
	// 13 RG_SSUSB_SYSPLL_FPEN - Enable PLL 4-phase clock
	//   - 0: Disable
	//   - 1'b1: Enable
	// 12 RG_SSUSB_SYSPLL_MONCK_EN - Enable monitor VCO clock for debug
	//   - 1'b0: Disable
	//   - 1'b1: Enable
	// 11 RG_SSUSB_SYSPLL_MONVC_EN - Enable monitor Vctrl Voltage for debug
	//   - 1'b0: Disable
	//   - 1'b1: Enable
	// 10 RG_SSUSB_SYSPLL_MONREF_EN - Enable PFD Clock Out for Frequency Meter
	//   - 1'b0: Disable
	//   - 1'b1: Enable
	// 9 RG_SSUSB_SYSPLL_VOD_EN - CHP OverDrive Enable
	//   - 1'b0: Disable
	//   - 1'b1: Enable
	// 8 RG_SSUSB_SYSPLL_CK_SEL - SYSPLL clock selection
	//   - 1'b0: SYSPLL
	//   - 1'b1: top clock
	//
	// Bits 7:0 undocument/unused - does this do anything at all?
	uphy_write8(ADDR_SIFSLV_PHYA_BASE + 0x0b, 0xe);
}

void uphy_init(void);
void uphy_init(void)
{
	u32 reg_val;

	if (atomic_inc_return(&uphy_init_instance) != 1)
		return;

#if defined(CONFIG_ECONET_EN7512)

	/* patch TxDetRx Timing for E1, from DR 20160421, Biker_20160516 */
	reg_val = uphy_read32(ADDR_SIFSLV_PHYD_B2_BASE + 0x28); // 0xBFA80a28
	reg_val &= ~(0x1ff << 9);
	reg_val |=  (0x010 << 9);
	uphy_write32(ADDR_SIFSLV_PHYD_B2_BASE + 0x28, reg_val);

	reg_val = uphy_read32(ADDR_SIFSLV_PHYD_B2_BASE + 0x2c); // 0xBFA80a2c
	reg_val &= ~0x1ff;
	reg_val |=  0x010;
	uphy_write32(ADDR_SIFSLV_PHYD_B2_BASE + 0x2c, reg_val);

	/* patch LFPS Filter Threshold for E1, from DR 20160421, Biker_20160516 */
	reg_val = uphy_read32(ADDR_SIFSLV_PHYD_BASE + 0x0c);
	reg_val &= ~(0x3f << 16);
	reg_val |=  (0x34 << 16);
	uphy_write32(ADDR_SIFSLV_PHYD_BASE + 0x0c, reg_val);

	/* configure for XTAL 25MHz */
	if (VPint(CR_AHB_HWCONF) & 0x01)
		uphy_setup_25mhz_xtal();

	// if (isEN7513 || isEN7513G) {
		printk(KERN_INFO "%s USB PHY config\n", "EN7513 (BGA)");

		uphy_write32(ADDR_U2_PHY_P0_BASE + 0x1c, 0xC0240008); /* enable port 0 */
		uphy_write32(ADDR_U2_PHY_P1_BASE + 0x1c, 0xC0240000); /* enable port 1 */
	// } else if (isEN7512) {
	// 	printk(KERN_INFO "%s USB PHY config\n", "EN7512 (QFP)");

	// 	uphy_write32(ADDR_U2_PHY_P0_BASE + 0x1c, 0xC0241580); /* disable port 0 */
	// 	uphy_write32(ADDR_U2_PHY_P1_BASE + 0x1c, 0xC0240000); /* enable port 1 */
	// }

#elif defined(CONFIG_ECONET_EN7516) || \
      defined(CONFIG_ECONET_EN7527)

	/* configure for XTAL 25MHz */
	reg_val = VPint(CR_AHB_HWCONF);
	if (!(reg_val & 0x40000))
		uphy_setup_25mhz_xtal();

	uphy_write32(ADDR_U2_PHY_P0_BASE + 0x1c, 0xC0240008); /* enable port 0 */
	uphy_write32(ADDR_U2_PHY_P1_BASE + 0x1c, 0xC0240000); /* enable port 1 */

	printk(KERN_INFO "%s USB PHY config\n", "EN7516/EN7527");

#elif defined(CONFIG_ECONET_EN7528)

	uphy_write32(ADDR_U2_PHY_P0_BASE + 0x1c, 0xC0240000); /* enable port 0 */
	uphy_write32(ADDR_U2_PHY_P1_BASE + 0x1c, 0xC0240000); /* enable port 1 */

	/* combo phy Rx R FT mean value too high, tune target R -5 Ohm */
	reg_val = uphy_read32(ADDR_SIFSLV_PHYA_BASE + 0x2c);
	reg_val &= ~(0x3 << 12);
	reg_val |=  (0x1 << 12);
	uphy_write32(ADDR_SIFSLV_PHYA_BASE + 0x2c, reg_val);

	printk(KERN_INFO "%s USB PHY config\n", "EN7528");

#endif

#if !defined(CONFIG_ECONET_EN7528)
	/* init UPHY */
	u2_phy_init(ADDR_U2_PHY_P0_BASE); // 0xBFA80800
	u2_phy_init(ADDR_U2_PHY_P1_BASE); // 0xBFA81000
#endif

	/* calibrate UPHY */
	u2_slew_rate_calibration(0, ADDR_U2_PHY_P0_BASE);
	u2_slew_rate_calibration(1, ADDR_U2_PHY_P1_BASE);
}
