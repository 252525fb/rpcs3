#pragma once
#include "Loader.h"

struct PsfHeader
{
	u32 psf_magic;
	u32 psf_version;
	u32 psf_offset_key_table;
	u32 psf_offset_values_table;
	u32 psf_entries_num;

	bool CheckMagic() const { return psf_magic == 0x46535000; }
};

class PSFLoader
{
	wxFile& psf_f;
	bool m_show_log;

public:
	PSFLoader(wxFile& f);
	PSFLoader(const wxString& path);

	wxArrayString m_table;
	GameInfo m_info;
	PsfHeader psfhdr;
	virtual bool Load(bool show = true);
	virtual bool Close();

private:
	bool LoadHdr();
	bool LoadKeyTable();
	bool LoadValuesTable();
};