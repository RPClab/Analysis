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

#include "CLI/CLI.hpp"
#include "fmt/color.h"

std::string readFileIntoString(const std::string& path)
{
  auto ss = std::ostringstream{};
  std::ifstream input_file(path);
  if(!input_file.is_open())
  {
    std::cerr << "Could not open the file - '"<< path << "'" << std::endl;
    std::exit(EXIT_FAILURE);
  }
  ss << input_file.rdbuf();
  return ss.str();
}

int main(int argc, char** argv)
{
  CLI::App    app{"Plotter"};
  std::string filename{"Results.csv"};
  app.add_option("-f,--file", filename, "Name of the .csv file to process")->check(CLI::ExistingFile);
  float min{std::numeric_limits<float>::min()};
  app.add_option("-m,--min", min, "Minimum voltage for the fit")->check(CLI::PositiveNumber);
  float max{std::numeric_limits<float>::max()};
  app.add_option("-M,--max", max, "Maximun voltage for the fit")->check(CLI::PositiveNumber);

  try
  {
    app.parse(argc, argv);
  }
  catch(const CLI::ParseError& e)
  {
    return app.exit(e);
  }

  std::string file_contents;
  std::map<int, std::vector<std::string>> csv_contents;
  char delimiter = ',';

  file_contents = readFileIntoString(filename);

  std::istringstream sstream(file_contents);
  std::vector<std::string> items;
  std::string record;

  int counter = 0;
  while (std::getline(sstream, record)) {
    std::istringstream line(record);
    while (std::getline(line, record, delimiter)) {
      items.push_back(record);
    }

    csv_contents[counter] = items;
    items.clear();
    counter += 1;
  }

  //TF1 func("sig", "[0]/(1+ TMath::Exp(-[1]*(x-[2])))", 0, 12);
  TCanvas* c1 = new TCanvas("c1","A Simple Graph with error bars",200,10,700,500);
  //c1->SetFillColor(42);
  c1->SetGrid();
  //c1->GetFrame()->SetFillColor(21);
  //c1->GetFrame()->SetBorderSize(12);
  auto gr = new TGraphErrors(csv_contents.size());
  gr->SetTitle("TGraphErrors Example");
  gr->SetMarkerColor(4);
  gr->SetMarkerStyle(21);
  gr->GetYaxis()->SetRangeUser(0,1.);


  float minVoltage{9000};
  float maxVoltage{-100};
  for(std::map<int, std::vector<std::string>>::iterator it=csv_contents.begin(); it!=csv_contents.end();++it)
  {
    static int j{0};
    if(minVoltage>std::stof(it->second[0])) minVoltage=std::stof(it->second[0]);
    if(maxVoltage<std::stof(it->second[0])) maxVoltage=std::stof(it->second[0]);
    gr->SetPoint(j,std::stof(it->second[0]),std::stof(it->second[1]));
    gr->SetPointError(j,0.,std::stof(it->second[2]));
    ++j;
  }
  gStyle->SetOptFit();
  gStyle->SetOptStat(1111111);
  // Set stat options
  gStyle->SetStatY(0.5);
  // Set y-position (fraction of pad size)
  gStyle->SetStatX(0.85);
  // Set x-position (fraction of pad size)
  //gStyle->SetStatW(0.4);
  // Set width of stat-box (fraction of pad size)
  //gStyle->SetStatH(0.2);

  if (min > minVoltage) minVoltage=min;
  if (max < maxVoltage) maxVoltage=max;

  std::cout<<minVoltage<<"  "<<maxVoltage<<std::endl;

  TF1* sigmoid = new TF1("sigmoid","[0]/(1+ TMath::Exp([1]*([2]-x)))",minVoltage,maxVoltage);
  sigmoid->SetParLimits(0,0,1.0);
  sigmoid->SetParName(0,"#varepsilon_{max}");
  sigmoid->SetParName(1,"#lambda");
  sigmoid->SetParLimits(1,0,100);
  sigmoid->SetParLimits(2,minVoltage,maxVoltage);
  sigmoid->SetParName(2,"HV_{50%}");
  gr->SetTitle("Efficiency vs Applied voltage");
  gr->GetXaxis()->SetTitle("Applied voltage (V)");
  gr->GetYaxis()->SetTitle("Efficiency (#varepsilon)");

  gr->Fit("sigmoid","REM","",minVoltage,maxVoltage);
  gr->Draw("AP");

  std::size_t found = filename.find(".csv");
  c1->SaveAs(fmt::format("{}_Fit_{:02.0f}_{:02.0f}.png",filename.substr(0,found),minVoltage,maxVoltage).c_str(),"Q");

  std::exit(EXIT_SUCCESS);
}
