#ifndef SECURE_ISP_RAM_H
#define SECURE_ISP_RAM_H

#include <stdint.h>

/* These symbols are defined by the dedicated SRAM assembly module. */
extern const uint32_t __isp_vectors[128];
extern uint32_t __isp_stack_top;
extern volatile uint32_t g_secure_isp_stage;

__attribute__((noreturn)) void SecureIspRam_Transfer(void);
__attribute__((noreturn)) void SecureIspRam_Run(void);
__attribute__((noreturn)) void SecureIspRam_Stop(void);
void SecureIspRam_UsbIrq(void);

#endif
