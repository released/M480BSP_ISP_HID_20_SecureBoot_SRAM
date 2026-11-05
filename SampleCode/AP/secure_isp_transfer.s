                PRESERVE8
                THUMB

                AREA    ISP_STACK, NOINIT, READWRITE, ALIGN=3
                EXPORT  __isp_stack_bottom
                EXPORT  __isp_stack_top
__isp_stack_bottom
                SPACE   0x1000
__isp_stack_top

                AREA    ISP_VECTORS, DATA, READONLY, ALIGN=9
                EXPORT  __isp_vectors
                IMPORT  SecureIspRam_Stop
                IMPORT  SecureIspRam_UsbIrq
                GBLA    VectorIndex
__isp_vectors
                DCD     __isp_stack_top
                DCD     SecureIspRam_Stop
VectorIndex     SETA    2
                WHILE   VectorIndex < 81
                DCD     SecureIspRam_Stop
VectorIndex     SETA    VectorIndex + 1
                WEND
                ; USBD20_IRQn is 65, so its vector index is 16 + 65.
                DCD     SecureIspRam_UsbIrq
VectorIndex     SETA    82
                WHILE   VectorIndex < 128
                DCD     SecureIspRam_Stop
VectorIndex     SETA    VectorIndex + 1
                WEND

                AREA    ISP_CODE, CODE, READONLY
                EXPORT  SecureIspRam_Transfer
                IMPORT  SecureIspRam_Run
SecureIspRam_Transfer PROC
                CPSID   I
                LDR     r0, =__isp_vectors
                LDR     r1, =0xE000ED08
                STR     r0, [r1]
                DSB
                ISB
                LDR     r0, =__isp_stack_top
                MSR     MSP, r0
                MOVS    r0, #0
                MSR     CONTROL, r0
                ISB
                MSR     PSP, r0
                MSR     BASEPRI, r0
                MSR     FAULTMASK, r0
                LDR     lr, =SecureIspRam_Stop
                B       SecureIspRam_Run
                ENDP
                ALIGN
                END
