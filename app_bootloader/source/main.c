#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctr/types.h>
#include <ctr/srv.h>
#include <ctr/svc.h>
#include <ctr/FS.h>
#include <ctr/GSP.h>
#include <ctr/GX.h>

#include "sys.h"
#include "3dsx.h"

#include "../../build/constants.h"

u8* _heap_base; // should be 0x08000000
const u32 _heap_size = 0x01000000;

u8* gspHeap;
u32* gxCmdBuf;
Handle gspEvent, gspSharedMemHandle;

void gspGpuInit()
{
	gspInit();

	GSPGPU_AcquireRight(NULL, 0x0);

	//set subscreen to red
	u32 regData=0x010000FF;
	GSPGPU_WriteHWRegs(NULL, 0x202A04, &regData, 4);

	//setup our gsp shared mem section
	u8 threadID;
	svc_createEvent(&gspEvent, 0x0);
	GSPGPU_RegisterInterruptRelayQueue(NULL, gspEvent, 0x1, &gspSharedMemHandle, &threadID);
	svc_mapMemoryBlock(gspSharedMemHandle, 0x10002000, 0x3, 0x10000000);

	//wait until we can write stuff to it
	svc_waitSynchronization1(gspEvent, 0x55bcb0);

	//GSP shared mem : 0x2779F000
	gxCmdBuf=(u32*)(0x10002000+0x800+threadID*0x200);
}

