#include "NuMicro.h"
#include "secure_update.h"
#include "secure_isp_ram.h"

static uint32_t s_pressed_since;
static uint8_t s_pressed;
static uint8_t s_handled;
static volatile uint32_t s_requested;

void SecureUpdate_Request(void)
{
    s_requested = 1UL;
}

static uint32_t SecureUpdate_TakeRequest(void)
{
    uint32_t primask;
    uint32_t requested;

    primask = __get_PRIMASK();
    __disable_irq();
    requested = s_requested;
    s_requested = 0UL;
    __set_PRIMASK(primask);
    return requested;
}

/* All APROM work ends before the dedicated SRAM stack/vector handoff. */
__attribute__((noinline)) static void SecureUpdate_Transfer(void)
{
    uint32_t i;
    uint32_t irq_banks;

    __disable_irq();
    SysTick->CTRL = 0UL;
    TIMER_Stop(TIMER1);
    TIMER_DisableInt(TIMER1);
    UART0->INTEN = 0UL;

    irq_banks = (SCnSCB->ICTR & SCnSCB_ICTR_INTLINESNUM_Msk) + 1UL;
    for (i = 0UL; i < irq_banks; i++)
    {
        NVIC->ICER[i] = 0xFFFFFFFFUL;
        NVIC->ICPR[i] = 0xFFFFFFFFUL;
    }
    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk | SCB_ICSR_PENDSVCLR_Msk;

    /* Match the original loader's protected-register/FMC API preconditions. */
    SYS_UnlockReg();
    CLK->AHBCLK |= CLK_AHBCLK_ISPCKEN_Msk;
    FMC_Open();
    g_secure_isp_stage = 1UL;
    __DSB();
    __ISB();
    SecureIspRam_Transfer();
}

static int32_t SecureUpdate_PrepareUsb(void)
{
    uint32_t locked;
    volatile uint32_t delay;

    locked = SYS_IsRegLocked();
    SYS_UnlockReg();
    PF->MODE &= ~(GPIO_MODE_MODE2_Msk | GPIO_MODE_MODE3_Msk);
    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);
    if ((CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk) == 0UL) ||
        (CLK_SetCoreClock(FREQ_192MHZ) != FREQ_192MHZ))
    {
        if (locked != 0UL)
        {
            SYS_LockReg();
        }
        return SECURE_UPDATE_ERROR_CLOCK;
    }
    CLK->PCLKDIV = CLK_PCLKDIV_APB0DIV_DIV2 | CLK_PCLKDIV_APB1DIV_DIV2;
    CLK_EnableModuleClock(HSUSBD_MODULE);
    SYS_ResetModule(HSUSBD_RST);
    SYS->USBPHY = (SYS->USBPHY &
                   ~(SYS_USBPHY_HSUSBROLE_Msk | SYS_USBPHY_HSUSBACT_Msk)) |
                  SYS_USBPHY_HSUSBEN_Msk;
    for (delay = 0UL; delay < 0x1000UL; delay++)
    {
    }
    SYS->USBPHY |= SYS_USBPHY_HSUSBACT_Msk;
    NVIC_DisableIRQ(USBD20_IRQn);
    NVIC_ClearPendingIRQ(USBD20_IRQn);
    if (locked != 0UL)
    {
        SYS_LockReg();
    }
    return SECURE_UPDATE_OK;
}

static int32_t SecureUpdate_Enter(void)
{
    uint32_t status;
    uint32_t stack;
    uint32_t vector;
    uint32_t i;
    uint32_t locked;
    uint32_t config[2];
    int32_t result;

    if ((__get_IPSR() != 0UL) || ((__get_CONTROL() & 1UL) != 0UL))
    {
        return SECURE_UPDATE_ERROR_CONTEXT;
    }

    status = FMC->ISPSTS;
    /* Mask ROM boot enables IAP; direct APROM boot requires CBS[0] == 0. */
    if (((status & FMC_ISPSTS_MBS_Msk) != 0UL) &&
        ((status & (1UL << FMC_ISPSTS_CBS_Pos)) != 0UL))
    {
        return SECURE_UPDATE_ERROR_IAP;
    }

    locked = SYS_IsRegLocked();
    SYS_UnlockReg();
    CLK->AHBCLK |= CLK_AHBCLK_ISPCKEN_Msk;
    FMC_Open();
    result = FMC_ReadConfig(config, 2UL);
    if (locked != 0UL)
    {
        SYS_LockReg();
    }
    /* Keep the IB in APROM; this example does not manage an LDROM IB page. */
    if ((result != 0) || ((config[0] & 0x80UL) == 0UL))
    {
        return SECURE_UPDATE_ERROR_CONFIG;
    }

    stack = __isp_vectors[0];
    if (((uint32_t)__isp_vectors != 0x20000000UL) ||
        ((stack & 7UL) != 0UL) || (stack <= 0x20002000UL) ||
        (stack > 0x20004000UL))
    {
        return SECURE_UPDATE_ERROR_RAM_IMAGE;
    }
    for (i = 1UL; i < 128UL; i++)
    {
        vector = __isp_vectors[i];
        if (((vector & 1UL) == 0UL) ||
            ((vector & ~1UL) < 0x20000200UL) ||
            ((vector & ~1UL) >= 0x20002000UL))
        {
            return SECURE_UPDATE_ERROR_RAM_IMAGE;
        }
    }

    result = SecureUpdate_PrepareUsb();
    if (result != SECURE_UPDATE_OK)
    {
        return result;
    }

    SecureUpdate_Transfer();
    return SECURE_UPDATE_OK;
}

void SecureUpdate_Init(void)
{
    uint32_t locked;

    locked = SYS_IsRegLocked();
    SYS_UnlockReg();
    CLK->AHBCLK |= CLK_AHBCLK_ISPCKEN_Msk;
    SYS->GPG_MFPH = (SYS->GPG_MFPH & ~SYS_GPG_MFPH_PG15MFP_Msk) |
                    SYS_GPG_MFPH_PG15MFP_GPIO;
    GPIO_SetMode(PG, BIT15, GPIO_MODE_INPUT);
    GPIO_SetPullCtl(PG, BIT15, GPIO_PUSEL_PULL_UP);
    PG->DINOFF &= ~(BIT15 << 16);
    if (locked != 0UL)
    {
        SYS_LockReg();
    }

    s_pressed_since = 0UL;
    s_pressed = 0U;
    s_handled = 0U;
    s_requested = 0UL;
}

int32_t SecureUpdate_Poll(uint32_t now_ms)
{
    if (SecureUpdate_TakeRequest() != 0UL)
    {
        return SecureUpdate_Enter();
    }

    if (PG15 != 0UL)
    {
        s_pressed = 0U;
        s_handled = 0U;
        return SECURE_UPDATE_OK;
    }

    if (s_pressed == 0U)
    {
        s_pressed = 1U;
        s_pressed_since = now_ms;
    }

    if ((s_handled == 0U) &&
        ((uint32_t)(now_ms - s_pressed_since) >= SECURE_UPDATE_DEBOUNCE_MS))
    {
        s_handled = 1U;
        return SecureUpdate_Enter();
    }
    return SECURE_UPDATE_OK;
}
