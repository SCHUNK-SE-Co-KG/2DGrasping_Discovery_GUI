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
#include "schunkdiscover/discover.h"
#include "schunkdiscover/ping.h"

#include "discover-thread.h"

#include "discover-frame.h"
#include "event-ids.h"
#include "schunkdiscover/utils.h"

#include <vector>
#include <algorithm>
#include <future>
#include <chrono>

#include <wx/window.h>

wxThread::ExitCode DiscoverThread::Entry()
{
  std::vector<wxVector<wxVariant>> device_list;

  try
  {
    schunkdiscover::Discover discover;
    discover.broadcastRequest();

    std::chrono::steady_clock::time_point tstart=std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point tend=tstart;

    std::vector<schunkdiscover::DeviceInfo> infos;

    while (discover.getResponse(infos, 100) ||
      std::chrono::duration<double, std::milli>(tend-tstart).count() < 3000)
    {
      tend=std::chrono::steady_clock::now();
    }

    std::sort(infos.begin(), infos.end());
    const auto it = std::unique(infos.begin(), infos.end(),
                                [](const schunkdiscover::DeviceInfo &lhs,
                                   const schunkdiscover::DeviceInfo &rhs)
                                {
                                  return lhs.getMAC() == rhs.getMAC() &&
                                         lhs.getIfaceName() ==
                                         rhs.getIfaceName();
                                });
    infos.erase(it, infos.end());

    std::vector<std::future<bool>> reachable(infos.size());
    std::transform(
        infos.begin(), infos.end(), reachable.begin(),
        [](const schunkdiscover::DeviceInfo &info) {
          return info.isValid()
                     ? std::async(
                           std::launch::async,
                           [&info] { return checkReachabilityOfSensor(info); })
                     : std::future<bool>{};
        });

    const schunkdiscover::DeviceInfo *last_info = nullptr;

    size_t i = 0;
    for (auto info_it = infos.cbegin(); info_it != infos.cend(); ++info_it, ++i)
    {
      const auto& info = *info_it;
      if (!info.isValid())
      {
        continue;
      }

      if (last_info && last_info->getMAC() == info.getMAC())
      {
        device_list.back()[DiscoverFrame::IFACE] = wxVariant(
                device_list.back()[DiscoverFrame::IFACE].GetString() + ',' +
                info.getIfaceName());
        continue;
      }

      last_info = &info;

      const std::string &name = info.getUserName().empty()
                                ? info.getModelName()
                                : info.getUserName();

      const std::string &manufacturer = info.getManufacturerName();

      wxVector<wxVariant> data(DiscoverFrame::NUM_COLUMNS);
      data[DiscoverFrame::NAME] = wxVariant(name);
      data[DiscoverFrame::MANUFACTURER] = wxVariant(manufacturer);
      data[DiscoverFrame::MODEL] = wxVariant(info.getModelName());
      data[DiscoverFrame::SERIAL] = wxVariant(info.getSerialNumber());
      data[DiscoverFrame::IP] = wxVariant(ip2string(info.getIP()));
      data[DiscoverFrame::MAC] = wxVariant(mac2string(info.getMAC()));
      data[DiscoverFrame::IFACE] = wxVariant(info.getIfaceName());
      data[DiscoverFrame::REACHABLE] = wxVariant(
              reachable[i].get() ? L"\u2713" : L"\u2717");
      data[DiscoverFrame::SENDERIP] = wxVariant(ip2string(info.getSenderIP()));
      data[DiscoverFrame::ROBOTIPNETWORK] = wxVariant(ip2string(info.getRobotIPNetwork()));
      data[DiscoverFrame::CAMERAIPNETWORK] = wxVariant(ip2string(info.getCameraIPNetwork()));

      device_list.push_back(std::move(data));
    }
  }
  catch (const std::exception &ex)
  {
    wxThreadEvent event(wxEVT_COMMAND_DISCOVERY_ERROR);
    event.SetString(ex.what());
    parent_->GetEventHandler()->QueueEvent(event.Clone());

    return ExitCode(1);
  }

  wxThreadEvent event(wxEVT_COMMAND_DISCOVERY_COMPLETED);
  event.SetPayload(device_list);
  parent_->GetEventHandler()->QueueEvent(event.Clone());

  return ExitCode(0);
}
