#include "stockpreyfullprinter.h"
#include "areatime.h"
#include "stockpreystdinfo.h"
#include "errorhandler.h"
#include "conversion.h"
#include "charptrvector.h"
#include "stock.h"
#include "readword.h"
#include "readaggregation.h"
#include "gadget.h"

#include "runid.h"
extern RunId RUNID;

//This printer prints all the information on the consumption of the prey,
//at the level of most disaggregation.

StockPreyFullPrinter::StockPreyFullPrinter(CommentStream& infile,
  const AreaClass* const Area, const TimeClass* const TimeInfo)
  : Printer(STOCKPREYFULLPRINTER), stockname(0), preyinfo(0) {

  ErrorHandler handle;
  char text[MaxStrLength];
  strncpy(text, "", MaxStrLength);
  int i;

  stockname = new char[MaxStrLength];
  strncpy(stockname, "", MaxStrLength);
  ReadWordAndValue(infile, "stockname", stockname);

  //Read in area aggregation from file
  char filename[MaxStrLength];
  strncpy(filename, "", MaxStrLength);
  ifstream datafile;
  CommentStream subdata(datafile);

  charptrvector areaindex;
  intmatrix tmpareas;
  ReadWordAndValue(infile, "areaaggfile", filename);
  datafile.open(filename);
  CheckIfFailure(datafile, filename);
  handle.Open(filename);
  i = ReadAggregation(subdata, tmpareas, areaindex);
  handle.Close();
  datafile.close();
  datafile.clear();

  //Check if we read correct input
  if (tmpareas.Nrow() != 1)
    handle.Message("Error - there should be only one aggregated area for stockpreyfullprinter");

  for (i = 0; i < tmpareas.Ncol(0); i++)
    outerareas.resize(1, tmpareas[0][i]);

  //Must change from outer areas to inner areas.
  areas.resize(outerareas.Size());
  for (i = 0; i < areas.Size(); i++)
    if ((areas[i] = Area->InnerArea(outerareas[i])) == -1)
      handle.UndefinedArea(outerareas[i]);

  //Open the printfile
  ReadWordAndValue(infile, "printfile", filename);
  outfile.open(filename, ios::out);
  CheckIfFailure(outfile, filename);

  infile >> text >> ws;
  if (!(strcasecmp(text, "yearsandsteps") == 0))
    handle.Unexpected("YearsAndSteps", text);
  if (!aat.ReadFromFile(infile, TimeInfo))
    handle.Message("Wrong format for yearsandsteps");

  //prepare for next printfile component
  infile >> ws;
  if (!infile.eof()) {
    infile >> text >> ws;
    if (!(strcasecmp(text, "[component]") == 0))
      handle.Unexpected("[component]", text);
  }

  //finished initializing. Now print first line.
  outfile << "; ";
  RUNID.print(outfile);
  outfile << "; Output file for the prey " << stockname
    << "\n; year-step-area-age-length-number consumed-biomass consumed"
    << "-number consumed by length-biomass consumed by length\n";
  outfile.flush();
}

StockPreyFullPrinter::~StockPreyFullPrinter() {
  outfile.close();
  outfile.clear();
  delete preyinfo;
  delete[] stockname;
}

void StockPreyFullPrinter::SetStock(Stockptrvector& stockvec) {
  charptrvector stocknames(1, stockname);
  Stockptrvector stocks;
  int index = 0;
  int i, j;

  for (i = 0; i < stockvec.Size(); i++)
    for (j = 0; j < stocknames.Size(); j++)
      if (strcasecmp(stockvec[i]->Name(), stocknames[j]) == 0) {
        stocks.resize(1);
        stocks[index++] = stockvec[i];
      }

  if (stocks.Size() != stocknames.Size()) {
    cerr << "Error in printer when searching for stock(s) with name matching:\n";
    for (i = 0; i < stocknames.Size(); i++)
      cerr << (const char*)stocknames[i] << sep;
    cerr << "\nDid only find the stock(s)\n";
    for (i = 0; i < stocks.Size(); i++)
      cerr << (const char*)stocks[i]->Name() << sep;
    cerr << endl;
    exit(EXIT_FAILURE);
  }

  //check that the stock lives in the areas.
  for (i = 0; i < stocks.Size(); i++)
    for (j = 0; j < areas.Size(); j++)
      if (!stocks[i]->IsInArea(areas[j])) {
        cerr << "Error in full printout for stocks preys. The stock "
          << stocks[i]->Name() << " is not defined on the area " << areas[j] << endl;
        exit(EXIT_FAILURE);
      }

  //Here comes some code that is only useful when handling one stock.
  if (stocks[0]->IsEaten())
    preyinfo = new StockPreyStdInfo((StockPrey*)stocks[0]->ReturnPrey(), areas);
  else {
    cerr << "Error in full printout for stock preys. The stock "
      << stocks[0]->Name() << " is not eaten\n";
    exit(EXIT_FAILURE);
  }
}

void StockPreyFullPrinter::Print(const TimeClass* const TimeInfo) {
  if (!aat.AtCurrentTime(TimeInfo))
    return;
  int a, age, l;

  for (a = 0; a < areas.Size(); a++)
    preyinfo->Sum(TimeInfo, areas[a]);

  const LengthGroupDivision* LgrpDiv = preyinfo->ReturnPreyLengthGroupDiv();

  for (a = 0; a < areas.Size(); a++) {
    const bandmatrix& Nbyageandl = preyinfo->NconsumptionByAgeAndLength(areas[a]);
    for (age = Nbyageandl.Minage(); age <= Nbyageandl.Maxage(); age++)
      for (l = 0; l < LgrpDiv->NoLengthGroups(); l++) {
        outfile << setw(smallwidth) << TimeInfo->CurrentYear() << sep
          << setw(smallwidth) << TimeInfo->CurrentStep() << sep
          << setw(smallwidth) << outerareas[a] << sep << setw(smallwidth) << age
          << sep << setprecision(smallprecision) << LgrpDiv->Meanlength(l) << sep;

        //JMB crude filter to remove the 'silly' values from the output
        if ((preyinfo->NconsumptionByAgeAndLength(areas[a])[age][l] < rathersmall)
           || (preyinfo->BconsumptionByAgeAndLength(areas[a])[age][l] < rathersmall)
           || (preyinfo->NconsumptionByLength(areas[a])[l] < rathersmall)
           || (preyinfo->BconsumptionByLength(areas[a])[l] < rathersmall))

          outfile << setw(largewidth) << 0 << sep << setw(largewidth) << 0
            << sep << setw(largewidth) << 0 << sep << setw(largewidth) << 0 << endl;

        else
          outfile << setw(largewidth) << preyinfo->NconsumptionByAgeAndLength(areas[a])[age][l] << sep
            << setw(largewidth) << preyinfo->BconsumptionByAgeAndLength(areas[a])[age][l] << sep
            << setw(largewidth) << preyinfo->NconsumptionByLength(areas[a])[l] << sep
            << setw(largewidth) << preyinfo->BconsumptionByLength(areas[a])[l] << sep << endl;

      }
  }
  outfile.flush();
}
