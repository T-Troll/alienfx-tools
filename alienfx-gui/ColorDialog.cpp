#include "alienfx-gui.h"

extern void SwitchLightTab(HWND, int);
extern bool SetColor(HWND hDlg, int id, groupset* mmap, AlienFX_SDK::afx_act* map);
extern AlienFX_SDK::Colorcode *Act2Code(AlienFX_SDK::afx_act*);
extern groupset* CreateMapping(int lid);
extern groupset* FindMapping(int mid);
extern void RemoveUnused(vector<groupset>* lightsets);
extern void RedrawButton(HWND hDlg, unsigned id, AlienFX_SDK::Colorcode*);
extern HWND CreateToolTip(HWND hwndParent, HWND oldTip);
extern void SetSlider(HWND tt, int value);
extern int UpdateZoneList(HWND hDlg, byte flag = 0);

extern int eItem;

extern zone* FindAmbMapping(int lid);
extern haptics_map* FindHapMapping(int lid);
extern void RemoveHapMapping(int devid, int lightid);
extern void RemoveAmbMapping(int devid, int lightid);
extern void RemoveLightFromGroup(AlienFX_SDK::group* grp, WORD devid, WORD lightid);

BOOL CALLBACK TabColorGrid(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
extern void CreateGridBlock(HWND gridTab, DLGPROC);
extern void OnGridSelChanged(HWND);
extern AlienFX_SDK::mapping* FindCreateMapping();
void RedrawGridButtonZone(bool recalc = false);

extern int gridTabSel;
extern AlienFX_SDK::lightgrid* mainGrid;

//vector<AlienFX_SDK::group*> lghGrp;

int effID = -1;

void SetEffectData(HWND hDlg, groupset* mmap) {
	bool hasEffects = false;
	if (mmap && mmap->color.size()) {
		//switch (fxhl->LocateDev(mmap->lights[0]->devid) ? fxhl->LocateDev(mmap->lights[0]->devid)->dev->GetVersion() : -1) {
		//case -1: case 1: case 2: case 3: case 4: case 7:
		//	// Have hardware effects
			hasEffects = true;
		//	break;
		//}
		// Set data
		ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_TYPE1), mmap->color[effID].type);
		SendMessage(GetDlgItem(hDlg, IDC_SPEED1), TBM_SETPOS, true, mmap->color[effID].tempo);
		SetSlider(sTip1, mmap->color[effID].tempo);
		SendMessage(GetDlgItem(hDlg, IDC_LENGTH1), TBM_SETPOS, true, mmap->color[effID].time);
		SetSlider(sTip2, mmap->color[effID].time);
	}
	EnableWindow(GetDlgItem(hDlg, IDC_TYPE1), hasEffects && !mmap->group->have_power);
	EnableWindow(GetDlgItem(hDlg, IDC_SPEED1), hasEffects);
	EnableWindow(GetDlgItem(hDlg, IDC_LENGTH1), hasEffects);
	RedrawButton(hDlg, IDC_BUTTON_C1, mmap && mmap->color.size() ? Act2Code(&mmap->color[effID]) : 0);
}

