/*
 * schunkdiscover - the network discovery tool for Schunk 2D Grasping Kit
 *
 * Copyright (c) 2017 Roboception GmbH
 * All rights reserved
 *
 * Author: Raphael Schaller
 * 
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

#include "schunkdiscover-gui/discover-frame.h"
#include "schunkdiscover-gui/resources.h"

#include <sstream>

#include "wx/app.h"
#include "wx/msgdlg.h"
#include "wx/html/htmlwin.h"
#include "wx/sizer.h"
#include "wx/button.h" // Include the header for wxButton
#include <fstream>
#include <sstream>
#include <string>
#include <iostream> // For std::cerr
#include <unistd.h> // For getcwd

// std::string readHtmlFile(const std::string &filePath)
// {
//     char cwd[1024];
//     if (getcwd(cwd, sizeof(cwd)) != nullptr)
//     {
//         std::cerr << "Current working directory: " << cwd << std::endl;
//     }
//     else
//     {
//         std::cerr << "Error: Unable to get current working directory" << std::endl;
//     }

//     std::cerr << "Attempting to open file: " << filePath << std::endl;

//     std::ifstream file(filePath);
//     if (!file.is_open())
//     {
//         std::cerr << "Error: Unable to open file: " << filePath << std::endl;
//         std::cerr << "Please check if the file exists and has the correct permissions." << std::endl;
//         return "Error: Unable to open file: " + filePath;
//     }

//     std::stringstream buffer;
//     buffer << file.rdbuf();
//     std::string content = buffer.str();
//     std::cerr << "File content: " << content << std::endl; // Debug output
//     return content;
// }

std::string readHtmlFileFromResources(const std::string &filePath)
{
    std::cerr << "Attempting to open resource: " << filePath << std::endl;
    wxFileSystem fs;
    wxFSFile *file = fs.OpenFile(filePath);
    if (!file)
    {
        std::cerr << "Error: Unable to open file: " << filePath << std::endl;
        return "Error: Unable to open file: " + filePath;
    }

    wxInputStream *stream = file->GetStream();
    std::stringstream buffer;
    char temp[1024];
    while (!stream->Eof())
    {
        stream->Read(temp, sizeof(temp));
        buffer.write(temp, stream->LastRead());
    }
    std::string content = buffer.str();
    std::cerr << "File content: " << content << std::endl; // Debug output
    return content;
}

class HtmlDialog : public wxDialog
{
public:
    HtmlDialog(wxWindow *parent, const wxString &title, const wxString &content)
        : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(600, 400))
    {
        wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
        wxHtmlWindow *htmlWindow = new wxHtmlWindow(this, wxID_ANY, wxDefaultPosition, wxSize(580, 360));
        
        // Set the HTML content directly
        htmlWindow->SetPage(content);
        sizer->Add(htmlWindow, 1, wxEXPAND | wxALL, 10);

        wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
        agreeButton = new wxButton(this, wxID_OK, "I agree");
        wxButton *declineButton = new wxButton(this, wxID_CANCEL, "Decline");
        // agreeButton->Disable(); // Initially disable the "I agree" button
        buttonSizer->Add(agreeButton, 0, wxALL, 5);
        buttonSizer->Add(declineButton, 0, wxALL, 5);

        sizer->Add(buttonSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 10);
        SetSizerAndFit(sizer);

        // Bind the scroll event to check if the user has scrolled to the bottom
        htmlWindow->Bind(wxEVT_SCROLLWIN_THUMBTRACK, &HtmlDialog::OnScroll, this);
        htmlWindow->Bind(wxEVT_SCROLLWIN_THUMBRELEASE, &HtmlDialog::OnScroll, this);
        htmlWindow->Bind(wxEVT_SCROLLWIN_LINEUP, &HtmlDialog::OnScroll, this);
        htmlWindow->Bind(wxEVT_SCROLLWIN_LINEDOWN, &HtmlDialog::OnScroll, this);
        htmlWindow->Bind(wxEVT_SCROLLWIN_PAGEUP, &HtmlDialog::OnScroll, this);
        htmlWindow->Bind(wxEVT_SCROLLWIN_PAGEDOWN, &HtmlDialog::OnScroll, this);
        htmlWindow->Bind(wxEVT_SCROLLWIN_TOP, &HtmlDialog::OnScroll, this);
        htmlWindow->Bind(wxEVT_SCROLLWIN_BOTTOM, &HtmlDialog::OnScroll, this);
    }

private:
    wxButton *agreeButton;

    void OnScroll(wxScrollWinEvent &event)
    {
        wxHtmlWindow *htmlWindow = dynamic_cast<wxHtmlWindow*>(event.GetEventObject());
        if (htmlWindow)
        {
            
            int scrollPos = htmlWindow->GetScrollPos(wxVERTICAL);
            int clientSize = htmlWindow->GetClientSize().GetHeight();
            int virtualSize = htmlWindow->GetVirtualSize().GetHeight();

            if (scrollPos + clientSize >= virtualSize)
            {
                agreeButton->Enable(); // Enable the "I agree" button if scrolled to the bottom
            }
        }
        event.Skip();
    }
};

class SchunkDiscoverApp : public wxApp
{
  public:
    SchunkDiscoverApp() :
      frame_(nullptr)
    { }

    virtual ~SchunkDiscoverApp() = default;

    virtual bool OnInit() override
    {
      if (!wxApp::OnInit())
      {
        return false;
      }

      SetAppName("schunkdiscover");
      SetVendorName("ROBOCEPTION");

      // // Read the content of the HTML file
      // std::string htmlContent = readHtmlFile("1_Eula.html");
      // Read the content of the HTML file from resources
      registerResources();
      std::string htmlContent = readHtmlFileFromResources("memory:eula.htm");

      // Display the welcome message dialog with "I agree" and "Decline" buttons
      HtmlDialog welcomeDialog(nullptr, "End User Licence Agreement", htmlContent);
      if (welcomeDialog.ShowModal() == wxID_CANCEL)
      {
          return false; // Exit the application if the user declines
      }

#ifdef WIN32
      ::WSADATA wsaData;
      int result;
      if ((result = ::WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
      {
        std::ostringstream oss;
        oss << "WSAStartup failed: " << result;
        wxMessageBox(oss.str(), "Error", wxOK | wxICON_ERROR);
      }
#endif

      // registerResources();

      frame_ = new DiscoverFrame("SCHUNK 2D Grasping Discovery", wxPoint(50,50));
      frame_->Show(true);
      return true;
    }

    virtual int OnExit() override
    {
#ifdef WIN32
      ::WSACleanup();
#endif

      return wxApp::OnExit();
    }

    virtual bool OnExceptionInMainLoop() override
    {
      try
      {
        throw;
      }
      catch (const std::exception &ex)
      {
        std::string error_msg = "Caught exception of type ";
        error_msg += typeid(ex).name();
        error_msg += ": ";
        error_msg += ex.what();
        wxMessageBox(error_msg, "Error", wxOK | wxICON_ERROR);
      }

      return wxApp::OnExceptionInMainLoop();
    }

  private:
    wxWindow *frame_;
};

wxIMPLEMENT_APP(SchunkDiscoverApp);