void* getOutputBaseAddr()
{
	//map GSP heap
	svc_controlMemory((u32*)&gspHeap, 0x0, 0x0, 0x01000000, 0x10003, 0x3);

	return &gspHeap[0x00100000];
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

void doGspwn(u32* src, u32* dst, u32 size)
{
	size += 0x1f;
	size &= ~0x1f;
	GX_SetTextureCopy(gxCmdBuf, src, 0xFFFFFFFF, dst, 0xFFFFFFFF, size, 0x00000008);
}

Result NSS_LaunchTitle(Handle* handle, u64 tid, u8 flags)
{
	if(!handle)return -1;

	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x000200C0; //request header code
	cmdbuf[1]=tid&0xFFFFFFFF;
	cmdbuf[2]=(tid>>32)&0xFFFFFFFF;
	cmdbuf[3]=flags;

	Result ret=0;
	if((ret=svc_sendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

Result NSS_TerminateProcessTID(Handle* handle, u64 tid, u64 timeout)
{
	if(!handle)return -1;

	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x00110100; //request header code
	cmdbuf[1]=tid&0xFFFFFFFF;
	cmdbuf[2]=(tid>>32)&0xFFFFFFFF;
	cmdbuf[3]=timeout&0xFFFFFFFF;
	cmdbuf[4]=(timeout>>32)&0xFFFFFFFF;

	Result ret=0;
	if((ret=svc_sendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

typedef struct {
    u32 base_addr;
    u32 size;
    u32 perm;
    u32 state;
} MemInfo;

typedef struct {
    u32 flags;
} PageInfo;

Result svc_queryMemory(MemInfo* info, PageInfo* out, u32 addr);
Result svc_duplicateHandle(Handle* output, Handle input);
Result svcControlProcessMemory(Handle KProcess, unsigned int Addr0, unsigned int Addr1, unsigned int Size, unsigned int Type, unsigned int Permissions);
int _Load3DSX(Handle file, Handle process, void* baseAddr, service_list_t* __service_ptr);
extern service_list_t _serviceList;
void start_execution(void);

typedef struct {
	u32 num;
	u32 text_end;
	u32 data_address;
	u32 data_size;
	struct {
		u32 src, dst, size;
	} map[];
} memorymap_t;

const memorymap_t camapp_map =
	{
		4,
		0x00347000,
		0x00429000,
		0x00046680 + 0x00099430,
		{
			{0x00100000, 0x00008000, 0x00300000 - 0x00008000},
			{0x00100000 + 0x00300000 - 0x00008000, - 0x00070000, 0x00070000},
			{0x00100000 + 0x00300000 + 0x00070000 - 0x00008000, - 0x00100000, 0x00090000},
			{0x00100000 + 0x00300000 + 0x00070000 + 0x00090000 - 0x00008000, - 0x00109000, 0x00009000},
		}
	};

const memorymap_t dlplay_map =
	{
		4,
		0x00193000,
		0x001A0000,
		0x00013790 + 0x0002A538,
		{
			{0x00100000, 0x00008000, 0x000B0000 - 0x00008000},
			{0x00100000 + 0x000B0000 - 0x00008000, - 0x000B4000, 0x00004000},
			{0x00100000 + 0x000B4000 - 0x00008000, - 0x000DE000, 0x0002A000},
			{0x00100000 + 0x000B4000 + 0x0002A000 - 0x00008000, - 0x000D2000, 0x00002000},
		}
	};

const memorymap_t actapp_map =
	{
		4,
		0x00388000,
		0x003F3000,
		0x0001A2FC + 0x00061ED4,
		{
			{0x00100000, 0x00008000, 0x00300000 - 0x00008000},
			{0x00100000 + 0x00300000 - 0x00008000, - 0x0000E000, 0x0000E000},
			{0x00100000 + 0x00300000 + 0x0000E000 - 0x00008000, - 0x00010000, 0x00002000},
			{0x00100000 + 0x00300000 + 0x0000E000 + 0x00002000 - 0x00008000, - 0x00070000, 0x00060000},
		}
	};

const memorymap_t * const app_maps[] =
	{
		(memorymap_t*)&camapp_map, // camera app
		(memorymap_t*)&dlplay_map, // dlplay app
		(memorymap_t*)&actapp_map, // act app
	};

const u32 _targetProcessIndex = 0xBABE0001;
const u32 _APP_START_LINEAR = 0xBABE0002;

void apply_map(memorymap_t* m)
{
	if(!m)return;
	int i;
	vu32* APP_START_LINEAR = &_APP_START_LINEAR;
	for(i=0; i<m->num; i++)
	{
		int remaining_size = m->map[i].size;
		u32 offset = 0;
		while(remaining_size > 0)
		{
			int size = remaining_size;
			if(size > 0x00080000)size = 0x00080000;

			GSPGPU_FlushDataCache(NULL, (u8*)&gspHeap[m->map[i].src + offset], size);
			svc_sleepThread(10*1000*1000);
			doGspwn((u32*)&gspHeap[m->map[i].src + offset], (u32*)(*APP_START_LINEAR + m->map[i].dst + offset), size);
			svc_sleepThread(10*1000*1000);

			remaining_size -= size;
			offset += size;
		}
	}
}

void setup3dsx(Handle executable, memorymap_t* m, service_list_t* serviceList, u32* argbuf)
{
	if(!m)return;

	Result ret = Load3DSX(executable, (void*)(0x00100000 + 0x00008000), (void*)m->data_address, m->data_size, serviceList, argbuf);

	apply_map(m);

	// sleep for 50ms
	svc_sleepThread(50*1000*1000);
}

void freeDataPages(u32 address)
{
	MemInfo minfo;
	PageInfo pinfo;
	Result ret = svc_queryMemory(&minfo, &pinfo, address);
	if(!ret)
	{
		u32 tmp;
		svc_controlMemory(&tmp, minfo.base_addr, 0x0, minfo.size, 0x1, 0x0);
	}
}

void svc_backdoor(void* callback);
void _invalidate_icache();

void invalidate_icache()
{
	svc_backdoor((void*)&_invalidate_icache);
}

void run3dsx(Handle executable, u32* argbuf)
{
	initSrv();
	gspGpuInit();

	// free extra data pages if any
	freeDataPages(0x14000000);
	freeDataPages(0x30000000);

	// duplicate service list on the stack
	u8 serviceBuffer[0x4+0xC*_serviceList.num];
	service_list_t* serviceList = (service_list_t*)serviceBuffer;
	serviceList->num = _serviceList.num;
	int i;
	for(i=0; i<_serviceList.num; i++)
	{
		memcpy(serviceList->services[i].name, _serviceList.services[i].name, 8);
		svc_duplicateHandle(&serviceList->services[i].handle, _serviceList.services[i].handle);
	}

	vu32* targetProcessIndex = &_targetProcessIndex;
	setup3dsx(executable, (memorymap_t*)app_maps[*targetProcessIndex], serviceList, argbuf);
	FSFILE_Close(executable);

	gspGpuExit();
	exitSrv();
	
	// grab ns:s handle
	Handle nssHandle = 0x0;
	for(i=0; i<_serviceList.num; i++)if(!strcmp(_serviceList.services[i].name, "ns:s"))nssHandle=_serviceList.services[i].handle;
	if(!nssHandle)*(vu32*)0xCAFE0001=0;

	// use ns:s to launch/kill process and invalidate icache in the process
	Result ret = NSS_LaunchTitle(&nssHandle, 0x0004013000003702LL, 0x1);
	if(ret)*(vu32*)0xCAFE0002=ret;
	svc_sleepThread(200*1000*1000);
	ret = NSS_TerminateProcessTID(&nssHandle, 0x0004013000003702LL, 100*1000*1000);
	if(ret)*(vu32*)0xCAFE0003=ret;

	// invalidate_icache();

	// free heap (has to be the very last thing before jumping to app as contains bss)
	u32 out; svc_controlMemory(&out, (u32)_heap_base, 0x0, _heap_size, MEMOP_FREE, 0x0);

	start_execution();
}

void runHbmenu()
{
	// grab fs:USER handle
	int i;
	Handle fsuHandle = 0x0;
	for(i=0; i<_serviceList.num; i++)if(!strcmp(_serviceList.services[i].name, "fs:USER"))fsuHandle=_serviceList.services[i].handle;
	if(!fsuHandle)*(vu32*)0xCAFE0002=0;

	Handle fileHandle;
	FS_archive sdmcArchive = (FS_archive){0x9, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FS_path filePath = (FS_path){PATH_CHAR, 18, (u8*)"/3ds_hb_menu.3dsx"};
	Result ret = FSUSER_OpenFileDirectly(fsuHandle, &fileHandle, sdmcArchive, filePath, FS_OPEN_READ, FS_ATTRIBUTE_NONE);

	run3dsx(fileHandle, NULL);
}
