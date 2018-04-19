/*
 * If a serious error occurs, one of the fault
 * exception vectors in this file will be called.
 *
 * This file attempts to aid the unfortunate debugger
 * to blame someone for the crashing code
 *
 *  Created on: 12.06.2013
 *      Author: uli
 *
 * Released under the CC0 1.0 Universal (public domain)
 */
#include <stdint.h>
#include <ch.h>
#include <string.h>

#ifdef DEBUG

/**
 * Executes the BKPT instruction that causes the debugger to stop.
 * If no debugger is attached, this will be ignored
 */
#define bkpt() __asm volatile("BKPT #0\n")

//See http://infocenter.arm.com/help/topic/com.arm.doc.dui0552a/BABBGBEC.html
typedef enum  {
    Reset = 1,
    NMI = 2,
    HardFault = 3,
    MemManage = 4,
    BusFault = 5,
    UsageFault = 6,
} FaultType;

/* On hard fault, copy FAULT_PSP to the sp reg so gdb can give a trace */
void **FAULT_PSP;
register void *stack_pointer asm("sp");

void HardFault_Handler(void) {
    asm("mrs %0, psp" : "=r"(FAULT_PSP) : :);
    stack_pointer = FAULT_PSP;
    // Here will be good to assert Error LED, like
    // GPIOA->ODR |= 1;
    bkpt();
    while(1);
}

void BusFault_Handler(void) __attribute__((alias("HardFault_Handler")));

void UsageFault_Handler(void) {
    asm("mrs %0, psp" : "=r"(FAULT_PSP) : :);
    stack_pointer = FAULT_PSP;
    bkpt();
    while(1);
}

void MemManage_Handler(void) {
    asm("mrs %0, psp" : "=r"(FAULT_PSP) : :);
    stack_pointer = FAULT_PSP;
    bkpt();
    while(1);
}

#endif
