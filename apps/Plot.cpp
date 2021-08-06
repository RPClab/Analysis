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

int main()
{
  std::string filename("Results.csv");
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

  TF1* sigmoid = new TF1("sigmoid","[0]/(1+ TMath::Exp([1]*([2]-x)))",minVoltage,maxVoltage);
  sigmoid->SetParLimits(0,0,100);
  sigmoid->SetParName(0,"#varepsilon_{max}");
  sigmoid->SetParName(1,"#lambda");
  sigmoid->SetParLimits(1,-100,100);
  sigmoid->SetParLimits(2,minVoltage,maxVoltage);
  sigmoid->SetParName(2,"HV_{50%}");
  gr->SetTitle("Efficiency vs Applied voltage");
  gr->GetXaxis()->SetTitle("Applied voltage (V)");
  gr->GetYaxis()->SetTitle("Efficiency (#varepsilon)");

  gr->Fit("sigmoid");
  gr->Draw("AP");


  c1->SaveAs("toto.png","Q");

  std::exit(EXIT_SUCCESS);
}