void RebuildEffectList(HWND hDlg, groupset* mmap) {
	HWND eff_list = GetDlgItem(hDlg, IDC_LEFFECTS_LIST),
		s1_slider = GetDlgItem(hDlg, IDC_SPEED1),
		l1_slider = GetDlgItem(hDlg, IDC_LENGTH1),
		type_c1 = GetDlgItem(hDlg, IDC_TYPE1);
	ListView_DeleteAllItems(eff_list);
	HIMAGELIST hOld = ListView_GetImageList(eff_list, LVSIL_SMALL);
	if (hOld) {
		ImageList_Destroy(hOld);
	}
	ListView_SetExtendedListViewStyle(eff_list, LVS_EX_FULLROWSELECT);
	if (!ListView_GetColumnWidth(eff_list, 0)) {
		LVCOLUMNA lCol{ LVCF_FMT, LVCFMT_LEFT };
		ListView_InsertColumn(eff_list, 0, &lCol);
	}
	if (mmap) {
		COLORREF* picData = NULL;
		HBITMAP colorBox = NULL;
		HIMAGELIST hSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
								  GetSystemMetrics(SM_CYSMICON),
								  ILC_COLOR32, 1, 1);
		for (int i = 0; i < mmap->color.size(); i++) {
			LVITEMA lItem{ LVIF_TEXT | LVIF_IMAGE | LVIF_STATE, i };
			char efName[16]{ 0 };
			picData = new COLORREF[GetSystemMetrics(SM_CXSMICON) * GetSystemMetrics(SM_CYSMICON)];
			fill_n(picData, GetSystemMetrics(SM_CXSMICON) * GetSystemMetrics(SM_CYSMICON), RGB(mmap->color[i].b, mmap->color[i].g, mmap->color[i].r));
			colorBox = CreateBitmap(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 1, 32, picData);
			delete[] picData;
			ImageList_Add(hSmall, colorBox, NULL);
			DeleteObject(colorBox);
			lItem.iImage = i;
			switch (mmap->color[i].type) {
			case AlienFX_SDK::AlienFX_A_Color:
				LoadString(hInst, IDS_TYPE_COLOR, efName, 16);
				break;
			case AlienFX_SDK::AlienFX_A_Pulse:
				LoadString(hInst, IDS_TYPE_PULSE, efName, 16);
				break;
			case AlienFX_SDK::AlienFX_A_Morph:
				LoadString(hInst, IDS_TYPE_MORPH, efName, 16);
				break;
			case AlienFX_SDK::AlienFX_A_Spectrum:
				LoadString(hInst, IDS_TYPE_SPECTRUM, efName, 16);
				break;
			case AlienFX_SDK::AlienFX_A_Breathing:
				LoadString(hInst, IDS_TYPE_BREATH, efName, 16);
				break;
			case AlienFX_SDK::AlienFX_A_Rainbow:
				LoadString(hInst, IDS_TYPE_RAINBOW, efName, 16);
				break;
			case AlienFX_SDK::AlienFX_A_Power:
				LoadString(hInst, IDS_TYPE_POWER, efName, 16);
				break;
			}
			lItem.pszText = efName;
			// check selection...
			if (i == effID) {
				lItem.state = LVIS_SELECTED;
			}
			ListView_InsertItem(eff_list, &lItem);
		}
		ListView_SetImageList(eff_list, hSmall, LVSIL_SMALL);
	}
	SetEffectData(hDlg, mmap);
	ListView_SetColumnWidth(eff_list, 0, LVSCW_AUTOSIZE);// width);
	ListView_EnsureVisible(eff_list, effID, false);
}

