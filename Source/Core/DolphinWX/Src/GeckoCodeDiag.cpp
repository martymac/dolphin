
#include "GeckoCodeDiag.h"

#ifdef _WIN32
#define _WINSOCK2API_
#endif
#include <SFML/Network/Http.hpp>

#include <sstream>

#define _connect_macro_(b, f, c, s)	(b)->Connect(wxID_ANY, (c), wxCommandEventHandler(f), (wxObject*)0, (wxEvtHandler*)s)

namespace Gecko
{

static const wxString wxstr_name(_("Name: ")),
	wxstr_notes(_("Notes: ")),
	wxstr_creator(_("Creator: "));

CodeConfigPanel::CodeConfigPanel(wxWindow* const parent)
	: wxPanel(parent, -1, wxDefaultPosition, wxDefaultSize)
{
	m_listbox_gcodes = new wxCheckListBox(this, -1, wxDefaultPosition, wxDefaultSize);
	_connect_macro_(m_listbox_gcodes, CodeConfigPanel::UpdateInfoBox, wxEVT_COMMAND_LISTBOX_SELECTED, this);
	_connect_macro_(m_listbox_gcodes, CodeConfigPanel::ToggleCode, wxEVT_COMMAND_CHECKLISTBOX_TOGGLED, this);

	m_infobox.label_name = new wxStaticText(this, -1, wxstr_name);
	m_infobox.label_creator = new wxStaticText(this, -1, wxstr_creator);
	m_infobox.label_notes = new wxStaticText(this, -1, wxstr_notes);
	m_infobox.textctrl_notes = new wxTextCtrl(this, -1, wxEmptyString, wxDefaultPosition, wxSize(64, -1), wxTE_MULTILINE | wxTE_READONLY);
	m_infobox.listbox_codes = new wxListBox(this, -1, wxDefaultPosition, wxSize(-1, 64));

	// TODO: buttons to add/edit codes

	// sizers
	wxBoxSizer* const sizer_infobox = new wxBoxSizer(wxVERTICAL);
	sizer_infobox->Add(m_infobox.label_name, 0, wxBOTTOM, 5);
	sizer_infobox->Add(m_infobox.label_creator, 0,  wxBOTTOM, 5);
	sizer_infobox->Add(m_infobox.label_notes, 0, wxBOTTOM, 5);
	sizer_infobox->Add(m_infobox.textctrl_notes, 0, wxBOTTOM | wxEXPAND, 5);
	sizer_infobox->Add(m_infobox.listbox_codes, 1, wxEXPAND, 5);

	// button sizer
	wxBoxSizer* const sizer_buttons = new wxBoxSizer(wxHORIZONTAL);
	wxButton* const btn_download = new wxButton(this, -1, _("Download Codes (WiiRD Database)"), wxDefaultPosition, wxSize(128, -1));
	_connect_macro_(btn_download, CodeConfigPanel::DownloadCodes, wxEVT_COMMAND_BUTTON_CLICKED, this);
	sizer_buttons->AddStretchSpacer(1);
	sizer_buttons->Add(btn_download, 1, wxEXPAND);

	// horizontal sizer
	wxBoxSizer* const sizer_vert = new wxBoxSizer(wxVERTICAL);
	sizer_vert->Add(sizer_infobox, 1, wxEXPAND);
	sizer_vert->Add(sizer_buttons, 0, wxEXPAND | wxTOP, 5);
	
	wxBoxSizer* const sizer_main = new wxBoxSizer(wxVERTICAL);
	sizer_main->Add(m_listbox_gcodes, 1, wxALL | wxEXPAND, 5);
	sizer_main->Add(sizer_vert, 0, wxALL | wxEXPAND, 5);

	SetSizerAndFit(sizer_main);
}

void CodeConfigPanel::UpdateCodeList()
{
	m_listbox_gcodes->Clear();
	// add the codes to the listbox
	std::vector<GeckoCode>::const_iterator
		gcodes_iter = m_gcodes.begin(),
		gcodes_end = m_gcodes.end();
	for (; gcodes_iter!=gcodes_end; ++gcodes_iter)
	{
		m_listbox_gcodes->Append(wxString::FromAscii(gcodes_iter->name.c_str()));
		if (gcodes_iter->enabled)
			m_listbox_gcodes->Check(m_listbox_gcodes->GetCount()-1, true);
	}
	
	wxCommandEvent evt;
	UpdateInfoBox(evt);
}

void CodeConfigPanel::LoadCodes(const IniFile& inifile, const std::string& gameid)
{
	m_gameid = gameid;

	m_gcodes.clear();
	Gecko::LoadCodes(inifile, m_gcodes);

	UpdateCodeList();
}

void CodeConfigPanel::ToggleCode(wxCommandEvent& evt)
{
	const int sel = evt.GetInt();	// this right?
	if (sel > -1)
		m_gcodes[sel].enabled = m_listbox_gcodes->IsChecked(sel);
}

void CodeConfigPanel::UpdateInfoBox(wxCommandEvent&)
{
	m_infobox.listbox_codes->Clear();
	const int sel = m_listbox_gcodes->GetSelection();

	if (sel > -1)
	{
		m_infobox.label_name->SetLabel(wxstr_name + wxString::FromAscii(m_gcodes[sel].name.c_str()));

		// notes textctrl
		m_infobox.textctrl_notes->Clear();
		std::vector<std::string>::const_iterator
			notes_iter = m_gcodes[sel].notes.begin(),
			notes_end = m_gcodes[sel].notes.end();
		for (; notes_iter!=notes_end; ++notes_iter)
			m_infobox.textctrl_notes->AppendText(wxString::FromAscii(notes_iter->c_str()));
		m_infobox.textctrl_notes->ScrollLines(-99);	// silly

		m_infobox.label_creator->SetLabel(wxstr_creator + wxString::FromAscii(m_gcodes[sel].creator.c_str()));

		// add codes to info listbox
		std::vector<GeckoCode::Code>::const_iterator
		codes_iter = m_gcodes[sel].codes.begin(),
		codes_end = m_gcodes[sel].codes.end();
		for (; codes_iter!=codes_end; ++codes_iter)
			m_infobox.listbox_codes->Append(wxString::Format(wxT("%08X %08X"), codes_iter->address, codes_iter->data));
	}
	else
	{
		m_infobox.label_name->SetLabel(wxstr_name);
		m_infobox.textctrl_notes->Clear();
		m_infobox.label_creator->SetLabel(wxstr_creator);
	}
}

//void CodeConfigPanel::ApplyChanges(wxCommandEvent&)
//{
//	Gecko::SetActiveCodes(m_gcodes);
//}

void CodeConfigPanel::DownloadCodes(wxCommandEvent&)
{
	if (m_gameid.empty())
		return;

	std::string gameid = m_gameid;

	// WiiWare are identified by their first four characters
	if (m_gameid[0] == 'W')
		gameid = m_gameid.substr(0, 4);

	sf::Http::Request req;
	req.SetURI("/txt.php?txt=" + gameid);

	sf::Http http;
	http.SetHost("geckocodes.org");

	const sf::Http::Response resp = http.SendRequest(req, 5.0f);

	if (sf::Http::Response::Ok == resp.GetStatus())
	{
		// temp vector containing parsed codes
		std::vector<GeckoCode>	gcodes;

		// parse the codes
		std::istringstream ss(resp.GetBody());

		// debug
		//PanicAlert("File size is %i bytes.", ss.str().size());

		std::string line;

		// make sure the txt file is for this game
		// eh w/e
		//std::getline(ss, line);
		//if (line != m_gameid)
		//	PanicAlert("Bad code file.");

		// seek past the header, get to the first code
		std::getline(ss, line);
		std::getline(ss, line);
		std::getline(ss, line);

		int read_state = 0;
		GeckoCode gcode;

		while ((std::getline(ss, line).good()))
		{
			// empty line
			if (0 == line.size() || line == "\r" || line == "\n")	// \r\n checks might not be needed
			{
				// add the code
				if (gcode.codes.size())
					gcodes.push_back(gcode);
				gcode = GeckoCode();
				read_state = 0;
				continue;
			}

			switch (read_state)
			{
				// read new code
			case 0 :
			{
				std::istringstream ssline(line);
				// stop at [ character (begining of contributer name)
				std::getline(ssline, gcode.name, '[');
				gcode.name = StripSpaces(gcode.name);
				// read the code creator name
				std::getline(ssline, gcode.creator, ']');
				read_state = 1;
			}
				break;

				// read code lines
			case 1 :
			{
				std::istringstream ssline(line);	
				std::string addr, data;
				ssline >> addr >> data;
				ssline.seekg(0);

				// check if this line a code, silly, but the dumb txt file comment lines can start with valid hex chars :/
				if (8 == addr.length() && 8 == data.length())
				{
					GeckoCode::Code new_code;
					new_code.original_line = line;
					ssline >> std::hex >> new_code.address >> new_code.data;
					gcode.codes.push_back(new_code);
				}
				else
				{
					gcode.notes.push_back(line);
					read_state = 2;	// start reading comments
				}

			}
				break;

				// read comment lines
			case 2 :
				// append comment line
				gcode.notes.push_back(line);
				break;

			}
		}

		// add the last code
		if (gcode.codes.size())
			gcodes.push_back(gcode);

		if (gcodes.size())
		{
			unsigned long added_count = 0;

			// append the codes to the code list
			std::vector<GeckoCode>::const_iterator
				gcodes_iter = gcodes.begin(),
				gcodes_end = gcodes.end();
			for (; gcodes_iter!= gcodes_end; ++gcodes_iter)
			{
				// only add codes which do not already exist
				std::vector<GeckoCode>::const_iterator
					existing_gcodes_iter = m_gcodes.begin(),
					existing_gcodes_end = m_gcodes.end();
				for (;; ++existing_gcodes_iter)
				{
					if (existing_gcodes_end == existing_gcodes_iter)
					{
						m_gcodes.push_back(*gcodes_iter);
						++added_count;
						break;
					}

					// code with this name+creator exists
					if (existing_gcodes_iter->name == gcodes_iter->name &&
						existing_gcodes_iter->creator == gcodes_iter->creator)
						break;
				}
			}

			PanicAlert(_wxt("Downloaded %lu codes. (added %lu)"),
				(unsigned long)gcodes.size(), added_count);

			// refresh the list
			UpdateCodeList();
		}
		else
			PanicAlert("%s", _wxt("File contained no codes."));
	}
	else
		PanicAlert("%s", _wxt("Failed to download codes."));
}

}

