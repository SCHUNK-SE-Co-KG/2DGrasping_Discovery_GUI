/*
 * schunkdiscover - the network discovery tool for Schunk 2D Grasping Kit
 *
 * Copyright (c) 2017 Roboception GmbH
 * All rights reserved
 *
 * Author: Raphael Schaller
 * 
 * Copyright (c) 2024 Schunk SE & Co. KG
 * All rights reserved
 * 
 * Author: Divya Sasidharan
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

// placed here to make sure to include winsock2.h before windows.h
#include "schunkdiscover/wol.h"
#include "schunkdiscover/utils.h"
#include "sensor-command-dialog.h"
#include "discover-frame.h"

#include "discover-thread.h"
#include "event-ids.h"
#include "reset-dialog.h"
#include "force-ip-dialog.h"
#include "force-perm-ip-dialog.h"
#include "reconnect-dialog.h"
#include "about-dialog.h"
#include "resources.h"

#include <memory>
#include <sstream>
#include <algorithm>

#include <wx/frame.h>
#include <wx/dataview.h>
#include <wx/button.h>
#include <wx/animate.h>
#include <wx/mstream.h>
#include <wx/menu.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/clipbrd.h>
#include <wx/dc.h>
#include <wx/msgdlg.h>
#include <wx/html/helpctrl.h>
#include <wx/cshelp.h>
#include <wx/statline.h>
#include <wx/persist/toplevel.h>

#include "resources/logo_128.xpm"
#include "resources/logo_32_rotate.h"
#include "resources/loading_spinner_32.h"
#include "resources/loading_spinner_empty_frame_32.h"
#include <wx/utils.h>
#include <wx/wx.h>
#include <wx/busyinfo.h>
#include <cstdlib>

static bool isMadeByRc(const wxVector<wxVariant> &device)
{
  return device[DiscoverFrame::MANUFACTURER].GetString() == SCHUNK ||
    device[DiscoverFrame::MODEL].GetString().StartsWith("2D");
}

static bool isMadeByRc(const wxDataViewListCtrl &device_list, unsigned int row)
{
  return device_list.GetTextValue(row, DiscoverFrame::MANUFACTURER) == SCHUNK ||
    device_list.GetTextValue(row, DiscoverFrame::MODEL).StartsWith("2D");
}

static bool isRcVisard(const wxVector<wxVariant> &device)
{
  return device[DiscoverFrame::MODEL].GetString().StartsWith(SCHUNK);
}

static bool isRcVisard(const wxDataViewListCtrl &device_list, unsigned int row)
{
  return device_list.GetTextValue(row, DiscoverFrame::MODEL).StartsWith(SCHUNK);
}

DiscoverFrame::DiscoverFrame(const wxString& title,
                const wxPoint& pos) :
  wxFrame(NULL, wxID_ANY, title, pos, wxSize(1080,350)),
  device_list_(nullptr),
  discover_button_(nullptr),
  filter_input_(nullptr),
  reset_button_(nullptr),
  force_ip_button_(nullptr),
  force_perm_ip_button_(nullptr),
  reset_dialog_(nullptr),
  force_ip_dialog_(nullptr),
  force_perm_ip_dialog_(nullptr),
  about_dialog_(nullptr),
  menu_event_item_(nullptr),
  only_rc_sensors_(false),
  filter_text_()
{
  // spinner
  wxIcon icon_128(logo_128_xpm);
  SetIcon(icon_128);

  // wxMemoryInputStream gif_stream(logo_32_rotate_gif,
  //                                sizeof(logo_32_rotate_gif));
  // wxMemoryInputStream gif_stream(loading_spinner_32_gif,
  //                                sizeof(loading_spinner_32_gif));
  wxMemoryInputStream gif_stream(loading_spinner_empty_frame_32_gif,
                                 sizeof(loading_spinner_empty_frame_32_gif));
  spinner_.Load(gif_stream, wxANIMATION_TYPE_GIF);

  // menu
  wxMenu *menuFile = new wxMenu();
  menuFile->Append(wxID_EXIT);

  wxMenu *menuHelp = new wxMenu();
  menuHelp->Append(wxID_HELP);
  menuHelp->Append(wxID_ABOUT);

  wxMenuBar *menuBar = new wxMenuBar();
  menuBar->Append(menuFile, "&File");
  menuBar->Append(menuHelp, "&Help");

  SetMenuBar(menuBar);
  CreateStatusBar();

  // window content
  auto *panel = new wxPanel(this, wxID_ANY);
  auto *vbox = new wxBoxSizer(wxVERTICAL);

  // uppper buttons
  {
    auto *button_box = new wxBoxSizer(wxHORIZONTAL);
    discover_button_ = new wxButton(panel, ID_DiscoverButton, "Rerun Discovery");
    button_box->Add(discover_button_, 1);

    button_box->AddSpacer(10);
    button_box->Add(new wxStaticLine(panel, wxID_ANY, wxDefaultPosition,
                    wxSize(-1,30), wxLI_VERTICAL));
    button_box->AddSpacer(10);

    int w, h;
    discover_button_->GetSize(&w, &h);
    // auto *only_rc_cbox = new wxCheckBox(panel, ID_OnlyRcCheckbox,
    //                                     "Only rc_... devices",
    //                                     wxDefaultPosition, wxSize(-1, h));
    // only_rc_cbox->SetValue(only_rc_sensors_);
    // button_box->Add(only_rc_cbox, 1);

    // button_box->AddSpacer(10);
    // button_box->Add(new wxStaticLine(panel, wxID_ANY, wxDefaultPosition,
    //                 wxSize(-1,30), wxLI_VERTICAL));
    // button_box->AddSpacer(10);

    auto *filter_text = new wxStaticText(panel, wxID_ANY, "Filter");
    button_box->Add(filter_text, 1, wxTOP, 6);

    button_box->AddSpacer(10);
    filter_input_ = new wxTextCtrl(panel, ID_FilterTextInput, wxEmptyString,
                                   wxDefaultPosition, wxSize(150, -1));
    filter_input_->SetToolTip("Use * and ? as wildcards");
    button_box->Add(filter_input_, 0);

    button_box->Add(-1, 0, wxEXPAND);

    spinner_ctrl_ = new wxAnimationCtrl(panel, wxID_ANY, spinner_,
                                        wxPoint(-1,-1), wxSize(32,32));
    button_box->Add(spinner_ctrl_, 0);

    vbox->Add(button_box, 0, wxALL, 10);
  }

  // device table
  {
    auto *data_box = new wxBoxSizer(wxHORIZONTAL);

    device_list_ = new wxDataViewListCtrl(panel,
                                          ID_DataViewListCtrl,
                                          wxPoint(-1,-1),
                                          wxSize(-1,-1));
    device_list_->AppendTextColumn("Name",
                                   wxDATAVIEW_CELL_INERT,
                                   100, wxALIGN_LEFT,
                                   wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE);
    device_list_->AppendTextColumn("Manufacturer",
                                   wxDATAVIEW_CELL_INERT,
                                   170, wxALIGN_LEFT,
                                   wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE);
    device_list_->AppendTextColumn("Model",
                                   wxDATAVIEW_CELL_INERT,
                                   130, wxALIGN_LEFT,
                                   wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE);
    device_list_->AppendTextColumn("Serial Number",
                                   wxDATAVIEW_CELL_INERT,
                                   130, wxALIGN_LEFT,
                                   wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE);
    device_list_->AppendTextColumn("IP Address",
                                   wxDATAVIEW_CELL_INERT,
                                   120, wxALIGN_LEFT,
                                   wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE);
    device_list_->AppendTextColumn("MAC Address",
                                   wxDATAVIEW_CELL_INERT,
                                   130, wxALIGN_LEFT,
                                   wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE);
    device_list_->AppendTextColumn("Interface(s)",
                                   wxDATAVIEW_CELL_INERT,
                                   130, wxALIGN_LEFT,
                                   wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE);
    device_list_->AppendTextColumn("Reachable",
                                   wxDATAVIEW_CELL_INERT,
                                   80, wxALIGN_CENTER,
                                   wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE);

    device_list_->SetToolTip("Select row to enable open button.");

    data_box->Add(device_list_, 1, wxEXPAND);

    vbox->Add(data_box, 1, wxLEFT | wxRIGHT | wxEXPAND, 10);
  }

  {
    auto *button_box = new wxBoxSizer(wxHORIZONTAL);
    // reset_button_ = new wxButton(panel, ID_ResetButton, "Reset visard");
    // button_box->Add(reset_button_, 1);
    int w, h;
    // reset_button_->GetSize(&w, &h);
    // button_box->AddSpacer(10);

    force_ip_button_ = new wxButton(panel, ID_ForceIpButton,
                                    "Open");
    button_box->Add(force_ip_button_, 1);
    // add tooltip to force_ip_button_
    force_ip_button_->SetToolTip("Set temporary IP and Open WebGUI of selected device");
    force_ip_button_->GetSize(&w, &h);
    button_box->AddSpacer(10);

    force_perm_ip_button_ = new wxButton(panel, ID_ForcePermIpButton,
                                    "Set permanent IP");
    button_box->Add(force_perm_ip_button_, 1);
    button_box->AddSpacer(10);


    // reconnect_button_ = new wxButton(panel, ID_ReconnectButton,
    //                                 "Reconnect device");
    // button_box->Add(reconnect_button_, 1);

    button_box->Add(-1, 0, wxEXPAND);
    // button_box->AddSpacer(10);

    auto *help_button = new wxContextHelpButton(panel, ID_Help_Discovery,
                                                wxDefaultPosition, wxSize(h,h));
    button_box->Add(help_button, 0);

    vbox->Add(button_box, 0, wxTOP | wxLEFT | wxRIGHT, 10);
  }

  panel->SetSizer(vbox);
  Centre();

  help_ctrl_ = new wxHtmlHelpController(wxHF_DEFAULT_STYLE, panel);
  help_ctrl_->AddBook("memory:help.hhp");

  Connect(ID_DiscoverButton,
          wxEVT_COMMAND_BUTTON_CLICKED,
          wxCommandEventHandler(DiscoverFrame::onDiscoverButton));
  Connect(wxID_ANY,
          wxEVT_COMMAND_DISCOVERY_COMPLETED,
          wxThreadEventHandler(DiscoverFrame::onDiscoveryCompleted));
  Connect(wxID_ANY,
          wxEVT_COMMAND_DISCOVERY_ERROR,
          wxThreadEventHandler(DiscoverFrame::onDiscoveryError));
  Connect(ID_ResetButton,
          wxEVT_COMMAND_BUTTON_CLICKED,
          wxCommandEventHandler(DiscoverFrame::onResetButton));
  Connect(ID_ForceIpButton,
          wxEVT_COMMAND_BUTTON_CLICKED,
          wxCommandEventHandler(DiscoverFrame::onForceIpButton));
  Connect(ID_ForcePermIpButton,
          wxEVT_COMMAND_BUTTON_CLICKED,
          wxCommandEventHandler(DiscoverFrame::onForcePermIpButton));
  Connect(ID_ReconnectButton,
          wxEVT_COMMAND_BUTTON_CLICKED,
          wxCommandEventHandler(DiscoverFrame::onReconnectButton));
  Connect(ID_Help_Discovery,
          wxEVT_COMMAND_BUTTON_CLICKED,
          wxCommandEventHandler(DiscoverFrame::onHelpDiscovery));
  // Connect(ID_DataViewListCtrl,
  //         wxEVT_DATAVIEW_ITEM_ACTIVATED,
  //         wxDataViewEventHandler(DiscoverFrame::onDeviceDoubleClick));
  Connect(ID_DataViewListCtrl,
          wxEVT_DATAVIEW_SELECTION_CHANGED,
          wxDataViewEventHandler(DiscoverFrame::onDeviceSelection));
  Connect(ID_DataViewListCtrl,
          wxEVT_DATAVIEW_ITEM_CONTEXT_MENU,
          wxDataViewEventHandler(DiscoverFrame::onDataViewContextMenu));
  Connect(ID_OpenWebGUI,
          wxEVT_MENU,
          wxMenuEventHandler(DiscoverFrame::onOpenWebGUI));
  Connect(ID_CopyName,
          wxEVT_MENU,
          wxMenuEventHandler(DiscoverFrame::onCopy));
  Connect(ID_CopyManufacturer,
          wxEVT_MENU,
          wxMenuEventHandler(DiscoverFrame::onCopy));
  Connect(ID_CopyModel,
          wxEVT_MENU,
          wxMenuEventHandler(DiscoverFrame::onCopy));
  Connect(ID_CopySerial,
          wxEVT_MENU,
          wxMenuEventHandler(DiscoverFrame::onCopy));
  Connect(ID_CopyIP,
          wxEVT_MENU,
          wxMenuEventHandler(DiscoverFrame::onCopy));
  Connect(ID_CopyMac,
          wxEVT_MENU,
          wxMenuEventHandler(DiscoverFrame::onCopy));
  Connect(ID_ResetButton,
          wxEVT_MENU,
          wxMenuEventHandler(DiscoverFrame::onResetContextMenu));
  Connect(ID_ForceIpButton,
          wxEVT_MENU,
          wxMenuEventHandler(DiscoverFrame::onForceIpContextMenu));
  Connect(ID_ForcePermIpButton,
          wxEVT_MENU,
          wxMenuEventHandler(DiscoverFrame::onForcePermIpContextMenu));
  Connect(ID_ReconnectButton,
          wxEVT_MENU,
          wxMenuEventHandler(DiscoverFrame::onReconnectContextMenu));
  Connect(wxID_EXIT,
          wxEVT_MENU,
          wxCommandEventHandler(DiscoverFrame::onExit));
  Connect(wxID_ABOUT,
          wxEVT_MENU,
          wxCommandEventHandler(DiscoverFrame::onAbout));
  Connect(wxID_HELP,
          wxEVT_MENU,
          wxCommandEventHandler(DiscoverFrame::onHelp));
  Connect(ID_OnlyRcCheckbox,
          wxEVT_CHECKBOX,
          wxCommandEventHandler(DiscoverFrame::onOnlyRcCheckbox));
  Connect(ID_FilterTextInput,
          wxEVT_TEXT,
          wxCommandEventHandler(DiscoverFrame::onFilterTextChange));

  reset_dialog_ = new ResetDialog(help_ctrl_, panel, wxID_ANY);
  force_ip_dialog_ = new ForceIpDialog(help_ctrl_, panel, wxID_ANY);
  force_perm_ip_dialog_ = new ForcePermIpDialog(help_ctrl_, panel, wxID_ANY);
  reconnect_dialog_ = new ReconnectDialog(help_ctrl_, panel, wxID_ANY);
  about_dialog_ = new AboutDialog(panel, wxID_ANY);

  wxPersistentRegisterAndRestore(this, "discover_frame");

  // start discovery on startup
  wxCommandEvent evt;
  onDiscoverButton(evt);
}

void DiscoverFrame::setBusy()
{
  discover_button_->Disable();
  // reset_button_->Disable();
  
  force_ip_button_->SetBackgroundColour(wxColour(240, 240, 240));
  force_ip_button_->SetForegroundColour(wxColour(128, 128, 128));
  force_ip_button_->Disable();
  force_perm_ip_button_->Disable();
  // reconnect_button_->Disable();
  spinner_ctrl_->Play();
}

void DiscoverFrame::clearBusy()
{
  discover_button_->Enable();
  // reset_button_->Enable();
  //force_ip_button_->Enable();  
  force_ip_button_->SetBackgroundColour(wxColour(240, 240, 240)); 
  force_ip_button_->SetForegroundColour(wxColour(128, 128, 128)); 
  force_ip_button_->Disable();
  force_perm_ip_button_->Enable();
  // reconnect_button_->Enable();
  spinner_ctrl_->Stop();

  // on Windows, wxAnimationCtrl is sometimes not stopping even if
  // Stop was called. Calling it multiple times reduces the chance
  // of this happening.
  spinner_ctrl_->Stop();
  spinner_ctrl_->Stop();
}

void DiscoverFrame::onDiscoverButton(wxCommandEvent &)
{
  setBusy();

  auto *thread = new DiscoverThread(this);
  if (thread->Run() != wxTHREAD_NO_ERROR)
  {
    std::cerr << "Could not spawn thread" << std::endl;
    delete thread;
    thread = nullptr;

    clearBusy();
  }
}

void DiscoverFrame::onDiscoveryCompleted(wxThreadEvent &event)
{
  updateDeviceList(event.GetPayload<std::vector<wxVector<wxVariant>>>());

  clearBusy();
}

void DiscoverFrame::updateDeviceList(const std::vector<wxVector<wxVariant>> &d)
{
  device_list_->DeleteAllItems();

  std::vector<bool> show_in_reset_dialog;

  last_data_ = d;
  for(const auto& d : last_data_)
  {
    const bool matches_filter = [&] {
      if (!filter_text_.empty())
      {
        const auto& filter_text = filter_text_;
        const bool none = std::none_of(d.begin(), d.end(), [&filter_text](const wxVariant& v) {
          const auto s = v.GetString().ToStdString();
          return wildcardMatch(s.begin(), s.end(), filter_text.begin(), filter_text.end());
        });
        if (none)
        {
          return false;
        }
      }
      return true;
    }();

    if (matches_filter && (!only_rc_sensors_ || isMadeByRc(d)))
    {
      device_list_->AppendItem(d);
      show_in_reset_dialog.push_back(isRcVisard(d));
    }
  }

  reset_dialog_->setDiscoveredSensors(device_list_->GetStore(), show_in_reset_dialog);
  force_ip_dialog_->setDiscoveredSensors(device_list_->GetStore());
  force_perm_ip_dialog_->setDiscoveredSensors(device_list_->GetStore());
  reconnect_dialog_->setDiscoveredSensors(device_list_->GetStore());
}

void DiscoverFrame::onDiscoveryError(wxThreadEvent &event)
{
  std::ostringstream oss;
  oss << "An error occurred during discovery: " << event.GetString();
  wxMessageBox(oss.str(), "Error", wxOK | wxICON_ERROR);

  clearBusy();
}

void DiscoverFrame::onResetButton(wxCommandEvent &)
{
  openResetDialog(device_list_->GetSelectedRow());
}

void DiscoverFrame::onForceIpButton(wxCommandEvent &)
{
  // openForceIpDialog(device_list_->GetSelectedRow());
  openWebGUI(device_list_->GetSelectedRow());
}

void DiscoverFrame::onForcePermIpButton(wxCommandEvent &)
{
  openForcePermIpDialog(device_list_->GetSelectedRow());
}

void DiscoverFrame::onReconnectButton(wxCommandEvent &)
{
  openReconnectDialog(device_list_->GetSelectedRow());
}

void DiscoverFrame::onHelpDiscovery(wxCommandEvent&)
{
  help_ctrl_->Display("help.htm#discovery");
}

void DiscoverFrame::onDeviceDoubleClick(wxDataViewEvent &event)
{
  const auto item = event.GetItem();
  const auto row = device_list_->ItemToRow(item);

  if (row == wxNOT_FOUND)
  {
    return;
  }

  openWebGUI(row);
}

void DiscoverFrame::onDeviceSelection(wxDataViewEvent &event)
{
  const auto item = event.GetItem();
  const auto row = device_list_->ItemToRow(item);

  if (row == wxNOT_FOUND)
  {
    return;
  }
  // enable set temporary ip button 
  force_ip_button_->Enable(isMadeByRc(*device_list_, row));
  force_ip_button_->SetBackgroundColour(wxColour(0, 61, 106));
  force_ip_button_->SetForegroundColour(wxColour(255, 255, 255));
  // set font color to white
  // force_ip_button_->SetForegroundColour(wxColour(255, 255, 255));

  // reset_button_->Enable(isRcVisard(*device_list_, row));
}

void DiscoverFrame::onDataViewContextMenu(wxDataViewEvent &event)
{
  menu_event_item_.reset(new std::pair<int, int>(
                           device_list_->ItemToRow(event.GetItem()),
                           event.GetColumn()));

  if (menu_event_item_->first < 0)
  {
    return;
  }

  wxMenu menu;
  menu.Append(ID_CopyName, "Copy name");
  menu.Append(ID_CopyManufacturer, "Copy manufacturer");
  menu.Append(ID_CopyModel, "Copy model");
  menu.Append(ID_CopySerial, "Copy serial number");
  menu.Append(ID_CopyIP, "Copy IP address");
  menu.Append(ID_CopyMac, "Copy MAC address");

  if (isMadeByRc(*device_list_, static_cast<unsigned int>(menu_event_item_->first)))
  {
    menu.AppendSeparator();
    menu.Append(ID_OpenWebGUI, "Open &WebGUI");
    menu.AppendSeparator();
    if (isRcVisard(*device_list_, static_cast<unsigned int>(menu_event_item_->first)))
    {
      menu.Append(ID_ResetButton, "Reset");
    }
  }

  // menu.Append(ID_ForceIpButton, "Open");
  // menu.Append(ID_ReconnectButton, "Reconnect");
  menu.Append(ID_ForcePermIpButton, "Set permanent IP");

  PopupMenu(&menu);
}

void DiscoverFrame::onCopy(wxMenuEvent &evt)
{
  int column;
  switch (evt.GetId())
  {
    case ID_CopyName:
      column = NAME;
      break;

    case ID_CopyManufacturer:
      column = MANUFACTURER;
      break;

    case ID_CopyModel:
      column = MODEL;
      break;

    case ID_CopySerial:
      column = SERIAL;
      break;

    case ID_CopyIP:
      column = IP;
      break;

    case ID_CopyMac:
      column = MAC;
      break;

    default:
      return;
  }

  const auto row = static_cast<unsigned int>(menu_event_item_->first);
  const auto cell = device_list_->GetTextValue(row, column);

  if (wxTheClipboard->Open())
  {
    wxTheClipboard->SetData(new wxTextDataObject(cell));
    wxTheClipboard->Close();
  }
}

void DiscoverFrame::onOpenWebGUI(wxMenuEvent &)
{
  if (!menu_event_item_ ||
      menu_event_item_->first < 0)
  {
    return;
  }

  openWebGUI(menu_event_item_->first);
}

void DiscoverFrame::onResetContextMenu(wxMenuEvent &)
{
  if (!menu_event_item_ ||
      menu_event_item_->first < 0)
  {
    return;
  }

  openResetDialog(menu_event_item_->first);
}

void DiscoverFrame::onForceIpContextMenu(wxMenuEvent &)
{
  if (!menu_event_item_ ||
      menu_event_item_->first < 0)
  {
    return;
  }

  // openForceIpDialog(menu_event_item_->first);
  openWebGUI(menu_event_item_->first);
}

void DiscoverFrame::onForcePermIpContextMenu(wxMenuEvent &)
{
  if (!menu_event_item_ ||
      menu_event_item_->first < 0)
  {
    return;
  }

  openForcePermIpDialog(menu_event_item_->first);
}

void DiscoverFrame::onReconnectContextMenu(wxMenuEvent &)
{
  if (!menu_event_item_ ||
      menu_event_item_->first < 0)
  {
    return;
  }

  openReconnectDialog(menu_event_item_->first);
}

void DiscoverFrame::onExit(wxCommandEvent &)
{
  Close(true);
}

void DiscoverFrame::onHelp(wxCommandEvent&)
{
  help_ctrl_->Display("help.htm");
}

void DiscoverFrame::onOnlyRcCheckbox(wxCommandEvent &evt)
{
  only_rc_sensors_ = evt.IsChecked();
  updateDeviceList(last_data_);
}

void DiscoverFrame::onFilterTextChange(wxCommandEvent &evt)
{
  filter_text_ = evt.GetString();
  std::transform(filter_text_.begin(), filter_text_.end(), filter_text_.begin(), ::tolower);
  if (!filter_text_.empty())
  {
    if (filter_text_.front() != '*')
    { filter_text_ = '*' + filter_text_; }
    if (filter_text_.back() != '*')
    { filter_text_ = filter_text_ + '*'; }
  }
  updateDeviceList(last_data_);
}

void DiscoverFrame::onAbout(wxCommandEvent &)
{
  about_dialog_->ShowModal();
}

void DiscoverFrame::openResetDialog(const int row)
{
  if (row != wxNOT_FOUND)
  {
    reset_dialog_->setActiveSensor(static_cast<unsigned int>(row));
  }

  reset_dialog_->Show();
}

void DiscoverFrame::openForceIpDialog(const int row)
{
  if (row != wxNOT_FOUND)
  {
    force_ip_dialog_->setActiveSensor(static_cast<unsigned int>(row));
  }

  force_ip_dialog_->Show();
}

void DiscoverFrame::openForcePermIpDialog(const int row)
{
  if (row != wxNOT_FOUND)
  {
    force_perm_ip_dialog_->setActiveSensor(static_cast<unsigned int>(row));
  }

  force_perm_ip_dialog_->Show();
}


void DiscoverFrame::openReconnectDialog(const int row)
{
  if (row != wxNOT_FOUND)
  {
    reconnect_dialog_->setActiveSensor(static_cast<unsigned int>(row));
  }

  reconnect_dialog_->Show();
}

void DiscoverFrame::openWebGUI(int row)
{
  if (isMadeByRc(*device_list_, row))
  {
    spinner_ctrl_->Play();
    const auto ip_wxstring = device_list_->GetTextValue(
        static_cast<unsigned int>(row), IP);  
    const auto reachable_value = device_list_->GetTextValue(
        static_cast<unsigned int>(row), REACHABLE);  
    // initialize the force ip dialog with the selected sensor
    force_ip_dialog_->setActiveSensor(static_cast<unsigned int>(row));
    // get the sender IP address
    std::array<uint8_t, 4> ip_sender_ = force_ip_dialog_->getSenderIp();  
    // Use a 32-bit integer to store the sender IP address
    if (ip_sender_[3] == 255 || ip_sender_[3] == 0 || ip_sender_[3] == 254)     
    {
      ip_sender_[3] = 1;
    }
    std::uint32_t ip_sender_uint = 0;
    ip_sender_uint |= static_cast<std::uint32_t>(ip_sender_[0]) << 24;
    ip_sender_uint |= static_cast<std::uint32_t>(ip_sender_[1]) << 16; 
    ip_sender_uint |= static_cast<std::uint32_t>(ip_sender_[2]) << 8;
    ip_sender_uint |= static_cast<std::uint32_t>(ip_sender_[3]);
    ip_sender_uint += 1;    
    // covert to string
    std::string ip_sender_string = std::to_string((ip_sender_uint >> 24) & 0xFF) + "." + std::to_string((ip_sender_uint >> 16) & 0xFF) + "." + std::to_string((ip_sender_uint >> 8) & 0xFF) + "." + std::to_string(ip_sender_uint & 0xFF);

    // substring before last '.' is equal to ip_wxstring then no need to set IP
    std::string ip_wxstring_str = ip_wxstring.ToStdString();
    std::string ip_sender_substr = ip_sender_string.substr(0, ip_sender_string.find_last_of('.'));

    // // ** TO DO ** -- TESTING in WINDOWS
    // // execute a ping command to check if the IP address is reachable
    // // if reachable then open webgui
    // // if not reachable then show a message dialog
    
    //std::string command = "ping -c 1 " + ip_wxstring_str;
    // wxMessageBox("Checking reachability of device to open application in browser. Please wait..." ,"Busy", wxOK | wxICON_INFORMATION);
    
    //int result = system(command.c_str());  
    // result = 0 if reachable and 1 if not reachable
     
    if (reachable_value == wxT("\u2713")) {
        // The ping was successful, open the webgui
        // ...
        
        wxLaunchDefaultBrowser("http://" + ip_wxstring + "/");
        spinner_ctrl_->Stop();

        // on Windows, wxAnimationCtrl is sometimes not stopping even if
        // Stop was called. Calling it multiple times reduces the chance
        // of this happening.
        spinner_ctrl_->Stop();
        spinner_ctrl_->Stop();
    }
    // else {
    //     // The ping was not successful, show a message dialog
    //     wxMessageBox("The IP address " + ip_wxstring_str + " is not reachable.", "Unable to open ", wxOK | wxICON_ERROR);
    // }
    
    // // if ip_sender_string is equal to  ip_wxstring then no need to set IP    
    // if(ip_sender_string == ip_wxstring.ToStdString())
    // {
    //   wxLaunchDefaultBrowser("http://" + ip_wxstring + "/");
    // }
    // else if(ip_sender_substr == ip_wxstring_str.substr(0, ip_wxstring_str.find_last_of('.')))
    // {
    //   wxLaunchDefaultBrowser("http://" + ip_wxstring + "/");
    // }
    else
    {
      // ask if user wants to set IP address to ip_sender
      int answer = wxMessageBox("IP address "+ ip_wxstring_str +" is not reachable .Do you want to set temporary IP address  ?", "Set IP address", wxYES_NO | wxICON_QUESTION);
      if(answer == wxYES)
      {
        spinner_ctrl_->Play();
        // set IP address to ip_sender
        // Create a temporary wxCommandEvent and pass it to the function
        wxCommandEvent evt;
        force_ip_dialog_->onForceIpButton(evt);        
        // Show a message dialog with a loading icon
        // wxMessageBox("Setting temporary IP address. Please wait...","Busy", wxOK | wxICON_INFORMATION);
        wxMessageBox("The temporary IP address is being configured. Please wait and press OK to continue...","Configuration in Progress", wxOK | wxICON_INFORMATION);

        setBusy();
        // Wait for 10 seconds
        wxMilliSleep(10000);
        clearBusy(); 
        // run discovery again
        wxCommandEvent evt1;
        onDiscoverButton(evt1);
        // open webgui
        std::array<uint8_t, 4> ip_sender = force_ip_dialog_->getSenderIp();  
        // Use a 32-bit integer to store the sender IP address
        if (ip_sender[3] == 255 || ip_sender[3] == 0 || ip_sender[3] == 254)
        {
          ip_sender[3] = 1;
        }
        std::uint32_t ip_sender_uint = 0;
        ip_sender_uint |= static_cast<std::uint32_t>(ip_sender[0]) << 24;
        ip_sender_uint |= static_cast<std::uint32_t>(ip_sender[1]) << 16; 
        ip_sender_uint |= static_cast<std::uint32_t>(ip_sender[2]) << 8;
        ip_sender_uint |= static_cast<std::uint32_t>(ip_sender[3]);
        ip_sender_uint += 1;    
        // covert to string
        std::string ip_sender_string = std::to_string((ip_sender_uint >> 24) & 0xFF) + "." + std::to_string((ip_sender_uint >> 16) & 0xFF) + "." + std::to_string((ip_sender_uint >> 8) & 0xFF) + "." + std::to_string(ip_sender_uint & 0xFF);
        
        wxLaunchDefaultBrowser("http://" + ip_sender_string + "/");
        spinner_ctrl_->Stop();

        // on Windows, wxAnimationCtrl is sometimes not stopping even if
        // Stop was called. Calling it multiple times reduces the chance
        // of this happening.
        spinner_ctrl_->Stop();
        spinner_ctrl_->Stop();
      }
      else
      {
        
        // Warn user if IP is in not same subnet then application will not open in browser
        wxMessageBox("IP address is not in same subnet. Application will not open in browser.", "Warning", wxOK | wxICON_WARNING);
        spinner_ctrl_->Stop();

        // on Windows, wxAnimationCtrl is sometimes not stopping even if
        // Stop was called. Calling it multiple times reduces the chance
        // of this happening.
        spinner_ctrl_->Stop();
        spinner_ctrl_->Stop();
      }
    }    
  }
}

BEGIN_EVENT_TABLE(DiscoverFrame, wxFrame)
END_EVENT_TABLE()
