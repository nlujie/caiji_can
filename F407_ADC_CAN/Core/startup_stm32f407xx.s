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
        REPT    82
        DCD     Default_Handler
        ENDR
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
