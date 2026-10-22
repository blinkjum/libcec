#pragma once

#include "cectypes.h"

namespace CEC
{
  inline void BuildTestAckAddresses(uint16_t mask, cec_logical_addresses& addresses)
  {
    // cec_logical_addresses is a POD with no default constructor. In particular,
    // Set(5) alone must never preserve a previous LA 4 or a stale primary LA.
    addresses.Clear();
    for (int address = 0; address < 15; ++address)
      if (mask & (1u << address))
        addresses.Set(static_cast<cec_logical_address>(address));
  }
}
