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
#ifndef SCHUNKDISCOVER_GIGE_REQUEST_COUNTER_H
#define SCHUNKDISCOVER_GIGE_REQUEST_COUNTER_H

#include <cstdint>
#include <tuple>

namespace schunkdiscover
{

/**
 * @brief A class wrapping a thread safe request counter.
 */
class GigERequestCounter
{
  public:
    /**
     * @brief Returns the next request number.
     * This method is thread safe.
     */
    static std::tuple<uint8_t, uint8_t> getNext();
};

}

#endif // SCHUNKDISCOVER_GIGE_REQUEST_COUNTER_H
