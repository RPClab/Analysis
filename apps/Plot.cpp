#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <fstream>

#include "TF1.h"
#include "TMath.h"
#include "TCanvas.h"
#include "TGraphErrors.h"
#include "TFrame.h"
#include "TStyle.h"
#include "TAxis.h"
#include "TPaveStats.h"
#include "TFitResult.h"
#include "CLI/CLI.hpp"
#include "fmt/color.h"
#include "TLatex.h"
#include "TLegend.h"
#include "rapidcsv.h"

int main(int argc, char** argv)
{
  CLI::App    app{"Plotter"};
  std::vector<std::string> filenames{"Results.csv"};
  app.add_option("-f,--files", filenames, "Name of the .csv file to process")->check(CLI::ExistingFile);
  std::vector<float> mins;
  app.add_option("-m,--min", mins, "Minimum voltages for the fit (-1) for default")->check(CLI::PositiveNumber);
  std::vector<float> maxs;
  app.add_option("-M,--max", maxs, "Maximun voltages for the fit (-1) for default")->check(CLI::PositiveNumber);
  std::string plotName{"Results.root"};
  app.add_option("-n,--name", plotName, "Name of the file with the plots");
  std::vector<std::string> plotTitles;
  app.add_option("-t,--titles", plotTitles, "Title of the plots");
  try
  {
    app.parse(argc, argv);
  }
  catch(const CLI::ParseError& e)
  {
    return app.exit(e);
  }

  if(mins.size()==maxs.size())
  {
    if(mins.size()==0)
    {
      mins=std::vector<float>(filenames.size(),-1);
      maxs=std::vector<float>(filenames.size(),-1);
    }
    else if (mins.size()!=filenames.size()) std::runtime_error("mins maxs and filenames must have the same size");
  }

  if(plotTitles.size()!=filenames.size()) std::runtime_error("titles should have the same size than files");

  gStyle->SetOptFit();
 // gStyle->SetOptStat(1000000001);
  gStyle->SetStatY(0.90);
  //gStyle->SetPalette(kCMYK);
  //gStyle->SetOptStat();
  //Create the TCanvas
  TCanvas canvas = TCanvas("Canvas","Fitted_Efficiencies",0,0,1200,800);
  canvas.SetGrid();

  auto legend = new TLegend(0.2,0.05*filenames.size());
  for(std::size_t file=0;file!=filenames.size();++file)
  {
    //Read the .csv file
    rapidcsv::Document doc(filenames[file], rapidcsv::LabelParams(0, -1));
    std::vector<float> HVs = doc.GetColumn<float>("HV");
    std::vector<float> efficiencies = doc.GetColumn<float>("Efficiency");
    std::vector<float> errorEfficiencies = doc.GetColumn<float>("Error Efficiency");
    std::vector<float> errorHVs(HVs.size(),0);

    std::vector<float>::iterator result;
    result = std::max_element(HVs.begin(),HVs.end());
    if(maxs[file]==-1||maxs[file]>*result) maxs[file]=*result;
    result = std::min_element(HVs.begin(),HVs.end());
    if(mins[file]==-1||mins[file]<*result) mins[file]=*result;

    TGraphErrors* gr = new TGraphErrors(HVs.size(),&HVs[0],&efficiencies[0],&errorHVs[0],&errorEfficiencies[0]);
    gr->SetTitle(plotTitles[file].c_str());
    gr->SetName(plotTitles[file].c_str());
    //gr->SetMarkerColor(4);
    //gr->SetMarkerStyle(21);
    gr->GetYaxis()->SetRangeUser(0,1.);

    TF1* sigmoid = new TF1("sigmoid","[0]/(1+ TMath::Exp([1]*([2]-x)))",mins[file],maxs[file]);
    sigmoid->SetParLimits(0,0,1.0);
    sigmoid->SetParName(0,"#varepsilon_{max}");
    sigmoid->SetParName(1,"#lambda");
    sigmoid->SetParLimits(1,0,100);
    sigmoid->SetParLimits(2,mins[file]+1,maxs[file]-1);
    sigmoid->SetParName(2,"HV_{50%}");
    sigmoid->SetLineColor(file+1);
    TFitResultPtr r = gr->Fit("sigmoid","REMS","",mins[file],maxs[file]);
    gr->GetXaxis()->SetTitle("Applied voltage (V)");
    gr->GetYaxis()->SetTitle("Efficiency (#varepsilon)");
    gr->SetMarkerSize(1);
    gr->SetMarkerStyle(21);
    gr->SetLineWidth(1);
    gr->SetLineColor(file+1);
    gr->SetFillColor(file+1);
    gr->SetMarkerColor(file+1);
    sigmoid->SetLineColor(file+1);
    sigmoid->SetFillColor(file+1);
    sigmoid->SetMarkerColor(file+1);
    if(file==0)gr->Draw("AP");
    else gr->Draw("PSAMES");
    gPad->Update(); //to force the creation of "stats"
    TPaveStats* st = (TPaveStats*)gr->FindObject("stats");
    // Add a new line in the stat box.
    // Note that "=" is a control character
    gPad->Update(); //to force the creation of "stats"
    st->SetLineColor(file+1);
    st->SetTextColor(file+1);
    gPad->Modified();
    static int line{-1};
    if(file%2==0)
    {
      ++line;
      st->SetX1NDC(0.1); //new x start position
      st->SetX2NDC(0.3); //new x end position
      std::cout<<line<<std::endl;
      st->SetY1NDC(0.9-0.1*line); //new x start position
      st->SetY2NDC(0.8-0.1*line); //new x end position
    }
    else
    {
      std::cout<<line<<std::endl;
      st->SetX1NDC(0.3); //new x start position
      st->SetX2NDC(0.5); //new x end position
      st->SetY1NDC(0.9-0.1*line); //new x start position
      st->SetY2NDC(0.8-0.1*line); //new x end position
    }
    gPad->Modified();

    //legend->SetHeader("The Legend Title","C"); // option "C" allows to center the header
    legend->AddEntry(gr);


  }
  legend->Draw();
  canvas.SaveAs(plotName.c_str(),"Q");
  std::exit(EXIT_SUCCESS);
}
