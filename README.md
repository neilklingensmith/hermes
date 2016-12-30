# README #

This README would normally document whatever steps are necessary to get your application up and running.

## Introduction ##



## Getting Started ##

### Prereqs ###

**You will need:**

* ** A windows machine/VM **
* ** An Atmel SAM E70 Xplained Demo Board **

## Procedure ##

1. Download the FreeRTOS Source code from [here](http://www.freertos.org/a00104.html).
2. Download Atmel Studio 7.0 from [here](http://atmel.com).
3. Navigate to FreeRTOSv9.0.0\FreeRTOS\Demo\CORTEX_M7_SAME70_Xplained_AtmelStudio\ and clone the hermes git repo into that directory.
4. Open the blinky demo project in Atmel Studio, located in the same directory.
5. Within the project, open the startup file: \src\ASF\sam\utils\cmsis\same70\source\templates\gcc\startup_same70.c. It might be easier to just search for startup_same70.c in the Solution Explorer pane on the right side of the screen.
6. At the bottom of startup_same70.c, there is a function called Reset_Handler which needs to be modified for Hermes. Modifications are shown below, commented with // ADD FOR HERMES:

```
#!C

void Reset_Handler(void)
{
    extern void *hvVectorTable[]; // ADD FOR HERMES
    uint32_t *pSrc, *pDest;

    /* Initialize the relocate segment */
    pSrc = &_etext;
    pDest = &_srelocate;

    if (pSrc != pDest) {
        for (; pDest < &_erelocate;) {
            *pDest++ = *pSrc++;
        }
    }

    /* Clear the zero segment */
    for (pDest = &_szero; pDest < &_ezero;) {
        *pDest++ = 0;
    }

    /* Set the vector table base address */
    pSrc = (uint32_t *) & _sfixed;
    //SCB->VTOR = ((uint32_t) pSrc & SCB_VTOR_TBLOFF_Msk); // REMOVE FOR HERMES
    SCB->VTOR = ((uint32_t) hvVectorTable); // ADD FOR HERMES

    hvInit(&exception_table); // ADD FOR HERMES

#if __FPU_USED
    fpu_enable();
#endif

    /* Initialize the C library */
    __libc_init_array();

    /* Branch to main function */
    main();

    /* Infinite loop */
    while (1);
}
```

7. Build the project.
8. To program the board, click the Start Debugging and Break button. This will erase and program the demo board with the project binary. It will hang when waiting for a reset. When Atmel Studio asks if you want to continue waiting for reset, click No.
9. Click the button labelled Attach to Target (next to Start Debugging and Break). This will start a debugging session normally and allow you to step through the code.


## Contributing ##

Contact [Neil Klingensmith](http://neilklingensmith.com)