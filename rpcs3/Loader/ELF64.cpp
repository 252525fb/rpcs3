#include "stdafx.h"
#include "ELF64.h"
#include "Gui/CompilerELF.h"
using namespace PPU_opcodes;

ELF64Loader::ELF64Loader(vfsStream& f)
	: elf64_f(f)
	, LoaderBase()
{
}

bool ELF64Loader::LoadInfo()
{
	if(!elf64_f.IsOpened()) return false;

	if(!LoadEhdrInfo()) return false;
	if(!LoadPhdrInfo()) return false;
	if(!LoadShdrInfo()) return false;

	return true;
}

bool ELF64Loader::LoadData(u64 offset)
{
	if(!elf64_f.IsOpened()) return false;

	if(!LoadEhdrData(offset)) return false;
	if(!LoadPhdrData(offset)) return false;
	if(!LoadShdrData(offset)) return false;

	return true;
}

bool ELF64Loader::Close()
{
	return elf64_f.Close();
}

bool ELF64Loader::LoadEhdrInfo(s64 offset)
{
	elf64_f.Seek(offset < 0 ? 0 : offset);
	ehdr.Load(elf64_f);

	if(!ehdr.CheckMagic()) return false;

	if(ehdr.e_phentsize != sizeof(Elf64_Phdr))
	{
		ConLog.Error("elf64 error:  e_phentsize[0x%x] != sizeof(Elf64_Phdr)[0x%x]", ehdr.e_phentsize, sizeof(Elf64_Phdr));
		return false;
	}

	if(ehdr.e_shentsize != sizeof(Elf64_Shdr))
	{
		ConLog.Error("elf64 error: e_shentsize[0x%x] != sizeof(Elf64_Shdr)[0x%x]", ehdr.e_shentsize, sizeof(Elf64_Shdr));
		return false;
	}

	switch(ehdr.e_machine)
	{
	case MACHINE_PPC64:
	case MACHINE_SPU:
		machine = (Elf_Machine)ehdr.e_machine;
	break;

	default:
		machine = MACHINE_Unknown;
		ConLog.Error("Unknown elf64 type: 0x%x", ehdr.e_machine);
		return false;
	}

	entry = ehdr.GetEntry();
	if(entry == 0)
	{
		ConLog.Error("elf64 error: entry is null!");
		return false;
	}

	return true;
}

bool ELF64Loader::LoadPhdrInfo(s64 offset)
{
	phdr_arr.Clear();

	if(ehdr.e_phoff == 0 && ehdr.e_phnum)
	{
		ConLog.Error("LoadPhdr64 error: Program header offset is null!");
		return false;
	}

	elf64_f.Seek(offset < 0 ? ehdr.e_phoff : offset);

	for(u32 i=0; i<ehdr.e_phnum; ++i)
	{
		Elf64_Phdr* phdr = new Elf64_Phdr();
		phdr->Load(elf64_f);
		phdr_arr.Move(phdr);
	}

	return true;
}

bool ELF64Loader::LoadShdrInfo(s64 offset)
{
	shdr_arr.Clear();
	shdr_name_arr.Clear();
	if(ehdr.e_shoff == 0 && ehdr.e_shnum)
	{
		ConLog.Error("LoadShdr64 error: Section header offset is null!");
		return false;
	}

	elf64_f.Seek(offset < 0 ? ehdr.e_shoff : offset);
	for(u32 i=0; i<ehdr.e_shnum; ++i)
	{
		Elf64_Shdr* shdr = new Elf64_Shdr();
		shdr->Load(elf64_f);
		shdr_arr.Move(shdr);
	}

	if(ehdr.e_shstrndx >= shdr_arr.GetCount())
	{
		ConLog.Error("LoadShdr64 error: shstrndx too big!");
		return false;
	}

	for(u32 i=0; i<shdr_arr.GetCount(); ++i)
	{
		elf64_f.Seek((offset < 0 ? shdr_arr[ehdr.e_shstrndx].sh_offset : shdr_arr[ehdr.e_shstrndx].sh_offset - ehdr.e_shoff + offset) + shdr_arr[i].sh_name);
		wxString name = wxEmptyString;
		while(!elf64_f.Eof())
		{
			char c;
			elf64_f.Read(&c, 1);
			if(c == 0) break;
			name += c;
		}

		shdr_name_arr.Add(name);
	}

	return true;
}

bool ELF64Loader::LoadEhdrData(u64 offset)
{
#ifdef LOADER_DEBUG
	ConLog.SkipLn();
	ehdr.Show();
	ConLog.SkipLn();
#endif
	return true;
}

