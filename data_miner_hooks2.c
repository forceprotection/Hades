

//
// This is a file that will be generated by a plugin
//

#include <ntddk.h>
#include "hades.h"
#include "ntimage.h"
#include "DataMineUserProcess.h"
#include "GeneratedDataMiner.h"

static void (__cdecl * Real_foo2)(int) = NULL;
static void (__cdecl * Real_foo4)(int, int) = NULL;

extern PMDL gmdlUserProcess;

//-----------------------------------------------------------------------------------------------------------------------------
// This is the area of memory that will be shared with the Kernel and User Process
// Note: My identifier to filter on - don't use "push [Real_func] since the mem storage would be in kernel space, the user 
// process could not deref that memory
//
// Each ID code block is 0xA bytes in length
//-----------------------------------------------------------------------------------------------------------------------------
void __declspec(naked) _cdecl SharedMemory_DataMining_OGA(void)
{  
	_asm {
		pushad             /* push all registers */
		push 0x409870      /* ID */
		jmp dword ptr MyHandler
		pushad             /* push all registers */
		push 0x4098B0      /* ID */
		jmp dword ptr MyHandler
MyHandler:
		mov eax, 0x61      /* ZwLoadDriver identifier */
		mov edx, esp
		_emit 0x0F         /* sysenter */
		_emit 0x34

	}
}



//-----------------------------------------------------------------------------------------------------------------------------
// Hooked functions
//-----------------------------------------------------------------------------------------------------------------------------
void __cdecl Mine_foo2(int a)
{
	DbgPrint("foo2(%X)\n", a);

	restore_context_switch_OGA();

	_asm
	{
		// Execute stolen bytes
		_emit 0x55                //push ebp
		_emit 0x8B                //mov ebp, esp
		_emit 0xEC
		_emit 0x8B                //mov eax, dword ptr [ebp+8]
		_emit 0x45
		_emit 0x08

		// Jump to user process
		add gID, 6
		jmp gID
	}
}

void __cdecl Mine_foo4(int a,int b)
{
	DbgPrint("foo4(%X, %X)\n", a, b);

	restore_context_switch_OGA();

	_asm
	{
		// Execute stolen bytes
		_emit 0x55                //push ebp
		_emit 0x8B                //mov ebp, esp
		_emit 0xEC
		_emit 0x51                //push ecx
		_emit 0x8B                //mov eax, dword ptr [ebp+8]
		_emit 0x45
		_emit 0x08

		// Jump to user process
		add gID, 7
		jmp gID
	}
}

//-----------------------------------------------------------------------------------------------------------------------------
// Process loaded - now data mine it
//-----------------------------------------------------------------------------------------------------------------------------
VOID DataMining_OGA(IN PUNICODE_STRING  FullImageName, IN HANDLE  ProcessId, IN PIMAGE_INFO  ImageInfo)
{
	static int replaced = 0;
	unsigned int *memory = NULL;
	UNICODE_STRING u_targetProcess;

	DbgPrint("name = %ws  PID = 0x%x\n", FullImageName->Buffer, ProcessId);
	RtlInitUnicodeString(&u_targetProcess, L"\\Device\\HarddiskVolume1\\Documents and Settings\\hp1\\Desktop\\Hades\\Hello.exe");

	if (RtlCompareUnicodeString(FullImageName, &u_targetProcess, TRUE) == 0)
	{
		int sizeofImage = 0x100;
		unsigned int *pStartAddr = 0x409870;

		//-------------------------------------------------------------------------------------------------------------------------
		// Probes the specified virtual memory pages, makes them resident, and locks them in memory
		//-------------------------------------------------------------------------------------------------------------------------
		gmdlUserProcess = MmCreateMdl(NULL, pStartAddr, 0xF00);
		MmProbeAndLockPages(gmdlUserProcess, KernelMode, IoReadAccess);

		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		//
		// ADD HOOKED CODE HERE
		//
		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		Real_foo2 = (int (__cdecl *)(int))0x409870;
		add_RerouteCode_OGA(Real_foo2, Mine_foo2);

		Real_foo4 = (int (__cdecl *)(int, int))0x4098B0;
		add_RerouteCode_OGA(Real_foo4, Mine_foo4);

		//-------------------------------------------------------------------------------------------------------------------------
		// Copy shared memory function to shared user space memory
		//-------------------------------------------------------------------------------------------------------------------------
		CLEAR_WP_FLAG;
		RtlCopyMemory((PVOID)g_shared_kernelMem, SharedMemory_DataMining_OGA, SIZE_OF_SHARED_MEM);
		RESTORE_CR0;

		if (gmdlUserProcess)
		{
			MmUnlockPages(gmdlUserProcess);
			IoFreeMdl(gmdlUserProcess);
		}

	}
}
