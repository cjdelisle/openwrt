#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/irqdomain.h>

#include <asm/irq.h>

#define XHCI_MTK_DRV_NAME	"xhci-mtk"
#define RALINK_USB_HOST_BASE		0x1FB90000
#define RALINK_USB_HOST_SIZE		0x00004000
#define RALINK_USB_IPPC_BASE		0x1FA80700
#define SURFBOARDINT_USB 			17

struct xhci_mtk_pdata {
	void (*uphy_init)(void);

	bool	usb3_lpm_capable;
};

static struct resource en75xx_xhci_resources[] = {
	[0] = {
		.start  = RALINK_USB_HOST_BASE,
		.end    = RALINK_USB_HOST_BASE + RALINK_USB_HOST_SIZE - 1,
		.flags  = IORESOURCE_MEM,
		.name   = "mac",
	},
	[1] = {
		.start  = RALINK_USB_IPPC_BASE,
		.end    = RALINK_USB_IPPC_BASE + 0xff,
		.flags  = IORESOURCE_MEM,
		.name   = "ippc",
	},
	[2] = {
		.start  = SURFBOARDINT_USB,
		.flags  = IORESOURCE_IRQ,
	},
};

static u64 en75xx_xhci_dmamask = DMA_BIT_MASK(32);

extern void uphy_init(void);

static struct xhci_mtk_pdata en75xx_xhci_pdata = {
	.uphy_init		= uphy_init,
	.usb3_lpm_capable	= true,
};

static struct platform_device en75xx_xhci_device = {
	.name		= XHCI_MTK_DRV_NAME,
	.id		= -1,
	.dev		= {
		.platform_data = &en75xx_xhci_pdata,
		.dma_mask = &en75xx_xhci_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
	.num_resources	= ARRAY_SIZE(en75xx_xhci_resources),
	.resource	= en75xx_xhci_resources,
};

int __init init_en75xx_xhci(void);
int __init init_en75xx_xhci(void)
{
	struct device_node *np;
	struct irq_domain *dom;
	int retval;
	int virq;

	np = of_find_compatible_node(NULL, NULL, "econet,en751221-intc");
	if (!np) {
		pr_err("Failed to find intc node\n");
		return -ENODEV;
	}

	dom = irq_find_host(np);
	of_node_put(np);

	if (!dom) {
		pr_err("Failed to find irq_domain for intc\n");
		return -ENODEV;
	}

    virq = irq_create_mapping(dom, SURFBOARDINT_USB);
    if (!virq) {
        pr_err("Failed to map IRQ %d\n", SURFBOARDINT_USB);
        return -EINVAL;
    }
	en75xx_xhci_resources[2].start = virq;

	retval = platform_device_register(&en75xx_xhci_device);
	if (retval != 0) {
		printk(KERN_ERR "register %s device fail!\n", "xHCI");
		return retval;
	}

    printk(KERN_ERR "xHCI registered!\n");

	return retval;
}

device_initcall(init_en75xx_xhci);
