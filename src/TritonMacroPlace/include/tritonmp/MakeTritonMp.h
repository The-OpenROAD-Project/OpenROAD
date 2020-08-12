
#ifndef MAKE_TRITONMP
#define MAKE_TRITONMP

namespace MacroPlace {
class TritonMacroPlace;
}

namespace ord {

class OpenRoad;

MacroPlace::TritonMacroPlace *
makeTritonMp();

void
initTritonMp(OpenRoad *openroad);

void
deleteTritonMp(MacroPlace::TritonMacroPlace *tritonmp);

}

#endif
