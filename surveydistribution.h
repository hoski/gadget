#ifndef surveydistribution_h
#define surveydistribution_h

#include "likelihood.h"
#include "stockaggregator.h"
#include "doublematrixptrmatrix.h"
#include "actionattimes.h"
#include "formulavector.h"
#include "timevariablevector.h"
#include "suitfunc.h"

class SurveyDistribution : public Likelihood {
public:
  /**
   * \brief This is the SurveyDistribution constructor
   * \param infile is the CommentStream to read the SurveyDistribution data from
   * \param Area is the AreaClass for the current model
   * \param TimeInfo is the TimeClass for the current model
   * \param keeper is the Keeper for the current model
   * \param weight is the weight for the likelihood component
   * \param name is the name for the likelihood component
   */
  SurveyDistribution(CommentStream& infile, const AreaClass* const Area,
    const TimeClass* const TimeInfo, Keeper* const keeper, double weight, const char* name);
  /**
   * \brief This is the default SurveyDistribution destructor
   */
  virtual ~SurveyDistribution();
  /**
   * \brief This function will calculate the likelihood score for the SurveyDistribution component
   * \param TimeInfo is the TimeClass for the current model
   */
  virtual void addLikelihood(const TimeClass* const TimeInfo);
  /**
   * \brief This function will reset the SurveyDistribution likelihood information
   * \param keeper is the Keeper for the current model
   */
  virtual void Reset(const Keeper* const keeper);
  /**
   * \brief This function will print the summary SurveyDistribution likelihood information
   * \param outfile is the ofstream that all the model information gets sent to
   */
  virtual void Print(ofstream& outfile) const;
  /**
   * \brief This will select the fleets and stocks required to calculate the SurveyDistribution likelihood score
   * \param Fleets is the FleetPtrVector of all the available fleets
   * \param Stocks is the StockPtrVector of all the available stocks
   */
  void setFleetsAndStocks(FleetPtrVector& Fleets, StockPtrVector& Stocks);
  /**
   * \brief This function will print information from each SurveyDistribution likelihood calculation
   * \param outfile is the ofstream that all the model likelihood information gets sent to
   */
  virtual void LikelihoodPrint(ofstream& outfile);
  /**
   * \brief This function will print summary information from each SurveyDistribution likelihood calculation
   * \param outfile is the ofstream that all the model likelihood information gets sent to
   */
  virtual void SummaryPrint(ofstream& outfile);
  virtual void PrintLikelihood(ofstream& catchfile, const TimeClass& TimeInfo);
  virtual void PrintLikelihoodHeader(ofstream& catchfile);
  void calcIndex(const AgeBandMatrix* alptr, const TimeClass* const TimeInfo);
private:
  /**
   * \brief This function will read the SurveyDistribution data from the input file
   * \param infile is the CommentStream to read the SurveyDistribution data from
   * \param TimeInfo is the TimeClass for the current model
   * \param numarea is the number of areas that the likelihood data covers
   * \param numage is the number of age groups that the likelihood data covers
   * \param numlen is the number of length groups that the likelihood data covers
   */
  void readDistributionData(CommentStream& infile, const TimeClass* TimeInfo,
    int numarea, int numage, int numlen);
  /**
   * \brief This function will calculate the likelihood score for the current timestep based on a multinomial function
   * \return likelihood score
   */
  double calcLikMultinomial();
  /**
   * \brief This function will calculate the likelihood score for the current timestep based on a Pearson function
   * \return likelihood score
   */
  double calcLikPearson();
  /**
   * \brief This function will calculate the likelihood score for the current timestep based on a gamma function
   * \return likelihood score
   */
  double calcLikGamma();
  /**
   * \brief This function will calculate the likelihood score for the current timestep based on a logarithmic function
   * \return likelihood score
   */
  double calcLikLog();
  /**
   * \brief This is the StockAggregator used to collect information about the stocks
   */
  StockAggregator* aggregator;
  /**
   * \brief This is the CharPtrVector of the names of the stocks that will be used to calculate the likelihood score
   */
  CharPtrVector stocknames;
  /**
   * \brief This is the name of the SurveyDistribution likelihood component
   */
  char* sdname;
  /**
   * \brief This is the IntMatrix used to store area information
   */
  IntMatrix areas;
  /**
   * \brief This is the IntMatrix used to store age information
   */
  IntMatrix ages;
  /**
   * \brief This is the DoubleVector used to store length information
   */
  DoubleVector lengths;
  /**
   * \brief This is the CharPtrVector of the names of the areas
   */
  CharPtrVector areaindex;
  /**
   * \brief This is the CharPtrVector of the names of the age groups
   */
  CharPtrVector ageindex;
  /**
   * \brief This is the CharPtrVector of the names of the length groups
   */
  CharPtrVector lenindex;
  /**
   * \brief This is the LengthGroupDivision used to store length information
   */
  LengthGroupDivision* LgrpDiv;
  /**
   * \brief This is the name of the fit function to be used to calculate the index
   */
  char* fittype;
  /**
   * \brief This is the identifier of the fit function to be used to calculate the index
   */
  int fitnumber;
  /**
   * \brief This is the name of the likelihood function to be used to calculate the likelihood score
   */
  char* liketype;
  /**
   * \brief This is the identifier of the likelihood function to be used to calculate the likelihood score
   */
  int likenumber;
  /**
   * \brief This ActionAtTimes stores information about when the likelihood score should be calculated
   */
  ActionAtTimes AAT;
  /**
   * \brief This is the IntVector used to store information about the years when the likelihood score should be calculated
   */
  IntVector Years;
  /**
   * \brief This is the IntVector used to store information about the steps when the likelihood score should be calculated
   */
  IntVector Steps;
  /**
   * \brief This is the DoubleMatrixPtrVector used to store survey distribution information specified in the input file
   * \note the indices for this object are [time][age][length]
   */
  DoubleMatrixPtrVector obsDistribution;
  /**
   * \brief This is the DoubleMatrixPtrVector used to store survey distribution information calculated in the model
   * \note the indices for this object are [time][age][length]
   */
  DoubleMatrixPtrVector modelDistribution;
  /**
   * \brief This is the DoubleVector used to store the calculated likelihood information
   */
  DoubleVector likelihoodValues;
  /**
   * \brief This is the index of the timesteps for the likelihood component data
   */
  int index;
  /**
   * \brief This is the value of epsilon used when calculating the likelihood score
   */
  double epsilon;
  SuitFunc* suitfunction;
  DoubleVector q_l; //length dependent catchability factor
  TimeVariableVector parameters;
  IntVector mincol;
  IntVector maxcol;
  int minrow;
  int maxrow;
  int stocktype;
};

#endif