#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctr/types.h>
#include <ctr/srv.h>
#include <ctr/svc.h>
#include <ctr/GSP.h>
#include <ctr/GX.h>
#include <ctr/FS.h>
#include "text.h"

#include "../../build/constants.h"

char console_buffer[4096];

Result _HBSPECIAL_GetHandle(Handle handle, u32 index, Handle* out)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x00040040; //request header code
	cmdbuf[1]=index;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(handle)))return ret;

	if(out && !cmdbuf[1])*out=cmdbuf[5];

	return cmdbuf[1];
}


u8* gspHeap;
u32* gxCmdBuf;

u8* _heap_base; // should be 0x08000000
const u32 _heap_size = 0x01000000;

u8 currentBuffer;
u8* topLeftFramebuffers[2];

Handle gspEvent, gspSharedMemHandle;

void gspGpuInit()
{
	gspInit();

	GSPGPU_AcquireRight(NULL, 0x0);
	GSPGPU_SetLcdForceBlack(NULL, 0x0);

	//set subscreen to blue
	u32 regData=0x01FF0000;
	GSPGPU_WriteHWRegs(NULL, 0x202A04, &regData, 4);

	//setup our gsp shared mem section
	u8 threadID;
	svc_createEvent(&gspEvent, 0x0);
	GSPGPU_RegisterInterruptRelayQueue(NULL, gspEvent, 0x1, &gspSharedMemHandle, &threadID);
	svc_mapMemoryBlock(gspSharedMemHandle, 0x10002000, 0x3, 0x10000000);

	//map GSP heap
	svc_controlMemory((u32*)&gspHeap, 0x0, 0x0, 0x01000000, 0x10003, 0x3);

	//wait until we can write stuff to it
	svc_waitSynchronization1(gspEvent, 0x55bcb0);

	//GSP shared mem : 0x2779F000
	gxCmdBuf=(u32*)(0x10002000+0x800+threadID*0x200);

	//grab main left screen framebuffer addresses
	topLeftFramebuffers[0] = &gspHeap[0] - 0x10000000;
	topLeftFramebuffers[1] = &gspHeap[0x46500] - 0x10000000;
	GSPGPU_WriteHWRegs(NULL, 0x400468, (u32*)&topLeftFramebuffers, 8);
	topLeftFramebuffers[0] += 0x10000000;
	topLeftFramebuffers[1] += 0x10000000;

	currentBuffer=0;
}

void gspGpuExit()
{
	GSPGPU_UnregisterInterruptRelayQueue(NULL);

	GSPGPU_ReleaseRight(NULL);

	//unmap GSP shared mem
	svc_unmapMemoryBlock(gspSharedMemHandle, 0x10002000);
	svc_closeHandle(gspSharedMemHandle);
	svc_closeHandle(gspEvent);
	
	gspExit();

	//free GSP heap
	svc_controlMemory((u32*)&gspHeap, (u32)gspHeap, 0x0, 0x01000000, MEMOP_FREE, 0x0);
}

void swapBuffers()
{
	u32 regData;
	GSPGPU_ReadHWRegs(NULL, 0x400478, (u32*)&regData, 4);
	regData^=1;
	currentBuffer=regData&1;
	GSPGPU_WriteHWRegs(NULL, 0x400478, (u32*)&regData, 4);
}

// const u8 hexTable[]=
// {
// 	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
// };

// void hex2str(char* out, u32 val)
// {
// 	int i;
// 	for(i=0;i<8;i++){out[7-i]=hexTable[val&0xf];val>>=4;}
// 	out[8]=0x00;
// }

// void renderString(char* str, int x, int y)
// {
// 	drawString(topLeftFramebuffers[0],str,x,y);
// 	drawString(topLeftFramebuffers[1],str,x,y);
// 	GSPGPU_FlushDataCache(NULL, topLeftFramebuffers[0], 240*400*3);
// 	GSPGPU_FlushDataCache(NULL, topLeftFramebuffers[1], 240*400*3);
// }

