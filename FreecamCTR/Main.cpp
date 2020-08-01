
#include <windows.h>
#include "Scanner.h"

// Can be negative
long long int baseAddress;
HANDLE handle;

void WriteMem(unsigned int psxAddr, void* pcAddr, int size)
{
	WriteProcessMemory(handle, (PBYTE*)(baseAddress + psxAddr), pcAddr, size, 0);
}

void ReadMem(unsigned int psxAddr, void* pcAddr, int size)
{
	ReadProcessMemory(handle, (PBYTE*)(baseAddress + psxAddr), pcAddr, size, 0);
}

void initialize()
{
	int choice = 0;
	HWND console = GetConsoleWindow();
	RECT r;
	GetWindowRect(console, &r); //stores the console's current dimensions

	// 300 + height of bar (35)
	MoveWindow(console, r.left, r.top, 400, 300, TRUE);

	printf("\n");
	printf("Step 1: Open any ps1 emulator\n");
	printf("Step 2: Open CTR SCUS_94426\n");
	printf("\n");
	printf("Step 3: Enter emulator PID from 'Details'\n");
	printf("           tab of Windows Task Manager\n");
	printf("Enter: ");

	DWORD procID = 0;
	scanf("%d", &procID);

	printf("\n");
	printf("Searching for CTR 94426 in emulator ram...\n");

	// open the process with procID, and store it in the 'handle'
	handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procID);

	// if handle fails to open
	if (!handle)
	{
		printf("Failed to open process\n");
		system("pause");
		exit(0);
	}

	// This idea to scan memory for 11 bytes to automatically
	// find that CTR is running, and to find the base address
	// of any emulator universally, was EuroAli's idea in the
	// CTR-Tools discord server. Thank you EuroAli

	// Shows at PSX address 0x8003C62C, only in CTR 94426
	unsigned char ctrData[12] = { 0x71, 0xDC, 0x01, 0x0C, 0x00, 0x00, 0x00, 0x00, 0xD0, 0xF9, 0x00, 0x0C };

	// can't be nullptr by default or it crashes,
	// it will become 1 when the loop starts
	baseAddress = 0;

	// Modified from https://guidedhacking.com/threads/hyperscan-fast-vast-memory-scanner.9659/
	std::vector<UINT_PTR> AddressHolder = Hyperscan::HYPERSCAN_SCANNER::Scan(procID, ctrData, 12, Hyperscan::HyperscanAllignment4Bytes,
		Hyperscan::HyperscanTypeExact);

	// take the first (should be only) result
	baseAddress = AddressHolder[0];

	// Remove 0x8003C62C address of PSX memory,
	// to find the relative address where PSX memory
	// is located in RAM. It is ok for baseAddress
	// to be a negative number
	baseAddress -= 0x8003C62C;
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

int main(int argc, char** argv)
{
	initialize();

	system("cls");

	// Main loop...
	while (true)
	{
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

		// 16ms = 60fps
		Sleep(16);
	}

	return 0;
}