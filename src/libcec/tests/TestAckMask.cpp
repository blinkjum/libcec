#include "TestAckMask.h"
#include <cstdio>

int main()
{
  CEC::cec_logical_addresses addresses;
  addresses.Clear();
  addresses.Set(CEC::CECDEVICE_PLAYBACKDEVICE1);
  CEC::BuildTestAckAddresses(0x0020, addresses);
  if (addresses.AckMask() != 0x0020 || addresses.primary != CEC::CECDEVICE_AUDIOSYSTEM)
  {
    std::fprintf(stderr, "FAIL: previous LA 4 survives LA 5 selection: got mask=%04X primary=%X\n",
                 addresses.AckMask(), addresses.primary);
    return 1;
  }

  // Reuse objects filled with every possible old LA, including F. This tests
  // the actual native converter, independently of Python's requested-mask cache.
  for (unsigned int mask = 0; mask <= 0x7FFF; ++mask)
  {
    addresses.Clear();
    for (int la = 0; la < 16; ++la)
      addresses.Set(static_cast<CEC::cec_logical_address>(la));
    CEC::BuildTestAckAddresses(static_cast<uint16_t>(mask), addresses);
    int primary = CEC::CECDEVICE_UNREGISTERED;
    for (int la = 0; la < 15; ++la)
      if (mask & (1u << la)) { primary = la; break; }
    if (addresses.AckMask() != mask || addresses.primary != primary)
    {
      std::fprintf(stderr, "FAIL: requested=%04X actual=%04X primary=%X expected=%X\n",
                   mask, addresses.AckMask(), addresses.primary, primary);
      return 2;
    }
  }
  std::puts("PASS: LA 4 -> LA 5 replacement and all 32768 valid native ACK masks, including release");
  return 0;
}
