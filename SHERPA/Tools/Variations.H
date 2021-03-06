#ifndef SHERPA_Tools_Variations_H
#define SHERPA_Tools_Variations_H

#include <ostream>
#include <map>
#include <string>
#include <vector>

#include "ATOOLS/Org/CXXFLAGS_PACKAGES.H"
#include "ATOOLS/Org/Exception.H"
#include "ATOOLS/Org/Data_Reader.H"
#include "ATOOLS/Org/Shell_Tools.H"

#define VARIATION_WARNINGS 0

namespace ATOOLS { class Data_Reader; }
namespace PDF { class PDF_Base; }
namespace MODEL { class One_Running_AlphaS; }


namespace SHERPA {

  class Variation_Parameters;
  class Variation_Weights;

  //! Initialise and hold all information that is needed for varying input parameters
  class Variations {
  public:
    Variations(ATOOLS::Data_Reader * const);
    ~Variations();
    typedef std::vector<Variation_Parameters *> Parameters_Vector;
    const Parameters_Vector * GetParametersVector() const { return &m_parameters_vector; }
    void IncrementOrInitialiseWarningCounter(const std::string);

  private:
    Parameters_Vector m_parameters_vector;

    //! (name -> counter value) map used to track warnings
    std::map<std::string, unsigned long> m_warnings;

    void CollectAndOutputWarnings();
#if defined USING__LHAPDF && defined USING__LHAPDF6
    void LoadLHAPDFInterfaceIfNecessary(ATOOLS::Data_Reader * const);
#endif
    void InitialiseParametersVector(ATOOLS::Data_Reader * const);
    std::vector<std::string> VariationArguments(ATOOLS::Data_Reader * const);
    std::vector<std::string> VariationArguments(ATOOLS::Data_Reader * const,
                                                std::string tag);
    std::vector<std::string> VariationArgumentParameters(std::string);
    void AddParameters(std::vector<std::string>, ATOOLS::Data_Reader * const);
    struct PDFs_And_AlphaS {
      PDFs_And_AlphaS();
      PDFs_And_AlphaS(ATOOLS::Data_Reader * const, std::string pdfname, size_t pdfmember);
      std::vector<PDF::PDF_Base *> m_pdfs;
      MODEL::One_Running_AlphaS *p_alphas;
    };

    //! whether the muR variation factor should be applied to the shower AlphaS scale arguments
    bool m_reweightsplittingalphasscales;
    //! whether the muF variation factor should be applied to the shower PDF scale arguments
    bool m_reweightsplittingpdfsscales;
  };

  std::ostream& operator<<(std::ostream&, const Variations&);


  /*!
   * Hold a set of input parameters and factors for a single input parameter variation
   *
   * Used as an argument to a reweighting function.
   */
  struct Variation_Parameters {
    Variation_Parameters(double muR2fac, double muF2fac,
                         double showermuR2fac, double showermuF2fac,
                         PDF::PDF_Base *pdf1,
                         PDF::PDF_Base *pdf2,
                         MODEL::One_Running_AlphaS *alphas,
                         bool deletepdfsandalphas):
      m_muR2fac(muR2fac), m_muF2fac(muF2fac),
      m_showermuR2fac(showermuR2fac), m_showermuF2fac(showermuF2fac),
      p_pdf1(pdf1), p_pdf2(pdf2), p_alphas(alphas),
      m_deletepdfsandalphas(deletepdfsandalphas),
      m_name(GenerateName())
    {};
    ~Variation_Parameters();

    void IncrementOrInitialiseWarningCounter(const std::string);

    const double m_muR2fac, m_muF2fac;
    const double m_showermuR2fac, m_showermuF2fac;
    //! Pointers to the beam PDFs, which can be NULL for (semi-)leptonic events
    PDF::PDF_Base * const p_pdf1;
    PDF::PDF_Base * const p_pdf2;
    MODEL::One_Running_AlphaS * const p_alphas;
    //! Set whether the pointers to the PDFs and the AlphaS should be deleted after usage
    const bool m_deletepdfsandalphas;
    const std::string m_name;

  private:
    //! Return a name based on the scale and PDF variations
    std::string GenerateName() const;
    //! Return part of a name based on a tag name and the associated value
    template <typename U>
    std::string GenerateNamePart(std::string tag, U value) const;
    //! (name -> counter value) map used to track warnings
    std::map<std::string, unsigned long> m_warnings;
    friend class Variations;
  };


  //! Convenience wrapper for storing and passing subevent weights corresponding to one variation
  class Subevent_Weights_Vector : public std::vector<double> {
  public:
    // inherit useful vector constructors
    //! Construct an empty weight vector
    Subevent_Weights_Vector();
    //! Construct a weight vector of size times 1.0 (i.e. we do not use the standard default 0.0!)
    Subevent_Weights_Vector(size_type count, const double& value = 1.0);
    //! Multiply all weights with a common factor
    Subevent_Weights_Vector & operator*=(const double&);
    /*!
     * Multiply all subevent weights one by one with those of another vec
     *
     * The other subevent weights must either be of equal size or they have to
     * contain only one entry. In the latter case, this single entry is used as
     * a common factor for all subevent weights.
     */
    Subevent_Weights_Vector & operator*=(const Subevent_Weights_Vector&);
  };

