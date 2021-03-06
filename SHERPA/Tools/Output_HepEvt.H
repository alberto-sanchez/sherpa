#ifndef SHERPA_Tools_Output_HepEvt_H
#define SHERPA_Tools_Output_HepEvt_H

#include "ATOOLS/Org/CXXFLAGS_PACKAGES.H"
#include "SHERPA/Tools/Output_Base.H"
#include "SHERPA/Tools/HepEvt_Interface.H"
#include <fstream>
#include "ATOOLS/Org/Gzip_Stream.H"

namespace SHERPA {

  class Output_HepEvt : public Output_Base {
    HepEvt_Interface m_hepevt;
    std::string m_basename, m_ext;
    ATOOLS::Gzip_Stream m_outstream;
  public:
    Output_HepEvt(const Output_Arguments &args);
    ~Output_HepEvt();
    void ChangeFile();
    void Output(ATOOLS::Blob_List* blobs, double weight);
  };

}

#endif