// void centerString(char* str, int y)
// {
// 	int x=200-(strlen(str)*4);
// 	drawString(topLeftFramebuffers[0],str,x,y);
// 	drawString(topLeftFramebuffers[1],str,x,y);
// 	GSPGPU_FlushDataCache(NULL, topLeftFramebuffers[0], 240*400*3);
// 	GSPGPU_FlushDataCache(NULL, topLeftFramebuffers[1], 240*400*3);
// }

// void drawHex(u32 val, int x, int y)
// {
// 	char str[9];

// 	hex2str(str,val);
// 	renderString(str,x,y);
// }

// void clearScreen(u8 shade)
// {
// 	memset(topLeftFramebuffers[0], shade, 240*400*3);
// 	memset(topLeftFramebuffers[1], shade, 240*400*3);
// 	GSPGPU_FlushDataCache(NULL, topLeftFramebuffers[0], 240*400*3);
// 	GSPGPU_FlushDataCache(NULL, topLeftFramebuffers[1], 240*400*3);
// }

// void drawTitleScreen(char* str)
// {
// 	clearScreen(0x00);
// 	centerString("snshax",0);
// 	centerString(BUILDTIME,10);
// 	renderString(str, 0, 40);
// }

// void resetConsole(void)
// {
// 	console_buffer[0] = 0x00;
// 	drawTitleScreen(console_buffer);
// }

// void print_str(char* str)
// {
// 	strcpy(&console_buffer[strlen(console_buffer)], str);
// 	drawTitleScreen(console_buffer);
// }

// void print_hex(u32 val)
// {
// 	char str[9];

// 	hex2str(str,val);
// 	print_str(str);
// }

void doGspwn(u32* src, u32* dst, u32 size)
{
	GX_SetTextureCopy(gxCmdBuf, src, 0xFFFFFFFF, dst, 0xFFFFFFFF, size, 0x00000008);
}

typedef struct {
	u32 num;

	struct {
		char name[8];
		Handle handle;
	} services[3];
} nonflexible_service_list_t;

extern Handle aptLockHandle, aptuHandle;

const char * const __apt_servicenames[3] = {"APT:U", "APT:S", "APT:A"};
char *__apt_servicestr = NULL;

static Result __apt_initservicehandle()
{
	Result ret=0;
	u32 i;

	if(__apt_servicestr)
	{
		return srv_getServiceHandle(NULL, &aptuHandle, __apt_servicestr);
	}

	for(i=0; i<3; i++)
	{
		ret = srv_getServiceHandle(NULL, &aptuHandle, (char*)__apt_servicenames[i]);
		if(ret==0)
		{
			__apt_servicestr = (char*)__apt_servicenames[i];
			return ret;
		}
	}

	return ret;
}

void _aptOpenSession()
{
	svc_waitSynchronization1(aptLockHandle, U64_MAX);
	srv_getServiceHandle(NULL, &aptuHandle, __apt_servicestr);
}

void _aptCloseSession()
{
	svc_closeHandle(aptuHandle);
	svc_releaseMutex(aptLockHandle);
}

Result _APT_ReceiveParameter(Handle* handle, u32 appID, u32 bufferSize, u32* buffer, u32* actualSize, u8* signalType, Handle* outhandle)
{
	if(!handle)handle=&aptuHandle;
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0xD0080; //request header code
	cmdbuf[1]=appID;
	cmdbuf[2]=bufferSize;
	
	cmdbuf[0+0x100/4]=(bufferSize<<14)|2;
	cmdbuf[1+0x100/4]=(u32)buffer;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(*handle)))return ret;

	if(signalType)*signalType=cmdbuf[3];
	if(actualSize)*actualSize=cmdbuf[4];
	if(outhandle)*outhandle=cmdbuf[6];

	return cmdbuf[1];
}

#define APP_START_LINEAR 0xBABE0002

extern u32* _bootloaderAddress;

