#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctr/types.h>
#include <ctr/srv.h>
#include <ctr/svc.h>
#include <ctr/APT.h>
#include <ctr/FS.h>
#include "text.h"
#include "spider_hook_rop_bin.h"
#include "spider_initial_rop_bin.h"
#include "spider_thread0_rop_bin.h"
#include "cn_bootloader_bin.h"

#include "../../build/constants.h"

#define TOPFBADR1 ((u8*)CN_TOPFBADR1)
#define TOPFBADR2 ((u8*)CN_TOPFBADR2)

typedef enum
{
	PAD_A = (1<<0),
	PAD_B = (1<<1),
	PAD_SELECT = (1<<2),
	PAD_START = (1<<3),
	PAD_RIGHT = (1<<4),
	PAD_LEFT = (1<<5),
	PAD_UP = (1<<6),
	PAD_DOWN = (1<<7),
	PAD_R = (1<<8),
	PAD_L = (1<<9),
	PAD_X = (1<<10),
	PAD_Y = (1<<11)
}PAD_KEY;

int _strlen(char* str)
{
	int l=0;
	while(*(str++))l++;
	return l;
}

void _strcpy(char* dst, char* src)
{
	while(*src)*(dst++)=*(src++);
	*dst=0x00;
}

Result _srv_RegisterClient(Handle* handleptr)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x10002; //request header code
	cmdbuf[1]=0x20;

	Result ret=0;
	if((ret=svc_sendSyncRequest(*handleptr)))return ret;

	return cmdbuf[1];
}

Result _initSrv(Handle* srvHandle)
{
	Result ret=0;
	if(svc_connectToPort(srvHandle, "srv:"))return ret;
	return _srv_RegisterClient(srvHandle);
}

Result _srv_getServiceHandle(Handle* handleptr, Handle* out, char* server)
{
	u8 l=_strlen(server);
	if(!out || !server || l>8)return -1;

	u32* cmdbuf=getThreadCommandBuffer();

	cmdbuf[0]=0x50100; //request header code
	_strcpy((char*)&cmdbuf[1], server);
	cmdbuf[3]=l;
	cmdbuf[4]=0x0;

	Result ret=0;
	if((ret=svc_sendSyncRequest(*handleptr)))return ret;

	*out=cmdbuf[3];

	return cmdbuf[1];
}

const u8 hexTable[]=
{
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

void hex2str(char* out, u32 val)
{
	int i;
	for(i=0;i<8;i++){out[7-i]=hexTable[val&0xf];val>>=4;}
	out[8]=0x00;
}

void drawHex(u32 val, int x, int y)
{
	char str[9];

	hex2str(str,val);
	drawString(TOPFBADR1,str,x,y);
	drawString(TOPFBADR2,str,x,y);
}

Result _APT_PrepareToStartSystemApplet(Handle handle, NS_APPID appId)
{
	u32* cmdbuf=getThreadCommandBuffer();
	
	cmdbuf[0]=0x00190040; //request header code
	cmdbuf[1]=appId;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(handle)))return ret;
	
	return cmdbuf[1];
}

Result _APT_StartSystemApplet(Handle handle, NS_APPID appId, u32* buffer, u32 bufferSize, Handle arg)
{
	u32* cmdbuf=getThreadCommandBuffer();
	
	cmdbuf[0]=0x001F0084; //request header code
	cmdbuf[1]=appId;
	cmdbuf[2]=bufferSize;
	cmdbuf[3]=0x0;
	cmdbuf[4]=arg;
	cmdbuf[5]=(bufferSize<<14)|0x2;
	cmdbuf[6]=(u32)buffer;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(handle)))return ret;
	
	return cmdbuf[1];
}

Result _GSPGPU_AcquireRight(Handle handle, u8 flags)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x160042; //request header code
	cmdbuf[1]=flags;
	cmdbuf[2]=0x0;
	cmdbuf[3]=0xffff8001;

	Result ret=0;
	if((ret=svc_sendSyncRequest(handle)))return ret;

	return cmdbuf[1];
}

