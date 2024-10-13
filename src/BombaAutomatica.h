#include <SinricProDevice.h>
#include <Capabilities/PowerStateController.h>
#include <Capabilities/RangeController.h>

class BombaAutomatica 
: public SinricProDevice
, public PowerStateController<BombaAutomatica>
, public RangeController<BombaAutomatica> {
  friend class PowerStateController<BombaAutomatica>;
  friend class RangeController<BombaAutomatica>;
public:
  BombaAutomatica(const String &deviceId) : SinricProDevice(deviceId, "BombaAutomatica") {};
};

 