void _main()
{
	Result ret;
	Handle hbSpecialHandle, fsuHandle, nssHandle, irrstHandle;

	initSrv();
	srv_RegisterClient(NULL);

	gspGpuInit();

	// resetConsole();
	// print_str("hello\n");

	__apt_initservicehandle();
	ret=APT_GetLockHandle(&aptuHandle, 0x0, &aptLockHandle);
	svc_closeHandle(aptuHandle);

	// print_str("\ngot APT:A lock handle ?\n");
	// print_hex(ret); print_str(", "); print_hex(aptuHandle); print_str(", "); print_hex(aptLockHandle);

	u32 outbuf[2];
	_aptOpenSession();
	ret = _APT_ReceiveParameter(NULL, 0x101, 0x8, outbuf, NULL, NULL, &fsuHandle);
	_aptCloseSession();

	// print_str("\ngot apt parameter ?\n");
	// print_hex(ret); print_str(", "); print_hex(fsuHandle); print_str(", "); print_hex(outbuf[0]); print_str(", "); print_hex(outbuf[1]); print_str(", "); print_hex(outbuf[2]);

	ret = 1;
	int cnt = 0;
	while(ret)
	{
		_aptOpenSession();
		ret = _APT_ReceiveParameter(NULL, 0x101, 0x8, outbuf, NULL, NULL, &nssHandle);
		_aptCloseSession();
		cnt++;
	}

	// print_str("\ngot apt parameter ?\n");
	// print_hex(cnt); print_str(", "); print_hex(nssHandle); print_str(", "); print_hex(outbuf[0]); print_str(", "); print_hex(outbuf[1]); print_str(", "); print_hex(outbuf[2]);

	ret = 1;
	cnt = 0;
	while(ret)
	{
		_aptOpenSession();
		ret = _APT_ReceiveParameter(NULL, 0x101, 0x8, outbuf, NULL, NULL, &irrstHandle);
		_aptCloseSession();
		cnt++;
	}

	// print_str("\ngot apt parameter ?\n");
	// print_hex(cnt); print_str(", "); print_hex(nssHandle); print_str(", "); print_hex(outbuf[0]); print_str(", "); print_hex(outbuf[1]); print_str(", "); print_hex(outbuf[2]);

	// print_str("\nconnecting to hb:SPECIAL...\n");
	// ret = svc_connectToPort(&hbSpecialHandle, "hb:SPECIAL");
	// print_hex(ret); print_str(", "); print_hex(hbSpecialHandle);

	// print_str("\ngrabbing fs:USER handle from menu through hb:SPECIAL\n");
	// ret = _HBSPECIAL_GetHandle(hbSpecialHandle, 2, &fsuHandle);
	// print_hex(ret); print_str(", "); print_hex(fsuHandle);

	// print_str("\ngrabbing ns:s handle from menu through hb:SPECIAL\n");
	// ret = _HBSPECIAL_GetHandle(hbSpecialHandle, 3, &nssHandle);
	// print_hex(ret); print_str(", "); print_hex(nssHandle);

	// copy bootloader code
	GSPGPU_FlushDataCache(NULL, (u8*)&gspHeap[0x00100000], 0x00005000);
	doGspwn(_bootloaderAddress, (u32*)&gspHeap[0x00100000], 0x00005000);

	// sleep for 200ms
	svc_sleepThread(200*1000*1000);

	// setup service list structure
	*(nonflexible_service_list_t*)(&gspHeap[0x00100000] + 0x4 * 8) = (nonflexible_service_list_t){3, {{"ns:s", nssHandle}, {"fs:USER", fsuHandle}, {"ir:rst", irrstHandle}}};

	// flush and copy
	GSPGPU_FlushDataCache(NULL, (u8*)&gspHeap[0x00100000], 0x00005000);
	doGspwn((u32*)&gspHeap[0x00100000], (u32*)APP_START_LINEAR, 0x00005000);

	// sleep for 200ms
	svc_sleepThread(200*1000*1000);

	gspGpuExit();
	exitSrv();

	// free heap (has to be the very last thing before jumping to app as contains bss)
	u32 out; svc_controlMemory(&out, (u32)_heap_base, 0x0, _heap_size, MEMOP_FREE, 0x0);

	void (*app_runmenu)() = (void*)(0x00100000 + 4);
	app_runmenu();
}
