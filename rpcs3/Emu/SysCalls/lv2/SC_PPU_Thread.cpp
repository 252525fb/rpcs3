#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"

#define PPU_THREAD_ID_INVALID 0xFFFFFFFFU

int SysCalls::lv2PPUThreadCreate(PPUThread& CPU)
{
	ConLog.Write("lv2PPUThreadCreate:");
	//ConLog.Write("**** id: %d", CPU.GPR[3]);
	ConLog.Write("**** entry: 0x%x", CPU.GPR[4]);
	ConLog.Write("**** arg: 0x%x", CPU.GPR[5]);
	ConLog.Write("**** prio: 0x%x", CPU.GPR[6]);
	ConLog.Write("**** stacksize: 0x%x", CPU.GPR[7]);
	ConLog.Write("**** flags: 0x%x", CPU.GPR[8]);
	ConLog.Write("**** threadname: \"%s\"[0x%x]", Memory.ReadString(CPU.GPR[9]), CPU.GPR[9]);

	if(!Memory.IsGoodAddr(CPU.GPR[4])) return PPU_THREAD_ID_INVALID;

	const u32 id = Emu.GetCPU().AddThread(true);

	CPU.GPR[3] = id;
	PPCThread& new_thread = *(PPCThread*)Emu.GetCPU().GetIDs().GetIDData(id).m_data;
	new_thread.SetPc(CPU.GPR[4]);
	new_thread.SetArg(CPU.GPR[5]);
	//CPU.GPR[6];
	new_thread.stack_size = CPU.GPR[7];
	//CPU.GPR[8];
	//new_thread.name = Memory.ReadString(CPU.GPR[9]);
	new_thread.Run();
	return CELL_OK;
}

int SysCalls::lv2PPUThreadExit(PPUThread& CPU)
{
	ConLog.Warning("PPU[%d] thread exit [return code: %lld]", CPU.GetId(), CPU.GPR[3]);
	CPU.Close();
	Emu.GetCPU().GetIDs().RemoveID(CPU.GetId());
	return CELL_OK;
}

int SysCalls::lv2PPUThreadYield(PPUThread& CPU)
{
	ConLog.Warning("TODO: PPU[%d] thread yield!", CPU.GetId());
	return CELL_OK;
}

int SysCalls::lv2PPUThreadJoin(PPUThread& CPU)
{
	ConLog.Warning("TODO: PPU[%d] thread join!", CPU.GPR[3]);
	return CELL_OK;
}

int SysCalls::lv2PPUThreadDetach(PPUThread& CPU)
{
	ConLog.Warning("TODO: PPU[%d] thread detach!", CPU.GPR[3]);
	return CELL_OK;
}

int SysCalls::lv2PPUThreadGetJoinState(PPUThread& CPU)
{
	ConLog.Warning("TODO: PPU[%d] thread detach!", CPU.GetId());
	return CELL_OK;
}

int SysCalls::lv2PPUThreadSetPriority(PPUThread& CPU)
{
	ConLog.Warning("TODO: PPU[%d] thread set priority!", CPU.GPR[3]);
	return CELL_OK;
}

int SysCalls::lv2PPUThreadGetPriority(PPUThread& CPU)
{
	ConLog.Warning("TODO: PPU[%d] thread get priority!", CPU.GPR[3]);
	return CELL_OK;
}

int SysCalls::lv2PPUThreadGetStackInformation(PPUThread& CPU)
{
	ConLog.Warning("PPU[%d] thread get stack information[r3: 0x%llx]", CPU.GetId(), CPU.GPR[3]);
	Memory.Write32(CPU.GPR[3],   CPU.GetStackAddr());
	Memory.Write32(CPU.GPR[3]+4, CPU.GetStackSize());
	return CELL_OK;
}

int SysCalls::lv2PPUThreadRename(PPUThread& CPU)
{
	ConLog.Write("PPU[%d] thread rename: [%s]", CPU.GPR[3], Memory.ReadString(CPU.GPR[4]));
	return 0;
}

int SysCalls::lv2PPUThreadRecoverPageFault(PPUThread& CPU)
{
	ConLog.Warning("TODO: PPU[%d] thread recover page fault!", CPU.GPR[3]);
	return CELL_OK;
}

int SysCalls::lv2PPUThreadGetPageFaultContext(PPUThread& CPU)
{
	ConLog.Warning("TODO: PPU[%d] thread get page fault context!", CPU.GPR[3]);
	return CELL_OK;
}