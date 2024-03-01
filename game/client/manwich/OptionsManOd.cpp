//========= Totally Not Copyright © 2024, Valve Cut Content, All rights reserved. ============//
//Options menu for Manwich's Odyssey stuff
//
//=============================================================================//

#include "cbase.h"
#include "ienginevgui.h"
#include "vgui_controls/PropertyPage.h"
#include "vgui_controls/CheckButton.h"
#include "vgui_controls/PropertyDialog.h"

using namespace vgui;

static const char* moduleName = "GameUI";

#define MODULENAME_PATCH 	const char* GetModuleName() OVERRIDE { return moduleName; }

class PatchedLabel : public Label
{
	DECLARE_CLASS_SIMPLE(PatchedLabel, Label);
	MODULENAME_PATCH;
	PatchedLabel(Panel* parent, const char* panelName, const char* text) : Label(parent, panelName, text) {}
};


class PatchedCheckButton : public CheckButton
{
	DECLARE_CLASS_SIMPLE(PatchedCheckButton, CheckButton);
	MODULENAME_PATCH;
	PatchedCheckButton(Panel* parent, const char* panelName, const char* text) : CheckButton(parent, panelName, text){}
};

class COptionsManOd : public PropertyPage
{
	DECLARE_CLASS_SIMPLE(COptionsManOd, PropertyPage);
public:
	COptionsManOd(Panel* parent);
	~COptionsManOd();

	void OnResetData();
	void OnApplyChanges();

	//Fixes for cross-module vgui stuff
	MODULENAME_PATCH;
	void SetBuildGroup(BuildGroup* buildGroup);
	static bool isAdded;

private:

	PatchedCheckButton*	m_pOldFire;
	PatchedCheckButton*	m_pOldBlood;
	PatchedCheckButton* m_pDisableAutoSwitch;
	PatchedCheckButton* m_pEnhanceStereo;
	PatchedLabel* m_pTopLabel;
};

bool COptionsManOd::isAdded = false;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
COptionsManOd::COptionsManOd(Panel* parent) : BaseClass(parent, NULL)
{
	m_pTopLabel = new PatchedLabel(this, "ManOdLabel", "#ManwichsOdyssey_Option_Label");
	m_pOldFire = new PatchedCheckButton(this, "OldFireCheck", "#ManwichsOdyssey_Option_OldFire");
	m_pOldBlood = new PatchedCheckButton(this, "OldBloodCheck", "#ManwichsOdyssey_Option_OldBlood");
	m_pDisableAutoSwitch = new PatchedCheckButton(this, "DisableAutoSwitchCheck", "#ManwichsOdyssey_Option_DisableAutoSwitch");
	m_pEnhanceStereo = new PatchedCheckButton(this, "EnhanceStereo", "#ManwichsOdyssey_Option_EnhanceStereo");

	m_pTopLabel->SetPos(30, 10);
	m_pTopLabel->SetSize(300, 30);

	m_pOldFire->SetSize(400, 30);
	m_pOldFire->SetPos(30, 40);

	m_pOldBlood->SetSize(400, 30);
	m_pOldBlood->SetPos(30, 70);

	m_pDisableAutoSwitch->SetSize(400, 30);
	m_pDisableAutoSwitch->SetPos(30, 100);

	m_pEnhanceStereo->SetSize(400, 30);
	m_pEnhanceStereo->SetPos(30, 130);

	isAdded = true;
}

COptionsManOd::~COptionsManOd()
{
	isAdded = false;
}

//-----------------------------------------------------------------------------
// Purpose: resets controls
//-----------------------------------------------------------------------------
void COptionsManOd::OnResetData()
{
	ConVarRef oldBloodVar("manod_old_blood");
	ConVarRef newFireVar("manod_fire_new");
	ConVarRef disableAutoSwitchVar("weapon_disable_autoswitch");
	ConVarRef enhanceStereoVar("dsp_enhance_stereo");

	m_pOldBlood->SetSelected(oldBloodVar.GetBool());
	m_pOldFire->SetSelected(!newFireVar.GetBool());
	m_pDisableAutoSwitch->SetSelected(disableAutoSwitchVar.GetBool());
	m_pEnhanceStereo->SetSelected(enhanceStereoVar.GetBool());
}

//-----------------------------------------------------------------------------
// Purpose: sets data based on control settings
//-----------------------------------------------------------------------------
void COptionsManOd::OnApplyChanges()
{
	ConVarRef oldBloodVar("manod_old_blood");
	ConVarRef newFireVar("manod_fire_new");
	ConVarRef disableAutoSwitchVar("weapon_disable_autoswitch");
	ConVarRef enhanceStereoVar("dsp_enhance_stereo");


	oldBloodVar.SetValue(m_pOldBlood->IsSelected());
	newFireVar.SetValue(!m_pOldFire->IsSelected());
	disableAutoSwitchVar.SetValue(m_pDisableAutoSwitch->IsSelected());
	enhanceStereoVar.SetValue(m_pEnhanceStereo->IsSelected());
}

// Static function to add this page to the optionsmenu
void AddCustomManOdOptions()
{
	//Already added, no need to add a new one
	if (COptionsManOd::isAdded)
		return;

	VPANEL vGameUI = enginevgui->GetPanel(PANEL_GAMEUIDLL);
	if (vGameUI == NULL)
		return;
	
	CUtlVector<VPANEL>& vectGameUIChildren = ipanel()->GetChildren(vGameUI);
	
	int i;
	VPANEL vBaseUI = NULL;
	for (i = 0; i < vectGameUIChildren.Count(); i++)
	{
		const char* string = ipanel()->GetName(vectGameUIChildren[i]);
		if (FStrEq("BaseGameUIPanel", string))
		{
			vBaseUI = vectGameUIChildren[i];
			break;
		}
	}
	if (vBaseUI == NULL)

	return;
	CUtlVector<VPANEL>& vectBaseUIChildren = ipanel()->GetChildren(vBaseUI);

	VPANEL vOptions = NULL;
	for (i = 0; i < vectBaseUIChildren.Count(); i++)
	{
		const char* string = ipanel()->GetName(vectBaseUIChildren[i]);

		if (FStrEq("OptionsDialog", string))
		{
			vOptions = vectBaseUIChildren[i];
			break;
		}
	}

	if (vOptions == NULL)
		return;

	PropertyDialog* pOptions = dynamic_cast<PropertyDialog*>(ipanel()->GetPanel(vOptions, "GameUI"));
	if (pOptions == NULL)
	{
		return;
	}
	pOptions->AddPage(new COptionsManOd(pOptions), "#ManwichsOdyssey_Option_PageName");
}

void COptionsManOd::SetBuildGroup(BuildGroup* buildGroup)
{
	//TODO: fix this, would cause a crash
	//BaseClass::SetBuildGroup(buildGroup);
}