#ifndef predpreystdprinter_h
#define predpreystdprinter_h

#include "printer.h"
#include "predstdinfo.h"
#include "predator.h"
#include "prey.h"

/**
 * \class PredPreyStdPrinter
 * \brief This is the base class used to print information about a specific predator - prey relationship
 * \note This will always be overridden by the derived classes that actually print the required information
 */
class PredPreyStdPrinter : public Printer {
public:
  /**
   * \brief This is the default PredPreyStdPrinter constructor
   * \param infile is the CommentStream to read the printer parameters from
   * \param Area is the AreaClass for the current model
   * \param TimeInfo is the TimeClass for the current model
   */
  PredPreyStdPrinter(CommentStream& infile, const AreaClass* const Area, const TimeClass* const TimeInfo);
  /**
   * \brief This is the default PredPreyStdPrinter destructor
   */
  virtual ~PredPreyStdPrinter();
  /**
   * \brief This will select the stocks, predators and preys required for the printer class to print the requested information
   * \param stockvec is the StockPtrVector of all the available stocks
   * \param predvec is the PredatorPtrVector of all the available predators
   * \param preyvec is the PreyPtrVector of all the available preys
   */
  virtual void setStocksAndPredAndPrey(const StockPtrVector& stockvec,
    const PredatorPtrVector& predvec, const PreyPtrVector& preyvec);
protected:
  /**
   * \brief This will select the predator and prey required for the printer class to print the requested information
   * \param predator is the PopPredator of the predator - prey relationship
   * \param prey is the Prey  of the predator - prey relationship
   * \param IsStockPredator is a flag to denite whether the predator is a modelled stock
   * \param IsStockPrey is a flag to denite whether the prey is a modelled stock
   */
  virtual void setPopPredAndPrey(const PopPredator* predator,
    const Prey* prey, int IsStockPredator, int IsStockPrey) = 0;
  /**
   * \brief This is the name of the predator
   */
  char* predname;
  /**
   * \brief This is the name of the prey
   */
  char* preyname;
  /**
   * \brief This ofstream is the file that all the model information gets sent to
   */
  ofstream outfile;
  /**
   * \brief This is the IntVector used to store aggregated internal area information
   */
  IntVector areas;
  /**
   * \brief This is the IntVector used to store aggregated area information
   */
  IntVector outerareas;
};

/**
 * \class PredPreyStdLengthPrinter
 * \brief This is the base class used to print information about a specific predator - prey relationship, split into length groups
 */
class PredPreyStdLengthPrinter : public PredPreyStdPrinter {
public:
  /**
   * \brief This is the default PredPreyStdLengthPrinter constructor
   * \param infile is the CommentStream to read the printer parameters from
   * \param Area is the AreaClass for the current model
   * \param TimeInfo is the TimeClass for the current model
   */
  PredPreyStdLengthPrinter(CommentStream& infile, const AreaClass* const Area, const TimeClass* const TimeInfo);
  /**
   * \brief This is the default PredPreyStdLengthPrinter destructor
   */
  ~PredPreyStdLengthPrinter();
  /**
   * \brief This will print the requested information for the printer class to the ofstream specified
   * \param TimeInfo is the TimeClass for the current model
   */
  virtual void Print(const TimeClass* const TimeInfo);
protected:
  /**
   * \brief This will select the predator and prey required for the printer class to print the requested information
   * \param predator is the PopPredator of the predator - prey relationship
   * \param prey is the Prey  of the predator - prey relationship
   * \param IsStockPredator is a flag to denite whether the predator is a modelled stock
   * \param IsStockPrey is a flag to denite whether the prey is a modelled stock
   */
  virtual void setPopPredAndPrey(const PopPredator* predator, const Prey* prey,
    int IsStockPredator, int IsStockPrey);
private:
  /**
   * \brief This is the PredStdInfoByLength used to collect information about the consumption of the prey by the predator
   */
  PredStdInfoByLength* predinfo;
  /**
   * \brief This is the PopPredator of the predator - prey relationship
   */
  const PopPredator* predator;
  /**
   * \brief This is the Prey of the predator - prey relationship
   */
  const Prey* prey;
};

/**
 * \class PredPreyStdAgePrinter
 * \brief This is the base class used to print information about a specific predator - prey relationship, split into age groups
 */
class PredPreyStdAgePrinter : public PredPreyStdPrinter {
public:
  /**
   * \brief This is the default PredPreyStdAgePrinter constructor
   * \param infile is the CommentStream to read the printer parameters from
   * \param Area is the AreaClass for the current model
   * \param TimeInfo is the TimeClass for the current model
   */
  PredPreyStdAgePrinter(CommentStream& infile, const AreaClass* const Area, const TimeClass* const TimeInfo);
  /**
   * \brief This is the default PredPreyStdAgePrinter destructor
   */
  ~PredPreyStdAgePrinter();
  /**
   * \brief This will print the requested information for the printer class to the ofstream specified
   * \param TimeInfo is the TimeClass for the current model
   */
  virtual void Print(const TimeClass* const TimeInfo);
protected:
  /**
   * \brief This will select the predator and prey required for the printer class to print the requested information
   * \param predator is the PopPredator of the predator - prey relationship
   * \param prey is the Prey  of the predator - prey relationship
   * \param IsStockPredator is a flag to denite whether the predator is a modelled stock
   * \param IsStockPrey is a flag to denite whether the prey is a modelled stock
   */
  virtual void setPopPredAndPrey(const PopPredator* predator, const Prey* prey,
    int IsStockPredator, int IsStockPrey);
private:
  /**
   * \brief This is the AbstrPredStdInfo used to collect information about the consumption of the prey by the predator
   */
  AbstrPredStdInfo* predinfo;
  /**
   * \brief This is the PopPredator of the predator - prey relationship
   */
  const PopPredator* predator;
  /**
   * \brief This is the Prey of the predator - prey relationship
   */
  const Prey* prey;
};

#endif
