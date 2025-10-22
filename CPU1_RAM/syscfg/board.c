/*
 * Copyright (c) 2020 Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "board.h"

//*****************************************************************************
//
// Board Configurations
// Initializes the rest of the modules. 
// Call this function in your application if you wish to do all module 
// initialization.
// If you wish to not use some of the initializations, instead of the 
// Board_init use the individual Module_inits
//
//*****************************************************************************
void Board_init()
{
	EALLOW;

	PinMux_init();
	CPUTIMER_init();
	GPIO_init();
	INTERRUPT_init();

	EDIS;
}

//*****************************************************************************
//
// PINMUX Configurations
//
//*****************************************************************************
void PinMux_init()
{
	//
	// PinMux for modules assigned to CPU1
	//
	
	// GPIO10 -> myLED1_GPIO Pinmux
	GPIO_setPinConfig(GPIO_10_GPIO10);
	// GPIO6 -> myLED2_GPIO Pinmux
	GPIO_setPinConfig(GPIO_6_GPIO6);

}


//*****************************************************************************
//
// CPUTIMER Configurations
//
//*****************************************************************************
void CPUTIMER_init(){
	timer1_init();
}

void timer1_init(){
	CPUTimer_setEmulationMode(timer1_BASE, CPUTIMER_EMULATIONMODE_STOPAFTERNEXTDECREMENT);
	CPUTimer_setPreScaler(timer1_BASE, 0U);
	CPUTimer_setPeriod(timer1_BASE, 10000000U);
	CPUTimer_enableInterrupt(timer1_BASE);
	CPUTimer_stopTimer(timer1_BASE);

	CPUTimer_reloadTimerCounter(timer1_BASE);
	CPUTimer_startTimer(timer1_BASE);
}

//*****************************************************************************
//
// GPIO Configurations
//
//*****************************************************************************
void GPIO_init(){
	myLED1_GPIO_init();
	myLED2_GPIO_init();
}

void myLED1_GPIO_init(){
	GPIO_writePin(myLED1_GPIO, 1);
	GPIO_setPadConfig(myLED1_GPIO, GPIO_PIN_TYPE_STD);
	GPIO_setQualificationMode(myLED1_GPIO, GPIO_QUAL_SYNC);
	GPIO_setDirectionMode(myLED1_GPIO, GPIO_DIR_MODE_OUT);
	GPIO_setControllerCore(myLED1_GPIO, GPIO_CORE_CPU1);
}
void myLED2_GPIO_init(){
	GPIO_writePin(myLED2_GPIO, 1);
	GPIO_setPadConfig(myLED2_GPIO, GPIO_PIN_TYPE_STD);
	GPIO_setQualificationMode(myLED2_GPIO, GPIO_QUAL_SYNC);
	GPIO_setDirectionMode(myLED2_GPIO, GPIO_DIR_MODE_OUT);
	GPIO_setControllerCore(myLED2_GPIO, GPIO_CORE_CPU1);
}

//*****************************************************************************
//
// INTERRUPT Configurations
//
//*****************************************************************************
void INTERRUPT_init(){
	
	// Interrupt Settings for INT_timer2
	// ISR need to be defined for the registered interrupts
	Interrupt_register(INT_timer2, &portTICK_ISR);
	Interrupt_disable(INT_timer2);
	
	// Interrupt Settings for INT_timer1
	// ISR need to be defined for the registered interrupts
	Interrupt_register(INT_timer1, &timer1_ISR);
	Interrupt_enable(INT_timer1);
}
