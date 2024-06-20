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
#ifndef FORCE_IP_H
#define FORCE_IP_H

#ifdef WIN32
#include "socket_windows.h"
#else
#include "socket_linux.h"
#endif

namespace schunkdiscover
{

/**
 * @brief Class for sending GigE Vision FORCEIP_CMD to camera.
 */
class ForceIP
{
  public:
#ifdef WIN32
    typedef SocketWindows SocketType;
#else
    typedef SocketLinux SocketType;
#endif

  public:
    /**
     * @brief Constructor.
     * Sets up sockets.
     */
    ForceIP();

    /**
     * @brief Send FORCEIP_CMD.
     * @param mac the destination MAC address
     * @param ip the desired IP address
     * @param subnet the desired subnet mask
     * @param gateway the desired default gateway
     * @param is_temporary if true, the IP address is only temporary
     *
     * @note If \p ip is set to 0, the camera will perform a reconnect.
     */
    void sendCommand(std::uint64_t mac, std::uint32_t ip,
                     std::uint32_t subnet, std::uint32_t gateway, bool is_temporary=true);

  private:
    std::vector<SocketType> sockets_;
};

}

#endif // FORCE_IP_H