//int FindLastActiveID() {
//
//	int devid = eItem < 0x10000 ? conf->afx_dev.GetMappings()->at(eItem)->devid : 0,
//		lightid = eItem < 0x10000 ? conf->afx_dev.GetMappings()->at(eItem)->lightid : eItem;
//
//	switch (conf->GetEffect()) {
//	case 0: case 3:
//	{
//		if (!conf->GetEffect())
//			// first check effects....
//			//find_if()
//			for (int iter = (int)conf->activeProfile->lightsets.size() - 1; iter >= 0; iter--) {
//				lightset* Iter = &conf->activeProfile->lightsets[iter];
//				if (Iter->flags & ~LEVENT_COLOR) {
//					if (Iter->devid == devid && Iter->lightid == lightid) {
//						return iter;
//					}
//					for (auto gIter = lghGrp.begin(); gIter < lghGrp.end(); gIter++)
//						if (Iter->lightid == (*gIter)->gid) {
//							return iter;
//						}
//				}
//			}
//		// now check colors only...
//		for (int iter = (int)conf->activeProfile->lightsets.size() - 1; iter >= 0; iter--) {
//			lightset* Iter = &conf->activeProfile->lightsets[iter];
//			if (Iter->flags & LEVENT_COLOR) {
//				if (Iter->devid == devid && Iter->lightid == lightid) {
//					return iter;
//				}
//				for (auto gIter = lghGrp.begin(); gIter < lghGrp.end(); gIter++)
//					if (Iter->lightid == (*gIter)->gid) {
//						return iter;
//					}
//			}
//		}
//	} break;
//	case 1:
//	{
//		for (int iter = (int)conf->amb_conf->zones.size() - 1; iter >= 0; iter--) {
//			zone* Iter = &conf->amb_conf->zones[iter];
//			if (Iter->devid || !devid) {
//				if (Iter->devid == devid && Iter->lightid == lightid) {
//					return iter;
//				}
//			}
//			else {
//				// check groups...
//				for (auto gIter = lghGrp.begin(); gIter < lghGrp.end(); gIter++)
//					if (Iter->lightid == (*gIter)->gid) {
//						return iter;
//					}
//			}
//		}
//	} break;
//	case 2:
//	{
//		for (int iter = (int)conf->hap_conf->haptics.size() - 1; iter >= 0; iter--) {
//			haptics_map* Iter = &conf->hap_conf->haptics[iter];
//			if (Iter->devid || !devid) {
//				if (Iter->devid == devid && Iter->lightid == lightid) {
//					return iter;
//				}
//			}
//			else {
//				// check groups...
//				for (auto gIter = lghGrp.begin(); gIter < lghGrp.end(); gIter++)
//					if (Iter->lightid == (*gIter)->gid) {
//						return iter;
//					}
//			}
//		}
//	} break;
//	}
//	return -1;
//}
//
//void SetLightColors(HWND hDlg, int id) {
//
//	if (id >= 0) {
//		lightset* mmap = FindMapping(id);
//		zone* amap = FindAmbMapping(id);
//		haptics_map* hmap = FindHapMapping(id);
//
//		AlienFX_SDK::Colorcode* from_c, * to_c, * from_e, * to_e, * from_h, * to_h, * from_f, * to_f;
//		from_c = to_c = from_e = to_e = from_h = to_h = from_f = to_f = { NULL };
//
//		if (mmap) {
//			int actIndex = mmap->flags & LEVENT_ACT ? 3 :
//				mmap->flags & LEVENT_PERF ? 2 :
//				mmap->flags & LEVENT_POWER ? 1 : 0;
//			if (actIndex) {
//				from_e = Act2Code(&mmap->eve[actIndex].map[0]);
//				to_e = Act2Code(&mmap->eve[actIndex].map[1]);
//			}
//			if (mmap->flags & LEVENT_COLOR) {
//				from_c = Act2Code(&mmap->eve[0].map.front());
//				to_c = Act2Code(&mmap->eve[0].map.back());
//				if (from_e)
//					from_e = from_c;
//			}
//		}
//		if (hmap) {
//			from_h = &hmap->freqs[0].colorfrom;
//			to_h = &hmap->freqs[0].colorto;
//		}
//		RedrawButton(hDlg, IDC_FROM_COLOR, from_c);
//		RedrawButton(hDlg, IDC_TO_COLOR, to_c);
//		RedrawButton(hDlg, IDC_FROM_EFFECT, from_e);
//		RedrawButton(hDlg, IDC_TO_EFFECT, to_e);
//		RedrawButton(hDlg, IDC_FROM_HAPTICS, from_h);
//		RedrawButton(hDlg, IDC_TO_HAPTICS, to_h);
//		CheckDlgButton(hDlg, IDC_CHECK_AMBIENT, amap != NULL);
//
//		int idf = FindLastActiveID();
//		if (idf >= 0) {
//			switch (conf->GetEffect()) {
//			case 0: // monitoring
//			{
//				mmap = &conf->activeProfile->lightsets[idf];
//				int actIndex = mmap->flags & LEVENT_ACT ? 3 :
//					mmap->flags & LEVENT_PERF ? 2 :
//					mmap->flags & LEVENT_POWER ? 1 : 0;
//				if (actIndex) {
//					from_f = Act2Code(&mmap->eve[actIndex].map[0]);
//					to_f = Act2Code(&mmap->eve[actIndex].map[1]);
//				}
//				if (mmap->flags & LEVENT_COLOR) {
//					if (!from_f)
//						to_f = Act2Code(&mmap->eve[0].map.back());
//					from_f = Act2Code(&mmap->eve[0].map.front());
//				}
//			} break;
//			case 2: // haptics
//			{
//				hmap = &conf->hap_conf->haptics[idf];
//				from_f = &hmap->freqs[0].colorfrom;
//				to_f = &hmap->freqs[0].colorto;
//			} break;
//			case 3: // off
//			{
//				mmap = &conf->activeProfile->lightsets[idf];
//				if (mmap->flags & LEVENT_COLOR) {
//					to_f = Act2Code(&mmap->eve[0].map.back());
//					from_f = Act2Code(&mmap->eve[0].map.front());
//				}
//			} break;
//			}
//		}
//
//		RedrawButton(hDlg, IDC_FROM_FINAL, from_f);
//		RedrawButton(hDlg, IDC_TO_FINAL, to_f);
//	}
//}

