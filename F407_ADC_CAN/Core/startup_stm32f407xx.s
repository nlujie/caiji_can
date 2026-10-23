        PRESERVE8
        THUMB

        AREA    STACK, NOINIT, READWRITE, ALIGN=3
Stack_Mem
        SPACE   0x800
__initial_sp

        AREA    RESET, DATA, READONLY
        EXPORT  __Vectors
        EXPORT  __Vectors_End
        EXPORT  __Vectors_Size
__Vectors
        DCD     __initial_sp
        DCD     Reset_Handler
        DCD     Default_Handler          ; NMI
        DCD     Default_Handler          ; HardFault
        DCD     Default_Handler          ; MemManage
        DCD     Default_Handler          ; BusFault
        DCD     Default_Handler          ; UsageFault
        DCD     0
        DCD     0
        DCD     0
        DCD     0
        DCD     Default_Handler          ; SVCall
        DCD     Default_Handler          ; DebugMonitor
        DCD     0
        DCD     Default_Handler          ; PendSV
        DCD     SysTick_Handler
        DCD     Default_Handler          ; IRQ0 WWDG
        DCD     Default_Handler          ; IRQ1 PVD
        DCD     Default_Handler          ; IRQ2 TAMP_STAMP
        DCD     Default_Handler          ; IRQ3 RTC_WKUP
        DCD     Default_Handler          ; IRQ4 FLASH
        DCD     Default_Handler          ; IRQ5 RCC
        DCD     Default_Handler          ; IRQ6 EXTI0
        DCD     Default_Handler          ; IRQ7 EXTI1
        DCD     Default_Handler          ; IRQ8 EXTI2
        DCD     Default_Handler          ; IRQ9 EXTI3
        DCD     Default_Handler          ; IRQ10 EXTI4
        DCD     Default_Handler          ; IRQ11 DMA1_Stream0
        DCD     Default_Handler          ; IRQ12 DMA1_Stream1
        DCD     Default_Handler          ; IRQ13 DMA1_Stream2
        DCD     Default_Handler          ; IRQ14 DMA1_Stream3
        DCD     Default_Handler          ; IRQ15 DMA1_Stream4
        DCD     Default_Handler          ; IRQ16 DMA1_Stream5
        DCD     Default_Handler          ; IRQ17 DMA1_Stream6
        DCD     Default_Handler          ; IRQ18 ADC
        DCD     Default_Handler          ; IRQ19 CAN1_TX
        DCD     Default_Handler          ; IRQ20 CAN1_RX0
        DCD     Default_Handler          ; IRQ21 CAN1_RX1
        DCD     Default_Handler          ; IRQ22 CAN1_SCE
        DCD     Default_Handler          ; IRQ23 EXTI9_5
        DCD     Default_Handler          ; IRQ24 TIM1_BRK_TIM9
        DCD     Default_Handler          ; IRQ25 TIM1_UP_TIM10
        DCD     Default_Handler          ; IRQ26 TIM1_TRG_COM_TIM11
        DCD     Default_Handler          ; IRQ27 TIM1_CC
        DCD     Default_Handler          ; IRQ28 TIM2
        DCD     Default_Handler          ; IRQ29 TIM3
        DCD     Default_Handler          ; IRQ30 TIM4
        DCD     Default_Handler          ; IRQ31 I2C1_EV
        DCD     Default_Handler          ; IRQ32 I2C1_ER
        DCD     Default_Handler          ; IRQ33 I2C2_EV
        DCD     Default_Handler          ; IRQ34 I2C2_ER
        DCD     Default_Handler          ; IRQ35 SPI1
        DCD     Default_Handler          ; IRQ36 SPI2
        DCD     Default_Handler          ; IRQ37 USART1
        DCD     Default_Handler          ; IRQ38 USART2
        DCD     Default_Handler          ; IRQ39 USART3
        DCD     Default_Handler          ; IRQ40 EXTI15_10
        DCD     Default_Handler          ; IRQ41 RTC_Alarm
        DCD     Default_Handler          ; IRQ42 OTG_FS_WKUP
        DCD     Default_Handler          ; IRQ43 TIM8_BRK_TIM12
        DCD     Default_Handler          ; IRQ44 TIM8_UP_TIM13
        DCD     Default_Handler          ; IRQ45 TIM8_TRG_COM_TIM14
        DCD     Default_Handler          ; IRQ46 TIM8_CC
        DCD     Default_Handler          ; IRQ47 DMA1_Stream7
        DCD     Default_Handler          ; IRQ48 FSMC
        DCD     Default_Handler          ; IRQ49 SDIO
        DCD     Default_Handler          ; IRQ50 TIM5
        DCD     Default_Handler          ; IRQ51 SPI3
        DCD     Default_Handler          ; IRQ52 UART4
        DCD     Default_Handler          ; IRQ53 UART5
        DCD     Default_Handler          ; IRQ54 TIM6_DAC
        DCD     Default_Handler          ; IRQ55 TIM7
        DCD     Default_Handler          ; IRQ56 DMA2_Stream0
        DCD     Default_Handler          ; IRQ57 DMA2_Stream1
        DCD     Default_Handler          ; IRQ58 DMA2_Stream2
        DCD     Default_Handler          ; IRQ59 DMA2_Stream3
        DCD     Default_Handler          ; IRQ60 DMA2_Stream4
        DCD     Default_Handler          ; IRQ61 ETH
        DCD     Default_Handler          ; IRQ62 ETH_WKUP
        DCD     Default_Handler          ; IRQ63 CAN2_TX
        DCD     Default_Handler          ; IRQ64 CAN2_RX0
        DCD     Default_Handler          ; IRQ65 CAN2_RX1
        DCD     Default_Handler          ; IRQ66 CAN2_SCE
        DCD     Default_Handler          ; IRQ67 OTG_FS
        DCD     Default_Handler          ; IRQ68 DMA2_Stream5
        DCD     Default_Handler          ; IRQ69 DMA2_Stream6
        DCD     Default_Handler          ; IRQ70 DMA2_Stream7
        DCD     Default_Handler          ; IRQ71 USART6
        DCD     Default_Handler          ; IRQ72 I2C3_EV
        DCD     Default_Handler          ; IRQ73 I2C3_ER
        DCD     Default_Handler          ; IRQ74 OTG_HS_EP1_OUT
        DCD     Default_Handler          ; IRQ75 OTG_HS_EP1_IN
        DCD     Default_Handler          ; IRQ76 OTG_HS_WKUP
        DCD     Default_Handler          ; IRQ77 OTG_HS
        DCD     Default_Handler          ; IRQ78 DCMI
        DCD     Default_Handler          ; IRQ79 CRYP
        DCD     Default_Handler          ; IRQ80 HASH_RNG
        DCD     Default_Handler          ; IRQ81 FPU
__Vectors_End
__Vectors_Size EQU __Vectors_End - __Vectors

        AREA    |.text|, CODE, READONLY
        EXPORT  Reset_Handler
        IMPORT  SystemInit
        IMPORT  __main
        IMPORT  SysTick_Handler

Reset_Handler PROC
        BL      SystemInit
        B       __main
        ENDP

Default_Handler PROC
        B       .
        ENDP

        END
