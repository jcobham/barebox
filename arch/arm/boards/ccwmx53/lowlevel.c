#include <common.h>
#include <mach/esdctl.h>
#include <mach/generic.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <mach/imx53-regs.h>

void __naked barebox_arm_reset_vector(void)
{
	imx5_cpu_lowlevel_init();
	barebox_arm_entry(MX53_CSD0_BASE_ADDR, SZ_512M, NULL);
}