bool ELF64Loader::LoadPhdrData(u64 offset)
{
	for(u32 i=0; i<phdr_arr.GetCount(); ++i)
	{
		phdr_arr[i].Show();

		if(phdr_arr[i].p_vaddr != phdr_arr[i].p_paddr)
		{
			ConLog.Warning
			( 
				"ElfProgram different load addrs: paddr=0x%8.8x, vaddr=0x%8.8x", 
				phdr_arr[i].p_paddr, phdr_arr[i].p_vaddr
			);
		}

		if(!Memory.MainMem.IsInMyRange(offset + phdr_arr[i].p_vaddr, phdr_arr[i].p_memsz))
		{
#ifdef LOADER_DEBUG
			ConLog.Warning("Skipping...");
			ConLog.SkipLn();
#endif
			continue;
		}

		switch(phdr_arr[i].p_type)
		{
			case 0x00000001: //LOAD
				if(phdr_arr[i].p_memsz)
				{
					Memory.MainMem.Alloc(offset + phdr_arr[i].p_vaddr, phdr_arr[i].p_memsz);

					if(phdr_arr[i].p_filesz)
					{
						elf64_f.Seek(phdr_arr[i].p_offset);
						elf64_f.Read(&Memory[offset + phdr_arr[i].p_vaddr], phdr_arr[i].p_filesz);
					}
				}
			break;

			case 0x00000007: //TLS
				Emu.SetTLSData(offset + phdr_arr[i].p_vaddr, phdr_arr[i].p_filesz, phdr_arr[i].p_memsz);
			break;

			case 0x60000001: //LOOS+1
			{
				if(!phdr_arr[i].p_filesz) break;

				const sys_process_param& proc_param = *(sys_process_param*)&Memory[offset + phdr_arr[i].p_vaddr];

				if(re(proc_param.size) < sizeof(sys_process_param))
				{
					ConLog.Warning("Bad proc param size! [0x%x : 0x%x]", re(proc_param.size), sizeof(sys_process_param));
				}
	
				if(re(proc_param.magic) != 0x13bcc5f6)
				{
					ConLog.Error("Bad magic! [0x%x]", Memory.Reverse32(proc_param.magic));
				}
				else
				{
					sys_process_param_info& info = Emu.GetInfo().GetProcParam();
					info.sdk_version = re(proc_param.info.sdk_version);
					info.primary_prio = re(proc_param.info.primary_prio);
					info.primary_stacksize = re(proc_param.info.primary_stacksize);
					info.malloc_pagesize = re(proc_param.info.malloc_pagesize);
					info.ppc_seg = re(proc_param.info.ppc_seg);
					//info.crash_dump_param_addr = re(proc_param.info.crash_dump_param_addr);
#ifdef LOADER_DEBUG
					ConLog.Write("*** sdk version: 0x%x", info.sdk_version);
					ConLog.Write("*** primary prio: %d", info.primary_prio);
					ConLog.Write("*** primary stacksize: 0x%x", info.primary_stacksize);
					ConLog.Write("*** malloc pagesize: 0x%x", info.malloc_pagesize);
					ConLog.Write("*** ppc seg: 0x%x", info.ppc_seg);
					//ConLog.Write("*** crash dump param addr: 0x%x", info.crash_dump_param_addr);
#endif
				}
			}
			break;

			case 0x60000002: //LOOS+2
			{
				if(!phdr_arr[i].p_filesz) break;

				sys_proc_prx_param proc_prx_param = *(sys_proc_prx_param*)&Memory[offset + phdr_arr[i].p_vaddr];

				proc_prx_param.size = re(proc_prx_param.size);
				proc_prx_param.magic = re(proc_prx_param.magic);
				proc_prx_param.version = re(proc_prx_param.version);
				proc_prx_param.libentstart = re(proc_prx_param.libentstart);
				proc_prx_param.libentend = re(proc_prx_param.libentend);
				proc_prx_param.libstubstart = re(proc_prx_param.libstubstart);
				proc_prx_param.libstubend = re(proc_prx_param.libstubend);
				proc_prx_param.ver = re(proc_prx_param.ver);

#ifdef LOADER_DEBUG
				ConLog.Write("*** size: 0x%x", proc_prx_param.size);
				ConLog.Write("*** magic: 0x%x", proc_prx_param.magic);
				ConLog.Write("*** version: 0x%x", proc_prx_param.version);
				ConLog.Write("*** libentstart: 0x%x", proc_prx_param.libentstart);
				ConLog.Write("*** libentend: 0x%x", proc_prx_param.libentend);
				ConLog.Write("*** libstubstart: 0x%x", proc_prx_param.libstubstart);
				ConLog.Write("*** libstubend: 0x%x", proc_prx_param.libstubend);
				ConLog.Write("*** ver: 0x%x", proc_prx_param.ver);
#endif

				if(proc_prx_param.magic != 0x1b434cec)
				{
					ConLog.Error("Bad magic! (0x%x)", proc_prx_param.magic);
				}
				else
				{
					for(u32 s=proc_prx_param.libstubstart; s<proc_prx_param.libstubend; s+=sizeof(Elf64_StubHeader))
					{
						Elf64_StubHeader stub = *(Elf64_StubHeader*)Memory.GetMemFromAddr(offset + s);

						stub.s_size = re(stub.s_size);
						stub.s_version = re(stub.s_version);
						stub.s_unk0 = re(stub.s_unk0);
						stub.s_unk1 = re(stub.s_unk1);
						stub.s_imports = re(stub.s_imports);
						stub.s_modulename = re(stub.s_modulename);
						stub.s_nid = re(stub.s_nid);
						stub.s_text = re(stub.s_text);

						const wxString& module_name = Memory.ReadString(stub.s_modulename);
						Module* module = GetModuleByName(module_name);
						if(module)
						{
							module->SetLoaded();
						}
						else
						{
							ConLog.Warning("Unknown module '%s'", module_name);
						}

#ifdef LOADER_DEBUG
						ConLog.SkipLn();
						ConLog.Write("*** size: 0x%x", stub.s_size);
						ConLog.Write("*** version: 0x%x", stub.s_version);
						ConLog.Write("*** unk0: 0x%x", stub.s_unk0);
						ConLog.Write("*** unk1: 0x%x", stub.s_unk1);
						ConLog.Write("*** imports: %d", stub.s_imports);
						ConLog.Write("*** module name: %s [0x%x]", module_name, stub.s_modulename);
						ConLog.Write("*** nid: 0x%x", stub.s_nid);
						ConLog.Write("*** text: 0x%x", stub.s_text);
#endif
						static const u32 section = 4 * 3;
						u64 tbl = Memory.MainMem.Alloc(stub.s_imports * 4 * 2);
						u64 dst = Memory.MainMem.Alloc(stub.s_imports * section);

						for(u32 i=0; i<stub.s_imports; ++i)
						{
							const u32 nid = Memory.Read32(stub.s_nid + i*4);
							const u32 text = Memory.Read32(stub.s_text + i*4);

							if(module)
							{
								if(!module->Load(nid))
								{
									ConLog.Warning("Unknown function 0x%08x in '%s' module", nid, module_name);
								}
							}
#ifdef LOADER_DEBUG
							ConLog.Write("import %d:", i+1);
							ConLog.Write("*** nid: 0x%x (0x%x)", nid, stub.s_nid + i*4);
							ConLog.Write("*** text: 0x%x (0x%x)", text, stub.s_text + i*4);
#endif
							Memory.Write32(stub.s_text + i*4, tbl + i*8);

							mem32_t out_tbl(tbl + i*8);
							out_tbl += dst + i*section;
							out_tbl += nid;

							mem32_t out_dst(dst + i*section);
							out_dst += ToOpcode(G_1f) | SetField(OR, 21, 30) | ToRA(11) | ToRS(2) | ToRB(2) | ToRC(0);
							out_dst += ToOpcode(SC) | ToSYS(2);
							out_dst += ToOpcode(G_13) | SetField(BCLR, 21, 30) | ToBO(0x10 | 0x04) | ToBI(0) | ToLK(0);
						}
					}
#ifdef LOADER_DEBUG
					ConLog.SkipLn();
#endif
				}
			}
			break;
		}
#ifdef LOADER_DEBUG
		ConLog.SkipLn();
#endif
	}

	return true;
}

