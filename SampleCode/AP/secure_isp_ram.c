#include "NuMicro.h"
#include "BootIspLib.h"
#include "secure_isp_ram.h"

/* The scatter file places this entire object and BootIspLib in SRAM. */
volatile uint32_t g_secure_isp_stage;

void SecureIspRam_Stop(void)
{
    __disable_irq();
    g_secure_isp_stage = 0xE001UL;
    while (1)
    {
        __WFI();
    }
}

void SecureIspRam_UsbIrq(void)
{
    HSUSBD_ISR();
}

void SecureIspRam_Run(void)
{
    g_secure_isp_stage = 2UL;
    __set_BASEPRI(0UL);
    __set_FAULTMASK(0UL);
    __DSB();
    __ISB();
    __enable_irq();
    HSUSBD_ISP();

    /* Follow the BSP: keep servicing USB interrupts if the ROM API returns. */
    g_secure_isp_stage = 3UL;
    while (1)
    {
        __WFI();
    }
}