BOOL CALLBACK TabColorDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND light_list = GetDlgItem(hDlg, IDC_LIGHTS),
		s1_slider = GetDlgItem(hDlg, IDC_SPEED1),
		l1_slider = GetDlgItem(hDlg, IDC_LENGTH1),
		type_c1 = GetDlgItem(hDlg, IDC_TYPE1),
		grpList = GetDlgItem(hDlg, IDC_GRPLIST),
		gridTab = GetDlgItem(hDlg, IDC_TAB_COLOR_GRID);

	groupset* mmap = FindMapping(eItem);

	//int devid = eItem >= 0 && eItem < 0x10000 ? conf->afx_dev.GetMappings()->at(eItem)->devid : 0,
	//	lightid = eItem >= 0 && eItem < 0x10000 ? conf->afx_dev.GetMappings()->at(eItem)->lightid : eItem;

	switch (message)
	{
	case WM_INITDIALOG:
	{
		if (eItem = UpdateZoneList(hDlg) < 0) {
			// no lights, switch to setup
			SwitchLightTab(hDlg, TAB_DEVICES);
			return false;
		}
		// Set types list...
		char buffer[100];
		LoadString(hInst, IDS_TYPE_COLOR, buffer, 100);
		ComboBox_AddString(type_c1, buffer);
		LoadString(hInst, IDS_TYPE_PULSE, buffer, 100);
		ComboBox_AddString(type_c1, buffer);
		LoadString(hInst, IDS_TYPE_MORPH, buffer, 100);
		ComboBox_AddString(type_c1, buffer);
		LoadString(hInst, IDS_TYPE_BREATH, buffer, 100);
		ComboBox_AddString(type_c1, buffer);
		LoadString(hInst, IDS_TYPE_SPECTRUM, buffer, 100);
		ComboBox_AddString(type_c1, buffer);
		LoadString(hInst, IDS_TYPE_RAINBOW, buffer, 100);
		ComboBox_AddString(type_c1, buffer);
		// now sliders...
		SendMessage(s1_slider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
		SendMessage(l1_slider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
		//TBM_SETTICFREQ
		SendMessage(s1_slider, TBM_SETTICFREQ, 32, 0);
		SendMessage(l1_slider, TBM_SETTICFREQ, 32, 0);
		sTip1 = CreateToolTip(s1_slider, sTip1);
		sTip2 = CreateToolTip(l1_slider, sTip2);

		// init grids...
		CreateGridBlock(gridTab, (DLGPROC)TabColorGrid);
		TabCtrl_SetCurSel(gridTab, gridTabSel);
		OnGridSelChanged(gridTab);

		// Restore selection....
		if (eItem >= 0) {
			SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_LIGHTS, LBN_SELCHANGE), (LPARAM)light_list);
		}

	} break;
	case WM_COMMAND: {
		int lid = (int) ListBox_GetItemData(light_list, ListBox_GetCurSel(light_list));
		switch (LOWORD(wParam))
		{
		case IDC_LIGHTS:
			switch (HIWORD(wParam))
			{
			case LBN_SELCHANGE:
				eItem = lid;// lbItem;
				effID = 0;
				mmap = FindMapping(lid);
				//if (mmap) {
				//	AlienFX_SDK::mapping *map = conf->afx_dev.GetMappingById(mmap->devid, mmap->lightid);
				//	if (map && (map->flags & ALIENFX_FLAG_POWER)) {
				//		mmap->eve[0].map[0].type = mmap->eve[0].map[1].type = AlienFX_SDK::AlienFX_A_Power;
				//		if (!mmap->eve[0].map[0].time)
				//			mmap->eve[0].map[0].time = mmap->eve[0].map[1].time = 3;
				//		if (!mmap->eve[0].map[0].tempo)
				//			mmap->eve[0].map[0].tempo = mmap->eve[0].map[1].tempo = 0x64;
				//	}
				//}
				RebuildEffectList(hDlg, mmap);
				RedrawGridButtonZone();
				// update light info...
				//devid = eItem < 0x10000 ? conf->afx_dev.GetMappings()->at(eItem)->devid : 0;
				//lightid = eItem < 0x10000 ? conf->afx_dev.GetMappings()->at(eItem)->lightid : eItem;
				//lghGrp.clear();
				//ListBox_ResetContent(grpList);
				//if (devid) {
				//	ListBox_SetItemData(grpList, ListBox_AddString(grpList, "(Light)"), eItem);
				//	for (auto Iter = conf->afx_dev.GetGroups()->begin(); Iter < conf->afx_dev.GetGroups()->end(); Iter++)
				//		for (auto lIter = Iter->lights.begin(); lIter < Iter->lights.end(); lIter++)
				//			if ((*lIter).first == devid && (*lIter).second == lightid) {
				//				lghGrp.push_back(&(*Iter));
				//				ListBox_SetItemData(grpList, ListBox_AddString(grpList, Iter->name.c_str()), Iter->gid);
				//			}
				//}
				//else {
				//	ListBox_SetItemData(grpList, ListBox_AddString(grpList, conf->afx_dev.GetGroupById(eItem)->name.c_str()), eItem);
				//}
				//ListBox_SetCurSel(grpList, 0);
				//EnableWindow(GetDlgItem(hDlg, IDC_REM_GROUP), false);
				//SetLightColors(hDlg, eItem);

				break;
			} break;
		case IDC_TYPE1:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				if (mmap != NULL) {
					int lType1 = (int)ComboBox_GetCurSel(type_c1);
					mmap->color[effID].type = lType1;
					RebuildEffectList(hDlg, mmap);
					fxhl->RefreshOne(mmap, true, true);
				}
			}
			break;
		case IDC_BUTTON_C1:
			switch (HIWORD(wParam))
			{
			case BN_CLICKED: {
				if (lid != -1) {
					if (!mmap) {
						// have selection, but no effect - let's create one!
						mmap = CreateMapping(lid);
						effID = 0;
					}
					SetColor(hDlg, IDC_BUTTON_C1, mmap, &mmap->color[effID]);
					RebuildEffectList(hDlg, mmap);
					//SetLightColors(hDlg, eItem);
				}
			} break;
			} break;
		case IDC_BUT_ADD_EFFECT:
			if (HIWORD(wParam) == BN_CLICKED) {
				AlienFX_SDK::afx_act act{ 0 };
				if (!mmap) {
					// create new mapping..
					mmap = CreateMapping(lid);
					if (mmap->color.size() && mmap->group->have_power) {
						act.type = AlienFX_SDK::AlienFX_A_Power;
						act.time = 3;
						act.tempo = 0x64;
						mmap->color.push_back(act);
						mmap->color.push_back(act);
					} else
						mmap->color.push_back(act);
					effID = 0;
				} else
					if (!mmap->group->have_power && mmap->color.size() < 9) {
						act = mmap->color[effID];
						mmap->color.push_back(act);
						effID = (int)mmap->color.size() - 1;
					}
				RebuildEffectList(hDlg, mmap);
				//SetLightColors(hDlg, eItem);
				fxhl->RefreshOne(mmap, true, true);
			}
			break;
		case IDC_BUTT_REMOVE_EFFECT:
			if (HIWORD(wParam) == BN_CLICKED && mmap) {
				if (mmap->group->have_power)
					mmap->color.clear();
				else {
					mmap->color.pop_back();
					if (effID == mmap->color.size())
						effID--;
				}
				// remove mapping if no colors and effects!
				if (mmap->color.empty()) {
					RemoveUnused(conf->active_set);
					RebuildEffectList(hDlg, NULL);
					effID = -1;
					fxhl->Refresh();
				} else {
					RebuildEffectList(hDlg, mmap);
					fxhl->RefreshOne(mmap, false, true);
				}
				//SetLightColors(hDlg, eItem);
			}
			break;
		case IDC_BUT_ADD_ZONE: {
			AlienFX_SDK::group* grp = conf->CreateGroup("New zone");
			conf->afx_dev.GetGroups()->push_back(*grp);
			eItem = grp->gid;
			UpdateZoneList(hDlg);
			//conf->afx_dev.SaveMappings();
		} break;
		case IDC_BUT_DEL_ZONE:
			if (eItem > 0) {
				// delete from all profiles...
				for (auto Iter = conf->profiles.begin(); Iter != conf->profiles.end(); Iter++) {
					auto pos = find_if((*Iter)->lightsets.begin(), (*Iter)->lightsets.end(),
						[](auto t) {
							return t.group->gid == eItem;
						});
					if (pos != (*Iter)->lightsets.end())
						(*Iter)->lightsets.erase(pos);
				}
				for (auto Iter = conf->afx_dev.GetGroups()->begin(); Iter != conf->afx_dev.GetGroups()->end(); Iter++)
					if (Iter->gid == eItem) {
						conf->afx_dev.GetGroups()->erase(Iter);
						break;
					}
				// ToDo: update UI

				/*conf->afx_dev.SaveMappings();
				conf->Save();*/
				//ComboBox_DeleteString(groups_list, gItem);
				//if (conf->afx_dev.GetGroups()->size() > 0) {
				//	if (eItem >= conf->afx_dev.GetGroups()->size())
				//		eItem--;
				//	gLid = conf->afx_dev.GetGroups()->at(gItem).gid;
				//	grp = &conf->afx_dev.GetGroups()->at(gItem);
				//	UpdateLightListG(light_list, grp);
				//}
				//else {
				//	eItem = -1;
				//}
			}
			break;
		//case IDC_BUTTON_SETALL:
		//	switch (HIWORD(wParam))
		//	{
		//	case BN_CLICKED: {
		//		if (mmap && mmap->eve[0].map[0].type != AlienFX_SDK::AlienFX_A_Power &&
		//			MessageBox(hDlg, "Do you really want to set all lights to this settings?", "Warning",
		//					   MB_YESNO | MB_ICONWARNING) == IDYES) {
		//			event light = mmap->eve[0];
		//			for (int i = 0; i < conf->afx_dev.GetMappings()->size(); i++) {
		//				AlienFX_SDK::mapping* map = conf->afx_dev.GetMappings()->at(i);
		//				if (!(map->flags & ALIENFX_FLAG_POWER)) {
		//					lightset* actmap = FindMapping(i);
		//					if (!actmap) {
		//						// create new mapping
		//						actmap = CreateMapping(i);
		//					}
		//					actmap->eve[0] = light;
		//				}
		//			}
		//			fxhl->RefreshState(true);
		//		}
		//	} break;
		//	} break;
		// info block controls.
		/*case IDC_GRPLIST:
			if (HIWORD(wParam) == LBN_SELCHANGE) {
				int selGrp = ListBox_GetCurSel(grpList);
				EnableWindow(GetDlgItem(hDlg, IDC_REM_GROUP), selGrp);
				EnableWindow(GetDlgItem(hDlg, IDC_REM_COLOR), !selGrp);
				EnableWindow(GetDlgItem(hDlg, IDC_REM_EFFECT), !selGrp);
				EnableWindow(GetDlgItem(hDlg, IDC_REM_AMBIENT), !selGrp);
				EnableWindow(GetDlgItem(hDlg, IDC_REM_HAPTICS), !selGrp);
				SetLightColors(hDlg, (int)ListBox_GetItemData(grpList, selGrp));
			}
			break;
		case IDC_REM_GROUP:
		{
			AlienFX_SDK::group* grp = conf->afx_dev.GetGroupById((DWORD)ListBox_GetItemData(grpList, ListBox_GetCurSel(grpList)));
			if (grp && devid) {
				ListBox_DeleteString(grpList, ListBox_GetCurSel(grpList));
				ListBox_SetCurSel(grpList, 0);
				RemoveLightFromGroup(grp, devid, lightid);
				SetLightColors(hDlg, eItem);
				fxhl->RefreshState(true);
			}
		} break;
		case IDC_REM_COLOR:
			if (mmap) {
				mmap->flags &= ~LEVENT_COLOR;
				if (!mmap->flags) {
					RemoveMapping(conf->active_set, mmap->devid, mmap->lightid);
					effID = -1;
					RebuildEffectList(hDlg, mmap);
					RedrawButton(hDlg, IDC_BUTTON_C1, NULL);
				}
				SetLightColors(hDlg, eItem);
				fxhl->RefreshState(true);
			}
			break;
		case IDC_REM_EFFECT:
			if (mmap) {
				mmap->flags &= LEVENT_COLOR;
				if (!mmap->flags) {
					RemoveMapping(conf->active_set, mmap->devid, mmap->lightid);
					effID = -1;
					RebuildEffectList(hDlg, mmap);
					RedrawButton(hDlg, IDC_BUTTON_C1, NULL);
				}
				SetLightColors(hDlg, eItem);
				fxhl->RefreshState(true);
			}
			break;
		case IDC_REM_AMBIENT:
			if (lightid >= 0) {
				RemoveAmbMapping(devid, lightid);
				SetLightColors(hDlg, eItem);
			}
			break;
		case IDC_REM_HAPTICS:
			if (lightid >= 0) {
				RemoveHapMapping(devid, lightid);
				SetLightColors(hDlg, eItem);
			}
			break;*/
		default: return false;
		}
	} break;
	case WM_VSCROLL:
		switch (LOWORD(wParam)) {
		case TB_THUMBTRACK: case TB_ENDTRACK:
			if (mmap != NULL) {
				if ((HWND)lParam == s1_slider) {
					mmap->color[effID].tempo = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
					SetSlider(sTip1, mmap->color[effID].tempo);
				}
				if ((HWND)lParam == l1_slider) {
					mmap->color[effID].time = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
					SetSlider(sTip2, mmap->color[effID].time);
				}
				fxhl->RefreshOne(mmap, true, true);
			}
			break;
		} break;
	case WM_DRAWITEM:
		switch (((DRAWITEMSTRUCT*)lParam)->CtlID) {
		case IDC_BUTTON_C1: {
			AlienFX_SDK::Colorcode* c{ NULL };
			if (mmap && effID < mmap->color.size()) {
				c = Act2Code(&mmap->color[effID]);
			}
			RedrawButton(hDlg, IDC_BUTTON_C1, c);
		} break;
		//case IDC_TO_FINAL:
		//	SetLightColors(hDlg, eItem);
		//	break;
		default: return false;
		}
		break;
	case WM_NOTIFY:
		switch (((NMHDR*)lParam)->idFrom) {
		case IDC_TAB_COLOR_GRID: {
			switch (((NMHDR*)lParam)->code) {
			case TCN_SELCHANGE: {
				int newSel = TabCtrl_GetCurSel(gridTab);
				if (newSel != gridTabSel) { // selection changed!
					if (newSel < conf->afx_dev.GetGrids()->size())
						OnGridSelChanged(gridTab);
				}
			} break;
			}
		} break;
		case IDC_LEFFECTS_LIST:
			switch (((NMHDR*) lParam)->code) {
			case LVN_ITEMCHANGED:
			{
				NMLISTVIEW* lPoint = (LPNMLISTVIEW) lParam;
				if (lPoint->uNewState & LVIS_FOCUSED) {
					// Select other item...
					effID = lPoint->iItem != -1 ? effID = lPoint->iItem : 0;
					SetEffectData(hDlg, mmap);
				}
			} break;
			case NM_DBLCLK:
			{
				// change color.
				if (eItem != -1) {
					if (!mmap) {
						// have selection, but no effect - let's create one!
						mmap = CreateMapping(eItem);
						effID = 0;
					}
					SetColor(hDlg, IDC_BUTTON_C1, mmap, &mmap->color[effID]);
					RebuildEffectList(hDlg, mmap);
				}
			} break;
			}
			break;
		case IDC_LIST_ZONES:
			switch (((NMHDR*)lParam)->code) {
			case LVN_ITEMACTIVATE: {
				ListView_EditLabel(((NMHDR*)lParam)->hwndFrom, ((NMITEMACTIVATE*)lParam)->iItem);
			} break;

			case LVN_ITEMCHANGED:
			{
				NMLISTVIEW* lPoint = (LPNMLISTVIEW)lParam;
				if (lPoint->uNewState && LVIS_FOCUSED && lPoint->iItem != -1) {
					// Select other item...
					eItem = (int)lPoint->lParam;// lbItem;
					effID = 0;
					mmap = FindMapping(eItem);
					RebuildEffectList(hDlg, mmap);
					RedrawGridButtonZone();
				}
				//else {
				//	/*eItem = -1;
				//	mmap = NULL;
				//	RebuildEffectList(hDlg, mmap);
				//	RedrawGridButtonZone();*/
				//}
			} break;
			case LVN_ENDLABELEDIT:
			{
				NMLVDISPINFO* sItem = (NMLVDISPINFO*)lParam;
				AlienFX_SDK::group* grp = conf->afx_dev.GetGroupById((int)sItem->item.lParam);
				if (grp && sItem->item.pszText) {
					grp->name = sItem->item.pszText;
					ListView_SetItem(((NMHDR*)lParam)->hwndFrom, &sItem->item);
					//ListView_SetColumnWidth(((NMHDR*)lParam)->hwndFrom, 0, LVSCW_AUTOSIZE);
					return true;
				}
				else
					return false;
			} break;
			}
			break;
		}
		break;
	default: return false;
	}
	return true;
}
