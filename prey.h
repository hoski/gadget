#ifndef prey_h
#define prey_h

#include "popinfovector.h"
#include "conversionindex.h"
#include "commentstream.h"
#include "charptrvector.h"
#include "hasname.h"
#include "livesonareas.h"
#include "agebandmatrix.h"
#include "gadget.h"

class Prey;
class Keeper;

enum PreyType { PREYTYPE = 1, MORTPREYTYPE, STOCKPREYTYPE, LENGTHPREYTYPE };

/**
 * \class Prey
 * \brief This is the base class used to model the consumption of a prey
 * \note This will be overridden by the derived classes that actually calculate the consumption
 */
class Prey : public HasName, public LivesOnAreas {
public:
  Prey(CommentStream& infile, const IntVector& areas, const char* givenname, Keeper* const keeper);
  /**
   * \brief This is the Prey constructor for a prey with a given length group
   * \param lengths is the DoubleVector of length groups of the prey
   * \param areas is the IntVector of areas that the prey lives on
   * \param givenname is the name of the prey
   */
  Prey(const DoubleVector& lengths, const IntVector& areas, const char* givenname);
  /**
   * \brief This is the default Prey destructor
   */
  virtual ~Prey() = 0;
  virtual void Sum(const AgeBandMatrix& Alkeys, int area, int NrofSubstep) {};
  virtual void Subtract(AgeBandMatrix& Alkeys, int area);
  void addConsumption(int area, const DoubleIndexVector& predconsumption);
  virtual void setCI(const LengthGroupDivision* const GivenLDiv);
  virtual void Print(ofstream& outfile) const;
  double Biomass(int area, int length) const { return biomass[AreaNr[area]][length]; };
  double Biomass(int area) const { return total[AreaNr[area]]; };
  int TooMuchConsumption(int area) const { return tooMuchConsumption[AreaNr[area]]; };
  virtual void checkConsumption(int area, int NrOfSubsteps);
  double Ratio(int area, int length) const { return ratio[AreaNr[area]][length]; };
  double Length(int j) const { return LgrpDiv->meanLength(j); };
  int NoLengthGroups() const { return LgrpDiv->NoLengthGroups(); };
  const DoubleVector& Bconsumption(int area) const { return consumption[AreaNr[area]]; };
  const DoubleVector& OverConsumption(int area) const { return overconsumption[AreaNr[area]]; };
  const LengthGroupDivision* returnLengthGroupDiv() const { return LgrpDiv; };
  virtual void Reset();
  const PopInfoVector& NumberPriortoEating(int area) const { return numberPriortoEating[AreaNr[area]]; };
  //The following functions are added 3/8 98 by kgf
  void Multiply(AgeBandMatrix& stock_alkeys, const DoubleVector& rat);
  virtual void calcZ(int area, const DoubleVector& nat_m) {};
  virtual void calcMeanN(int area) {};
  virtual void setCannibalism(int area, const DoubleVector& cann) {};
  virtual void setAgeMatrix(int pred_no, int area, const DoubleVector& agegroupno) {};
  virtual void setConsumption(int area, int pred_no, const BandMatrix& consum) {};
  const DoubleVector& getCons(int area) const { return cons[AreaNr[area]]; };
  /**
   * \brief This will return the type of prey class
   * \return type
   */
  PreyType Type() { return type; };
protected:
  void InitialiseObjects();
  ConversionIndex* CI;
  LengthGroupDivision* LgrpDiv;
  PopInfoMatrix Number;
  PopInfoMatrix numberPriortoEating;
  DoubleMatrix biomass;
  DoubleMatrix ratio;
  DoubleMatrix consumption;
  IntVector tooMuchConsumption;      //set if any lengthgr is overconsumed in area
  DoubleVector total;
  DoubleMatrix overconsumption;
  DoubleMatrix overcons;             //overconsumption of prey in subinterval
  DoubleMatrix cons;                 //consumption of prey in subinterval
  /**
   * \brief This denotes what type of prey class has been created
   */
  PreyType type;
};

#endif
