
#include <pspsdk.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <pspsyscon.h>
#include <pspdisplay.h>
#include <pspfpu.h>
#include <stdio.h>
#include <string.h> // memcpy

// bad practice, i'll fix it later
#include "pspfpu.c"

PSP_MODULE_INFO("NikoTest", 0x1000, 0, 1);
PSP_MAIN_THREAD_ATTR(0);

SceCtrlData pad; // Structure which represents the PSP controls, we'll use it to interact with the PSP

// PSP
// base: 0x8800000
// size: 0x1800000
// tail: 0xA000000

// PS1
// base: 0x9800000
// size: 0x0200000
// tail: 0x9A00000

const int pspBaseAddr = 0x8800000;
const int ps1BaseAddr = 0x9800000;

float sin(float x)
{
	return pspFpuSin(x);
}

float cos(float x)
{
	return pspFpuCos(x);
}

void WriteMem(unsigned int psxAddr, void* pspAddr, int size)
{
	// remove 0x80 prefix
	psxAddr = psxAddr & 0xFFFFFF;
	
	// add ps1BaseAddr
	psxAddr = psxAddr + ps1BaseAddr;
	
	// copy
	memcpy((void*)psxAddr,(void*)pspAddr,size);
}

void ReadMem(unsigned int psxAddr, void* pspAddr, int size)
{
	// remove 0x80 prefix
	psxAddr = psxAddr & 0xFFFFFF;
	
	// add ps1BaseAddr
	psxAddr = psxAddr + ps1BaseAddr;
	
	// copy
	memcpy((void*)pspAddr,(void*)psxAddr,size);
}
	
#define UP 0x1
#define DOWN 0x2
#define LEFT 0x4
#define RIGHT 0x8
#define CROSS 0x10
#define SQUARE 0x20
#define CIRCLE 0x40
//#define L2 0x80
//#define L2 0x100 // which is it?
#define R2 0x200
#define R1 0x400
#define L1 0x800
#define START 0x1000
#define SELECT 0x2000
//#define CROSS 0x4000 -- are my notes right?
//#define SQUARE 0x8000
#define L3 0x10000
#define R3 0x20000
#define TRIANGLE 0x40000
	
int main_thread(SceSize args, void* argp)
{	
	pspFpuSetEnable(0);

	while(1)
	{
		int buttons;
		ReadMem(0x80096804 + 0x10, &buttons, sizeof(buttons));
		
		if (buttons & SELECT) break;
		
		sceDisplayWaitVblankStart();
	}

	while(1)
	{
		sceCtrlPeekBufferPositive(&pad, 1);
		
		// close when you press the home button
		if(pad.Buttons & PSP_CTRL_HOME) break;

		// Allow us to move the camera
		int status = 0;
		WriteMem(0x80098000, &status, sizeof(status));

		// posX, posY, posZ, rotX, rotY, rotZ
		short variables[6];

		// If you want to mess with other cameras, each additional
		// camera is 0x110 bytes added to the address

		// grab the position and rotation
		ReadMem(0x80096B20 + 0x168, variables, sizeof(variables));

		int speed = 0x18;

		// Get Controller Data
		// If you want input from other controllers, each additional
		// controlller is 0x50 bytes added to the address. Also, 
		// you can use + 0x14 instead of 0x10 for 'tap' instead of 'hold'
		int buttons;
		ReadMem(0x80096804 + 0x10, &buttons, sizeof(buttons));
		
		if (buttons & SELECT) speed *= 3;

		if (buttons & TRIANGLE) variables[3] += speed;
		if (buttons & CROSS) variables[3] -= speed;
		if (buttons & SQUARE) variables[4] += speed;
		if (buttons & CIRCLE) variables[4] -= speed;

		float angle = 2 * 3.14159f * (float)variables[4] / (float)0x1000;

		if (buttons & LEFT)
		{
			variables[0] -= speed * cos(angle);
			variables[2] += speed * sin(angle);
		}

		else if (buttons & RIGHT)
		{
			variables[0] += speed * cos(angle);
			variables[2] -= speed * sin(angle);
		}

		if (buttons & UP)
		{
			variables[0] -= speed * sin(angle);
			variables[2] -= speed * cos(angle);
		}
		
		else if (buttons & DOWN)
		{
			variables[0] += speed * sin(angle);
			variables[2] += speed * cos(angle);
		}

		if (buttons & L1) variables[1] -= speed;
		if (buttons & R1) variables[1] += speed;

		// inject the new position and rotation
		WriteMem(0x80096B20 + 0x168, variables, sizeof(variables));

		// Get address of P1's 0x670 buffer
		int AddrP1;
		ReadMem(0x8009900C, &AddrP1, sizeof(AddrP1));

		// Compress position to shorts
		int pos[3];
		pos[0] = (int)variables[0] << 8;
		pos[1] = (int)variables[1] << 8;
		pos[2] = (int)variables[2] << 8;

		// Change player position to camera position
		int zero = 0;
		WriteMem(AddrP1 + 0x2d4, pos, sizeof(pos));

		// +7c
		// Disable the teleportation of the player to 2d4, 2d8, 2dc,
		// so player wont use it's own position variables
		WriteMem(AddrP1 + 0x7c, &zero, sizeof(zero));
	
		sceDisplayWaitVblankStart();
	}
	
	sceKernelExitDeleteThread(0);
    return 0;
}

// start a thread for the program
extern "C"{
int module_start(SceSize args, void *argp)
{
	int thid1 = sceKernelCreateThread("NikoTest", main_thread, 10, 0x2000, 0, NULL);
	if(thid1 >= 0)
		sceKernelStartThread(thid1, args, argp);
	return 0;
}
}

// terminate the plugin
extern "C"{
int module_stop (void) {
	return 0;
}
}