bool ELF64Loader::LoadShdrData(u64 offset)
{
	u64 max_addr = 0;

	for(uint i=0; i<shdr_arr.GetCount(); ++i)
	{
		Elf64_Shdr& shdr = shdr_arr[i];

		if(i < shdr_name_arr.GetCount())
		{
			const wxString& name = shdr_name_arr[i];

#ifdef LOADER_DEBUG
			ConLog.Write("Name: %s", shdr_name_arr[i]);
#endif
		}

#ifdef LOADER_DEBUG
		shdr.Show();
		ConLog.SkipLn();
#endif
		if(shdr.sh_addr + shdr.sh_size > max_addr) max_addr = shdr.sh_addr + shdr.sh_size;

		if((shdr.sh_flags & SHF_ALLOC) != SHF_ALLOC) continue;

		const u64 addr = shdr.sh_addr;
		const u64 size = shdr.sh_size;

		if(size == 0 || !Memory.IsGoodAddr(offset + addr, size)) continue;

		switch(shdr.sh_type)
		{
		case SHT_NOBITS:
			memset(&Memory[offset + addr], 0, size);
		break;

		case SHT_PROGBITS:
			/*
			elf64_f.Seek(shdr.sh_offset);
			elf64_f.Read(&Memory[addr], shdr.sh_size);
			*/
		break;
		}
	}

	return true;
}