  std::ostream& operator<<(std::ostream&, const Subevent_Weights_Vector&);


  //! Types to denote different variation sources
  struct Variations_Type {
    enum code {
      all,     //!< use for weight retrieval, to get the product of the below
      main,    //!< use for everything that is not one of the below
      sudakov  //!< use for weight factors from the reweighting of the Sudakov
               //!< Veto Algorithm, i.e. the reweighting of parton shower
               //!< emissions
    };
  };

  std::ostream& operator<<(std::ostream&, const Variations_Type::code&);


  typedef
    std::map<Variations_Type::code, std::vector<Subevent_Weights_Vector> >
    Subevent_Weights_Map;


  //! Wrap the actual event weight variations and apply reweighting functions to it
  class Variation_Weights {
  public:

    Variation_Weights(Variations * v):
      p_variations(v),
      m_reweighting(false)
    {}

    void Reset();

    /*!
     * Multiply each weight with the result of the passed reweighting function
     *
     * The reweighting function is evaluated once for each parameter variation.
     */
    template <class Reweighter, class Data>
    void UpdateOrInitialiseWeights(double (Reweighter::*reweightfct)(Variation_Parameters *, Variation_Weights *, Data &),
                                   Reweighter & reweighter,
                                   Data & data,
                                   Variations_Type::code t=Variations_Type::main)
    {
      if (!AreWeightsInitialised(t)) {
        InitialiseWeights(Subevent_Weights_Vector(1, 1.0), t);
      }
      const Variations::Parameters_Vector * const paramsvec(p_variations->GetParametersVector());
      m_reweighting = true;
      for (m_currentparametersindex = 0;
           m_currentparametersindex < paramsvec->size();
           ++m_currentparametersindex ) {
        double weight((reweighter.*reweightfct)((*paramsvec)[m_currentparametersindex], this, data));
        m_weights[t][m_currentparametersindex] *= weight;
      }
      m_reweighting = false;
    }

    /*!
     * Initialise weights with the result of the passed reweighting function
     *
     * The reweighting function is evaluated once for each parameter variation.
     * This function is supposed to return a vector weights to account for
     * possible subevent weights.
     */
    template <class Reweighter, class Data>
    void InitialiseWeights(Subevent_Weights_Vector (Reweighter::*reweightfct)(Variation_Parameters *, Variation_Weights *, Data &),
                           Reweighter & reweighter,
                           Data & data,
                           Variations_Type::code t=Variations_Type::main)
    {
      if (AreWeightsInitialised(t)) {
        THROW(fatal_error, "The variation weights are already initialised");
      }
      const Variations::Parameters_Vector * const paramsvec(p_variations->GetParametersVector());
      const size_t size(paramsvec->size());
      m_weights[t].clear();
      m_weights[t].reserve(size);
      m_reweighting = true;
      for (m_currentparametersindex = 0;
           m_currentparametersindex < size;
           ++m_currentparametersindex ) {
        m_weights[t].push_back(
            (reweighter.*reweightfct)((*paramsvec)[m_currentparametersindex],
                                      this,
                                      data));
      }
      m_reweighting = false;
    }

    //! Multiply *all* weights with a common factor
    Variation_Weights & operator*=(const double &);

    //! Multiply weights with weights from another set of variation weights
    Variation_Weights & operator*=(const Variation_Weights &);

    //! Get variation name at a given index
    std::string GetVariationNameAt(Variations::Parameters_Vector::size_type) const;

    /*!
     * Get a variation weight at a given index and a given subevent index
     *
     * If the subevent index is -1 (default), the sum of all subevent weights
     * will be returned. For events without subevents, this is equivalent to
     * just returning the first (and only) weight.
     */
    double GetVariationWeightAt(Variations::Parameters_Vector::size_type,
                                Variations_Type::code t=Variations_Type::all,
                                int subevtidx=-1) const;

    // Obtain informations about the parameter variations
    size_t GetNumberOfVariations() const;
    size_t GetNumberOfSubevents() const;
    Variations * GetVariations() const { return p_variations; }
    Variations::Parameters_Vector::size_type CurrentParametersIndex() const;

    inline void IncrementOrInitialiseWarningCounter(const std::string name)
    { p_variations->IncrementOrInitialiseWarningCounter(name); }

private:
    Variations * p_variations;
    //! a vector of (sub)event weights for each set of parameters
    Subevent_Weights_Map m_weights;
    /*!
     * index of the parameters for the current reweighting
     *
     * This is only valid from within a callback to a reweighting function.
     * This can for example be helpful when some additional variation weights
     * are used during the reweighting (e.g. for subprocesses).
     */
    Variations::Parameters_Vector::size_type m_currentparametersindex;
    //! whether this instance is currently in a reweighting loop
    bool m_reweighting;

    // the non-member function operator<< will have access to private members
    friend std::ostream & operator<<(std::ostream &, const Variation_Weights &);

    void InitialiseWeights(const Subevent_Weights_Vector & subweights,
                           const Variations_Type::code t=Variations_Type::main);

    bool AreWeightsInitialised(
        const Variations_Type::code t=Variations_Type::main) const;
  };

  std::ostream & operator<<(std::ostream &, const Variation_Weights &);

}

#endif // #ifndef SHERPA_Tools_Variations_H
