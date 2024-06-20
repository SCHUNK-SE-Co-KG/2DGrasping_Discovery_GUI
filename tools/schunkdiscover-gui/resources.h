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

#ifndef REGISTER_RESOURCES
#define REGISTER_RESOURCES

/**
 * @brief Registers virtual resource files using wxMemoryFSHandler.
 * They can be retrieved in wxWidgets with the "memory:" prefix.
 */
void registerResources();

const static std::string ROBOCEPTION = "Roboception GmbH";
const static std::string SCHUNK = "Schunk SE";
const static std::string RC_VISARD = "rc_visard";

#endif /* REGISTER_RESOURCES */