Result _GSPGPU_ReleaseRight(Handle handle)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x170000; //request header code

	Result ret=0;
	if((ret=svc_sendSyncRequest(handle)))return ret;

	return cmdbuf[1];
}

#define _aptOpenSession() \
	svc_waitSynchronization1(aptLockHandle, U64_MAX);\
	_srv_getServiceHandle(srvHandle, &aptuHandle, "APT:U");\

#define _aptCloseSession()\
	svc_closeHandle(aptuHandle);\
	svc_releaseMutex(aptLockHandle);\

void doGspwn(u32* src, u32* dst, u32 size)
{
	Result (*nn__gxlow__CTR__CmdReqQueueTx__TryEnqueue)(u32** sharedGspCmdBuf, u32* cmdAdr)=(void*)CN_nn__gxlow__CTR__CmdReqQueueTx__TryEnqueue;
	u32 gxCommand[]=
	{
		0x00000004, //command header (SetTextureCopy)
		(u32)src, //source address
		(u32)dst, //destination address
		size, //size
		0xFFFFFFFF, // dim in
		0xFFFFFFFF, // dim out
		0x00000008, // flags
		0x00000000, //unused
	};

	u32** sharedGspCmdBuf=(u32**)(CN_GSPSHAREDBUF_ADR);
	nn__gxlow__CTR__CmdReqQueueTx__TryEnqueue(sharedGspCmdBuf, gxCommand);
}

Result _APT_ReceiveParameter(Handle handle, NS_APPID appID, u32 bufferSize, u32* buffer, u32* actualSize, u8* signalType, Handle* outHandle)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0xD0080; //request header code
	cmdbuf[1]=appID;
	cmdbuf[2]=bufferSize;
	
	cmdbuf[0+0x100/4]=(bufferSize<<14)|2;
	cmdbuf[1+0x100/4]=(u32)buffer;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(handle)))return ret;

	if(!cmdbuf[1])
	{
		if(signalType)*signalType=cmdbuf[3];
		if(actualSize)*actualSize=cmdbuf[4];
		if(outHandle)*outHandle=cmdbuf[6];
	}

	return cmdbuf[1];
}

Result _APT_CancelParameter(Handle handle, NS_APPID appID)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0xF0100; //request header code
	cmdbuf[1]=0x0;
	cmdbuf[2]=0x0;
	cmdbuf[3]=0x0;
	cmdbuf[4]=appID;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(handle)))return ret;

	return cmdbuf[1];
}

Result _HB_FlushInvalidateCache(Handle handle)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x00010042; //request header code
	cmdbuf[1]=0x00100000;
	cmdbuf[2]=0x00000000;
	cmdbuf[3]=0xFFFF8001;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(handle)))return ret;

	return cmdbuf[1];
}

Result _HB_SetupBootloader(Handle handle, u32 addr)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x00020042; //request header code
	cmdbuf[1]=addr;
	cmdbuf[2]=0x00000000;
	cmdbuf[3]=0xFFFF8001;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(handle)))return ret;

	return cmdbuf[1];
}

Result _HB_GetHandle(Handle handle, u32 index, Handle* out)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x00040040; //request header code
	cmdbuf[1]=index;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(handle)))return ret;

	if(out)*out=cmdbuf[3];

	return cmdbuf[1];
}

void bruteforceCloseHandle(u16 index, u32 maxCnt)
{
	int i;
	for(i=0;i<maxCnt;i++)if(!svc_closeHandle((index)|(i<<15)))return;
}

