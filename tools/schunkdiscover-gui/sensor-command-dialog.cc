/*
* Roboception GmbH
* Munich, Germany
* www.roboception.com
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
*/
#include "sensor-command-dialog.h"

#include "event-ids.h"
#include "resources.h"
#include "force-ip-dialog.h"
#include "schunkdiscover/utils.h"
#include "discover-frame.h"

#include <sstream>
#include <stdint.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/combobox.h>
#include <wx/dataview.h>
#include <wx/html/helpctrl.h>
#include <wx/cshelp.h>
#include <cstdint>
#include <wx/variant.h>


SensorCommandDialog::SensorCommandDialog(wxHtmlHelpController *help_ctrl,
                                         wxWindow *parent, wxWindowID id,
                                         std::string title,
                                         const int additional_grid_rows,
                                         const wxPoint &pos,
                                         long style,
                                         const wxString &name) :
  wxDialog(parent, id, std::move(title), pos, wxSize(-1,-1), style, name),
  sensors_(nullptr),
  mac_{{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
  senderip_{{nullptr, nullptr, nullptr, nullptr}},
  sensor_list_(nullptr),
  help_ctrl_(help_ctrl)
{
  panel_ = new wxPanel(this, -1);
  vbox_ = new wxBoxSizer(wxVERTICAL);

  grid_ = new wxFlexGridSizer(3 + additional_grid_rows, 2, 10, 25);
  grid_->AddGrowableCol(1, 1);  // Allow the second column to grow

  auto *sensors_text = new wxStaticText(panel_, wxID_ANY, "Device");
  grid_->Add(sensors_text);

  auto *sensors_box = new wxBoxSizer(wxHORIZONTAL);
  sensors_ = new wxChoice(panel_, ID_Sensor_Combobox);
  sensors_box->Add(sensors_, 1);
  grid_->Add(sensors_box, 1, wxEXPAND);

  auto *mac_text = new wxStaticText(panel_, wxID_ANY, "MAC address");
  grid_->Add(mac_text);

  auto *mac_box = new wxBoxSizer(wxHORIZONTAL);
  int i = 0;
  for (auto& m : mac_)
  {
    if (i > 0)
    {
      mac_box->Add(new wxStaticText(panel_, ID_MAC_Textbox, ":"));
    }
    m = new wxTextCtrl(panel_, wxID_ANY, wxEmptyString,
                       wxDefaultPosition, wxSize(35, -1));
    m->Disable();
    mac_box->Add(m, 1);
    ++i;
  }
  grid_->Add(mac_box, 1, wxEXPAND);

  // Sender IP
  auto *senderip_text = new wxStaticText(panel_, wxID_ANY, "Host PC IP address");
  grid_->Add(senderip_text);
  
  auto *senderip_box = new wxBoxSizer(wxHORIZONTAL);
  i = 0;

  for (auto& s : senderip_)
  {
    if (i > 0)
    {
      senderip_box->Add(new wxStaticText(panel_, ID_SenderIP_Textbox, "."));
    }
    s = new wxTextCtrl(panel_, wxID_ANY, wxEmptyString,
                       wxDefaultPosition, wxSize(35, -1));
    s->Disable();
    senderip_box->Add(s, 1);
    ++i;
  }
  grid_->Add(senderip_box, 1, wxEXPAND);
  vbox_->Add(grid_, 0, wxALL | wxEXPAND, 15);
  panel_->SetSizer(vbox_);


  Connect(ID_Sensor_Combobox,
          wxEVT_CHOICE,
          wxCommandEventHandler(SensorCommandDialog::onSensorSelected));
}

void SensorCommandDialog::setDiscoveredSensors(
    const wxDataViewListModel *sensor_list,
    const std::vector<bool>& show)
{
  sensor_list_ = sensor_list;

  if (sensor_list != nullptr)
  {
    sensors_->Clear();

        

    const auto rows = sensor_list->GetCount();
    unsigned int sensors_row = 0;
    for (typename std::decay<decltype(rows)>::type i = 0; i < rows; ++i)
    {
      if (show.empty() || show[i])
      {
        wxVariant hostname{};
        wxVariant mac{};
        wxVariant ifaceVariant{};
        wxVariant sender{};
        sensor_list->GetValueByRow(hostname, i, DiscoverFrame::NAME);
        sensor_list->GetValueByRow(mac, i, DiscoverFrame::MAC);
        sensor_list->GetValueByRow(ifaceVariant, i, DiscoverFrame::IFACE);
        sensor_list ->GetValueByRow(sender, i, DiscoverFrame::SENDERIP);
        const auto s = wxString::Format("%s(PC INTERFACE: %s)", hostname.GetString(),ifaceVariant.GetString());
        sensors_->Append("< Select a device for setting IP >");
        sensors_->Append(s);
        row_map_.emplace(i, sensors_row + 1);
        row_map_inv_.emplace(sensors_row + 1, i);
        ++sensors_row;
      }
    }
  }

  clear();
}

void SensorCommandDialog::setActiveSensor(const unsigned int row)
{
  clear();

  const auto found = row_map_.find(static_cast<int>(row));
  if (found != row_map_.cend())
  {
    sensors_->Select(found->second);
    fillMac();
    fillSenderIp();
  }
  else
  {
    sensors_->Select(0);
  }
}

wxBoxSizer *SensorCommandDialog::getVerticalBox()
{
  return vbox_;
}

wxPanel *SensorCommandDialog::getPanel()
{
  return panel_;
}

wxFlexGridSizer *SensorCommandDialog::getGrid()
{
  return grid_;
}



std::array<uint8_t, 6> SensorCommandDialog::getMac() const
{
  std::array<uint8_t, 6> mac;
  for (uint8_t i = 0; i < 6; ++i)
  {
    const auto s = mac_[i]->GetValue().ToStdString();

    try
    {
      const auto v = std::stoul(s, nullptr, 16);
      if (v > 0xff)
      {
        throw std::invalid_argument("");
      }
      mac[i] = static_cast<uint8_t>(v);
    }
    catch(const std::invalid_argument&)
    {
      throw std::runtime_error(
            std::string("Each MAC address segment must contain ") +
            "a hex value ranging from 0x00 to 0xff.");
    }
  }
  return mac;
}

std::string SensorCommandDialog::getMacString() const
{
  const auto mac = getMac();

  std::ostringstream mac_string;
  bool first = true;
  for (const auto m : mac)
  {
    if (!first)
    {
      mac_string << ":";
    }
    mac_string << std::hex << std::setfill('0') << std::setw(2);
    mac_string << static_cast<unsigned int>(m);
    first = false;
  }
  return mac_string.str();
}

std::array<uint8_t, 4> SensorCommandDialog::getSenderIp() const
{
  std::array<uint8_t, 4> senderip;
  for (uint8_t i = 0; i < 4; ++i)
  {
    const auto s = senderip_[i]->GetValue().ToStdString();

    try
    {
      const auto v = std::stoul(s, nullptr, 10);
      if (v > 255)
      {
        throw std::invalid_argument("");
      }
      senderip[i] = static_cast<uint8_t>(v);
    }
    catch(const std::invalid_argument&)
    {
      throw std::runtime_error(
            std::string("Each sender ip address segment must contain ") +
            "a decimal value ranging from 0 to 255.");
    }
  }
  return senderip;
}

std::string SensorCommandDialog::getSenderIpString() const
{
  const auto senderip = getSenderIp();

  std::ostringstream senderip_string;
  bool first = true;
  for (const auto s : senderip)
  {
    if (!first)
    {
      senderip_string << ".";
    }
    senderip_string << std::dec << static_cast<unsigned int>(s);
    first = false;
  }
  return senderip_string.str();
}

void SensorCommandDialog::displayHelp(const std::string &section)
{
  const std::string url = std::string("help.htm#") + section;
  help_ctrl_->Display(url);

  // need second call otherwise it does not jump to the section if the
  // help is displayed the first time
  help_ctrl_->Display(url);
}

void SensorCommandDialog::clear()
{
  clearMac();
  clearSenderIp();
}

void SensorCommandDialog::onSensorSelected(wxCommandEvent &)
{
  if (sensors_->GetSelection() != wxNOT_FOUND)
  {
    if (sensors_->GetSelection() == 0)
    {
      clearMac();
      clearSenderIp();
    }
    else
    {
      fillMac();
      fillSenderIp();
    }
  }
}

void SensorCommandDialog::fillMac()
{
  const int row = sensors_->GetSelection();

  if (row == wxNOT_FOUND)
  {
    return;
  }

  wxVariant mac_string{};
  sensor_list_->GetValueByRow(mac_string,
                              row_map_inv_.at(static_cast<unsigned int>(row)),
                              DiscoverFrame::MAC);

  const auto mac = split<6>(mac_string.GetString().ToStdString(), ':');

  for (uint8_t i = 0; i < 6; ++i)
  {
    mac_[i]->ChangeValue(mac[i]);
    mac_[i]->SetEditable(false);
  }
   
}

void SensorCommandDialog::fillSenderIp()
{
  const int row = sensors_->GetSelection();

  if (row == wxNOT_FOUND)
  {
    return;
  }

  wxVariant senderip_string{};
  sensor_list_->GetValueByRow(senderip_string,
                              row_map_inv_.at(static_cast<unsigned int>(row)),
                              DiscoverFrame::SENDERIP);

  const auto senderip = split<4>(senderip_string.GetString().ToStdString(), '.');

  for (uint8_t i = 0; i < 4; ++i)
  {
    if (i == 3)
    {
    // Parse each segment of the IP address string, add 1, and set the value
    int ip_segment = std::stoi(senderip[i]);
    if (ip_segment > 255) ip_segment = 1; // Ensure it doesn't exceed 255
    senderip_[i]->ChangeValue(std::to_string(ip_segment));
    senderip_[i]->SetEditable(false);
    
    }
    else
    {

    senderip_[i]->ChangeValue(senderip[i]);
    senderip_[i]->SetEditable(false);
    }
  }
}
void SensorCommandDialog::clearMac()
{
  sensors_->SetSelection(0);

  for (uint8_t i = 0; i < 6; ++i)
  {
    mac_[i]->Clear();
    mac_[i]->SetEditable(true);
  }
}

void SensorCommandDialog::clearSenderIp()
{
  sensors_->SetSelection(0);

  for (uint8_t i = 0; i < 4; ++i)
  {
    senderip_[i]->Clear();
    senderip_[i]->SetEditable(true);
  }
}

BEGIN_EVENT_TABLE(SensorCommandDialog, wxDialog)
END_EVENT_TABLE()
