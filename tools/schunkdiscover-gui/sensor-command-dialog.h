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
#ifndef SENSORCOMMANDDIALOG_H
#define SENSORCOMMANDDIALOG_H

#include <wx/dialog.h>

#include <array>
#include <unordered_map>
#include <vector>
#include <stdint.h>

class wxChoice;
class wxTextCtrl;
class wxDataViewListModel;
class wxHtmlHelpController;
class wxBoxSizer;
class wxPanel;
class wxFlexGridSizer;

/**
 * @brief Base class for dialogs for sending commands to a camera.
 *
 * It will add a drop down list containing the discovered cameras
 * and text boxes for the MAC address.
 */
class SensorCommandDialog : public wxDialog
{
  public:
    SensorCommandDialog() = default;

    SensorCommandDialog(wxHtmlHelpController *help_ctrl,
                        wxWindow *parent, wxWindowID id,
                        std::string title,
                        int additional_grid_rows,
                        const wxPoint &pos = wxDefaultPosition,
                        long style = wxDEFAULT_DIALOG_STYLE,
                        const wxString &name = wxDialogNameStr);

    virtual ~SensorCommandDialog() = default;
    std::array<uint8_t, 4> getSenderIp() const;
    std::string getSenderIpString() const;
    std::array<uint8_t, 4> getRobotIPNetwork() const;
    std::array<uint8_t, 4> getCameraIPNetwork() const;
  public:
    /**
     * @brief Set list of discovered devices to provide a drop down menu
     * to the user.
     * @param sensor_list list of devices
     */
    void setDiscoveredSensors(const wxDataViewListModel *sensor_list,
                              const std::vector<bool>& show = {});

    /**
     * @brief Select a specific device of the list set by
     * setDiscoveredSensors.
     * @param row row of device list
     */
    void setActiveSensor(const unsigned int row);

  protected:
    wxBoxSizer *getVerticalBox();
    wxPanel *getPanel();
    wxFlexGridSizer *getGrid();
    std::array<uint8_t, 6> getMac() const;
    // std::array<uint8_t, 4> getSenderIp() const;
    std::string getMacString() const;
    // std::string getSenderIpString() const;
    void displayHelp(const std::string &section);

    /**
     * @brief Reset and clear all fields.
     */
    virtual void clear();

  private:
    /**
     * @brief Event handler for selection of an device from drop down menu.
     * @param event event
     */
    void onSensorSelected(wxCommandEvent &event);

    /**
     * @brief Fill MAC address according to selected device.
     */
    void fillMac();

    /**
     * @brief Fill SenderIp address according to selected device.
     */
    void fillSenderIp();

    /**
     * @brief Fill robot network address according to selected device.
     */
    void fillRobot();

    /**
     * @brief Fill camera network address according to selected device.
     */
    void fillCamera();

    /**
     * @brief Clear MAC address.
     */
    void clearMac();
    /**
     * @brief Clear SenderIp address.
     */
    void clearSenderIp();

    
    wxDECLARE_EVENT_TABLE();

  private:
    wxPanel *panel_;
    wxBoxSizer *vbox_;
    wxFlexGridSizer *grid_;
    wxChoice *sensors_;
    std::array<wxTextCtrl *, 6> mac_;
    std::array<wxTextCtrl *, 4> senderip_;
    std::array<wxTextCtrl *, 4> robotipnetwork_;
    std::array<wxTextCtrl *, 4> cameraipnetwork_;
    std::unordered_map<unsigned int, unsigned int> row_map_;
    std::unordered_map<unsigned int, unsigned int> row_map_inv_;

    const wxDataViewListModel *sensor_list_;

    wxHtmlHelpController *help_ctrl_;
};

#endif // SENSORCOMMANDDIALOG_H
