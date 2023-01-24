#ifndef DFT_MAKE_DFT_H_
#define DFT_MAKE_DFT_H_

namespace ord {
class OpenRoad;
}

namespace dft {
class Dft;

Dft *makeDft();
void initDft(ord::OpenRoad *openroad);
void deleteDft(Dft *dft);

}

#endif // DFT_MAKE_DFT_H_