int main()
{
	int line=10;
	drawString(TOPFBADR1,"spiderto",0,line);
	drawString(TOPFBADR2,"spiderto",0,line+=10);

	Handle* srvHandle=(Handle*)CN_SRVHANDLE_ADR;
	Handle* gspHandle=(Handle*)CN_GSPHANDLE_ADR;

	Handle aptLockHandle=*((Handle*)CN_APTLOCKHANDLE_ADR);
	Handle aptuHandle=0x00;
	Result ret;

	u8 recvbuf[0x1000];

	Result (*_GSPGPU_FlushDataCache)(Handle* handle, Handle kprocess, u32* addr, u32 size)=(void*)CN_GSPGPU_FlushDataCache_ADR;

	{
		u32 buf;

		_GSPGPU_ReleaseRight(*gspHandle); //disable GSP module access

		_aptOpenSession();
			ret=_APT_PrepareToStartSystemApplet(aptuHandle, APPID_WEB);
		_aptCloseSession();
		drawHex(ret,0,line+=10);

		_aptOpenSession();
			ret=_APT_StartSystemApplet(aptuHandle, APPID_WEB, &buf, 0, 0);
		_aptCloseSession();
		drawHex(ret,0,line+=10);

		svc_sleepThread(100000000); //sleep just long enough for menu to grab rights

		_GSPGPU_AcquireRight(*gspHandle, 0x0); //get in line for gsp rights

		//need to sleep longer on 4.x ?
		svc_sleepThread(1000000000); //sleep long enough for spider to startup

		//read spider memory
		{
			_GSPGPU_FlushDataCache(gspHandle, 0xFFFF8001, (u32*)0x14100000, 0x00000200);
			doGspwn((u32*)(SPIDER_HOOKROP_PADR-0x0C000000), (u32*)0x14100000, 0x00000200);
		}

		svc_sleepThread(1000000); //sleep long enough for memory to be read

		//patch memdump and write it
		{
			((u8*)0x14100000)[0x14]=0xFF;
			memcpy(((u8*)(0x14100000+SPIDER_HOOKROP_OFFSET)), spider_hook_rop_bin, 0xC);
			_GSPGPU_FlushDataCache(gspHandle, 0xFFFF8001, (u32*)0x14100000, 0x00000200);

			doGspwn((u32*)0x14100000, (u32*)(SPIDER_HOOKROP_PADR-0x0C000000), 0x00000200);
		}

		svc_sleepThread(100000000);

		{
			memset((u8*)0x14100000, 0x00, 0x2000);
			memcpy((u8*)0x14100000, spider_initial_rop_bin, spider_initial_rop_bin_size);
			_GSPGPU_FlushDataCache(gspHandle, 0xFFFF8001, (u32*)0x14100000, 0x1000);

			doGspwn((u32*)0x14100000, (u32*)(SPIDER_INITIALROP_PADR-0x0C000000), 0x1000);
		}

		svc_sleepThread(100000000);

		{
			memset((u8*)0x14100000, 0x00, 0x2000);
			memcpy((u8*)0x14100000, spider_thread0_rop_bin, spider_thread0_rop_bin_size);
			_GSPGPU_FlushDataCache(gspHandle, 0xFFFF8001, (u32*)0x14100000, 0x2000);

			doGspwn((u32*)0x14100000, (u32*)(SPIDER_THREAD0ROP_PADR-0x0C000000), 0x2000);
		}

		svc_sleepThread(100000000);//sleep long enough for memory to be written

		_aptOpenSession();
			_APT_CancelParameter(aptuHandle, APPID_WEB);
		_aptCloseSession();

		//cleanup
		{
			//unmap GSP and HID shared mem
			svc_unmapMemoryBlock(*((Handle*)CN_HIDMEMHANDLE_ADR), 0x10000000);
			svc_unmapMemoryBlock(*((Handle*)CN_GSPMEMHANDLE_ADR), 0x10002000);

			Handle _srvHandle=*srvHandle;
			Handle _gspHandle=*gspHandle;

			//close all handles in data and .bss sections
			int i;
			for(i=0;i<(CN_DATABSS_SIZE)/4;i++)
			{
				Handle val=((Handle*)(CN_DATABSS_START))[i];
				if(val && (val&0x7FFF)<0x30 && val!=_srvHandle && val!=_gspHandle)svc_closeHandle(val);
			}

			//bruteforce the cnt part of the remaining 3 handles
			bruteforceCloseHandle(0x4, 0x1FFFF);
			bruteforceCloseHandle(0xA, 0x1FFFF);
			bruteforceCloseHandle(0x1E, 0x1FFFF);

			//free GSP heap and regular heap
			u32 out;
			svc_controlMemory(&out, 0x08000000, 0x00000000, CN_HEAPSIZE, MEMOP_FREE, 0x0);
		}

		_GSPGPU_ReleaseRight(*gspHandle); //disable GSP module access
	}

	svc_sleepThread(100000000); //sleep just long enough for spider to grab rights

	_GSPGPU_AcquireRight(*gspHandle, 0x0); //get in line for gsp rights

	u32* debug=(u32*)CN_DATABSS_START;
	debug[6]=0xDEADBABE;
	debug[7]=0xDEADBABE;
	debug[8]=0xDEADBABE;
	debug[9]=0xDEADBABE;

	ret=0xC880CFFA;
	while(ret==0xC880CFFA || ret==0xC8A0CFEF)
	{
		_aptOpenSession();
			ret=_APT_ReceiveParameter(aptuHandle, APPID_APPLICATION, 0x1000, (u32*)recvbuf, &debug[6], (u8*)&debug[7], (Handle*)&debug[8]);
			debug[0]=0xDEADCAFE;
			debug[1]=0xDEADBABE;
			debug[2]=debug[1]+1;
			debug[3]=debug[2]+1;
			debug[4]=debug[3]+1;
			debug[5]=ret;
		_aptCloseSession();
	}

	Handle hbHandle=debug[8];
	debug[8]=0x0;
	_HB_FlushInvalidateCache(hbHandle);
	Handle fsHandle;
	debug[8]=_HB_GetHandle(hbHandle, 0x0, &fsHandle);

	//allocate some memory for the bootloader code
	u32 out; ret=svc_controlMemory(&out, 0x13FF0000, 0x00000000, 0x00008000, MEMOP_COMMIT, 0x3);

	int i;
	for(i=0;i<0x1000;i++)
	{
		drawHex(line++,0,40);
		drawHex(0x00DEAD01,0,50);
		_GSPGPU_FlushDataCache(gspHandle, 0xFFFF8001, (u32*)TOPFBADR1, 0x46500*2);
	}

	memcpy((u8*)0x13FF0000, cn_bootloader_bin, cn_bootloader_bin_size);

	for(i=0;i<0x1000;i++)
	{
		drawHex(line++,0,40);
		drawHex(0x00DEAD02,0,50);
		_GSPGPU_FlushDataCache(gspHandle, 0xFFFF8001, (u32*)TOPFBADR1, 0x46500*2);
	}
	
	if(_HB_SetupBootloader(hbHandle, 0x13FF0000))*((u32*)NULL)=0xBABE0061;

	for(i=0;i<0x1000;i++)
	{
		drawHex(line++,0,40);
		drawHex(0x00DEAD03,0,50);
		_GSPGPU_FlushDataCache(gspHandle, 0xFFFF8001, (u32*)TOPFBADR1, 0x46500*2);
	}
	
	svc_controlMemory(&out, 0x14000000, 0x00000000, 0x02000000, MEMOP_FREE, 0x0);

	void (*callBootloader)(Handle hb, Handle file)=(void*)0x000F0000;
	_GSPGPU_ReleaseRight(*gspHandle); //disable GSP module access
	svc_closeHandle(*gspHandle);

	//open sdmc 3dsx file
	Handle fileHandle;
	FS_archive sdmcArchive=(FS_archive){0x9, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FS_path filePath=(FS_path){PATH_CHAR, 11, (u8*)"/boot.3dsx"};
	if((ret=FSUSER_OpenFileDirectly(fsHandle, &fileHandle, sdmcArchive, filePath, FS_OPEN_READ, FS_ATTRIBUTE_NONE))!=0)*(u32*)ret=0xCAFE0038;

	callBootloader(hbHandle, fileHandle);

	while(1);
	return 0;
}
