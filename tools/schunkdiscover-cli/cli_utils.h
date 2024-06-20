/*
 * schunkdiscover - the network discovery tool for Schunk 2D Grasping Kit
 *
 * Copyright (c) 2018 Roboception GmbH
 * All rights reserved
 *
 * Author: Raphael Schaller
 *
 *
 * Copyright (c) 2024 Schunk SE & Co. KG
 * All rights reserved
 * 
 * Author: Divya Sasidharan
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

#ifndef SCHUNKDISCOVER_CLI_UTILS_H
#define SCHUNKDISCOVER_CLI_UTILS_H

#include <string>
#include <vector>
#include <map>

#include <schunkdiscover/deviceinfo.h>

struct DeviceFilter
{
  std::vector<std::string> name = {};
  std::vector<std::string> serial = {};
  std::vector<std::string> mac = {};
  std::vector<std::string> iface = {};
  std::vector<std::string> model = {};
};

int parseFilterArguments(int argc, char **argv, DeviceFilter &filter);

bool filterDevice(const schunkdiscover::DeviceInfo &device_info,
                  const DeviceFilter &filter);

std::vector<schunkdiscover::DeviceInfo> discoverWithFilter(
    const DeviceFilter &filter);

void printTable(std::ostream &oss,
                const std::vector<std::vector<std::string>> &to_be_printed);

void printDeviceTable(std::ostream &oss,
                      const std::vector<schunkdiscover::DeviceInfo> &devices,
                      bool print_header, bool iponly, bool serialonly);

template<typename K, typename V>
int getMaxCommandLen(const std::map<K, V> &commands)
{
  int max_size = 0;
  for (const auto &cmd : commands)
  {
    max_size = std::max(max_size, static_cast<int>(cmd.first.length()));
  }
  return max_size;
}

#endif //SCHUNKDISCOVER_CLI_UTILS_H
