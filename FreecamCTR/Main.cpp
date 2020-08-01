
#include <windows.h>
#include <time.h>
#include <stdio.h>

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

	// Initialize random number generator
	srand((unsigned int)time(NULL));

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

void MoveCamera()
{
	// Player 1
	int playerIndex = 0;
	
	// posX, posY, posZ, rotX, rotY, rotZ
	short variables[6];
	
	// grab the position and rotation
	ReadMem(0x80096B20 + 0x168 + 0x110 * playerIndex, variables, sizeof(variables));
	
	int speed = 0x18;
	
	if (GetAsyncKeyState(VK_UP))	variables[3] += speed;
	if (GetAsyncKeyState(VK_DOWN))	variables[3] -= speed;
	if (GetAsyncKeyState(VK_LEFT))	variables[4] += speed;
	if (GetAsyncKeyState(VK_RIGHT)) variables[4] -= speed;
	
	float angle = 2 * 3.14159f * (float)variables[4] / (float)0x1000;
	
	if (GetAsyncKeyState('A'))
	{
		variables[2] += speed * sin(angle);
		variables[0] -= speed * cos(angle);
	}
	
	if (GetAsyncKeyState('D'))
	{
		variables[2] -= speed * sin(angle);
		variables[0] += speed * cos(angle);
	}
	
	if (GetAsyncKeyState('W'))
	{
		variables[0] -= speed * sin(angle);
		variables[2] -= speed * cos(angle);
	}
	if (GetAsyncKeyState('S'))
	{
		variables[0] += speed * sin(angle);
		variables[2] += speed * cos(angle);
	}
	
	if (GetAsyncKeyState('F')) variables[1] -= speed;
	if (GetAsyncKeyState('R')) variables[1] += speed;
	
	// inject the new position and rotation
	WriteMem(0x80096B20 + 0x168 + 0x110 * playerIndex, variables, sizeof(variables));

	int AddrP1;
	ReadMem(0x8009900C, &AddrP1, sizeof(AddrP1));

	int pos[3];
	pos[0] = (int)variables[0] << 8;
	pos[1] = (int)variables[1] << 8;
	pos[2] = (int)variables[2] << 8;

	int zero = 0;
	WriteMem(AddrP1 + 0x2d4, pos, sizeof(pos));

	// Disable the teleportation of the player to 2d4, 2d8, 2dc
	WriteMem(AddrP1 + 0x7c, &zero, sizeof(zero));
}

int main(int argc, char** argv)
{
	initialize();

	system("cls");

	// Main loop...
	while (true)
	{
		int status = 0;
		WriteMem(0x80098000, &status, sizeof(status));

		MoveCamera();

		// 16ms = 60fps
		Sleep(16);
	}

	return 0;
}