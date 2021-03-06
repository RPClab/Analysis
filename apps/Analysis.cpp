#include "CLI/CLI.hpp"
#include "Channel.hpp"
#include "Event.hpp"
#include "TCanvas.h"
#include "TFile.h"
#include "TH1F.h"
#include "TROOT.h"
#include "TSpectrum.h"
#include "TGraph.h"
#include "TTree.h"
#include "TTreeReader.h"
#include "TPaveLabel.h"
#include "TArrow.h"
#include "TStyle.h"
#include "TLine.h"
#include "TSystemDirectory.h"
#include "TLatex.h"
#include "TGaxis.h"
#include <algorithm>
#include <iostream>
#include <map>
#include <utility>
#include <vector>
#include <filesystem>
#include <limits>

#include "TApplication.h"
namespace fs = std::filesystem;

#include "rapidcsv.h"

#include "Style.hpp"
#include "Screen.hpp"

enum class Polarity
{
  Positive=1,
  Negative=-1,
  Unknown=0,
};

namespace Analysis
{
  class Channel
  {
  public:
    Channel(const int& id,const int& number,const int& chamber,const Polarity& polarity) : m_ID(id),m_Number(number), m_Chamber(chamber), m_Polarity(polarity)
    {

    }
    void setNumber(const int& ch)
    {
      m_Number=ch;
    }
    int getNumber() const
    {
      return m_Number;
    }
    void setID(const int& id)
    {
      m_ID=id;
    }
    // The channel number from the digitizer point of view
    int getID() const
    {
      return m_ID;
    }
    int getSignPolarity() const
    {
      if(m_Polarity==Polarity::Negative) return -1;
      else return +1;
    }
    void setPolarity(const Polarity& polarity)
    {
      m_Polarity=polarity;
    }
    Polarity getPolarity()
    {
      return m_Polarity;
    }
    void setOnChamber(const int& chamber)
    {
      m_Chamber=chamber;
    }
    int getOnChamber() const
    {
      return m_Chamber;
    }
  private:
    Polarity m_Polarity{Polarity::Negative};
    int m_Chamber{-1};
    int m_ID{-1};
    int m_Number{-1};
  };


  //
  class Channels
  {
  public:
    void insert(const int& id,const int& Number,const int& chamber,const Polarity& polarity)
    {
      Channels.emplace(id,Channel(id,Number,chamber,polarity));
    }
    std::size_t getNumberChannels()
    {
      return Channels.size();
    }
    const Analysis::Channel& getChannel(const std::size_t& id) const
    {
      return Channels.at(id);
    }

    const Analysis::Channel& getChannelByNumber(const std::size_t& num) const
    {
      std::map<int,Analysis::Channel>::const_iterator ret=Channels.end();
      for(std::map<int,Analysis::Channel>::const_iterator it= Channels.begin(); it!=Channels.end(); ++it)
      {
        if(it->second.getNumber()==num) ret= it;
      }
      return ret->second;
    }

    void print()
    {
      fmt::print("{} channels will be analysed :\n",Channels.size());
      for(std::map<int,Analysis::Channel>::iterator it= Channels.begin(); it!=Channels.end(); ++it)
      {
        fmt::print("\t * ID: {}, channel {}, with polarity {}, in chamber {}\n",it->second.getID(),it->second.getNumber(),it->second.getPolarity(),it->second.getOnChamber());
      }
    }
    bool hasToBeAnalysed(const int& ch)
    {
      if(Channels.find(ch) != Channels.end()) return true;
      else return false;
    }
    std::map<int,Analysis::Channel> get()
    {
      return Channels;
    }
    int getNumberChannelActivatedForChamber(const int& chamber)
    {
      int number{0};
      for(std::map<int,Analysis::Channel>::iterator it=Channels.begin();it!=Channels.end();++it)
      {
        if(it->second.getOnChamber()==chamber) number++;
      }
      return number;
    }
  private:
    //In file the channels in sequence 0...N
    std::map<int,Analysis::Channel> Channels;
  };

}


int NbrEventToProcess(int& nbrEvents, const Long64_t& nentries)
{
  if(nbrEvents == 0) return nentries;
  else if(nbrEvents > nentries)
  {
    std::cout << "WARNING : You ask to process " << nbrEvents << " but this run only have " << nentries << " !!!";
    return nbrEvents = nentries;
  }
  else return nbrEvents;
}


int findWichTrigger(const int& channel,const std::vector<int>& triggers)
{
  for(std::size_t i=0;i!=triggers.size();++i)
  {
    if(channel<triggers[i]) return triggers[i];
  }
  //Should never go there
  return -1;
}

TH1F CreateAndFillWaveform(const int& eventNbr, const Channel& channel, const std::string& name = "", const std::string title = "Signal;Time (ns);Signal (mV)")
{
  std::string my_name  = name + " channel " + std::to_string(int(channel.Group*8+channel.Number));
  TH1F th1(my_name.c_str(), title.c_str(), channel.Data.size(), 0, channel.Data.size());
  for(std::size_t i = 0; i != channel.Data.size(); ++i){th1.Fill(i, channel.Data[i]);}
  return std::move(th1);
}


void ToVolt(Channel& channel)
{
  float VPP{560}; //1.12VPP for 0-4096
  float DAC_TO_VOLT{VPP/2048};
  for(std::size_t j = 0; j != channel.Data.size(); ++j)
  {
    channel.Data[j] = (channel.Data[j] - 2048) * DAC_TO_VOLT;
  }
}

void SupressBaseLine(Channel& channel)
{

  double min{std::numeric_limits<double>::max()};
  double max{std::numeric_limits<double>::min()};
  double meanwindows{0};
  int bin{0};
  for(std::size_t j = 0; j != channel.Data.size(); ++j)
  {
      bin++;
      meanwindows += channel.Data[j];
  }
  meanwindows/=bin;
  for(std::size_t j = 0; j != channel.Data.size(); ++j)
  {
    channel.Data[j]-=meanwindows;
  }
}

std::pair<std::pair<double,int>,std::pair<double,int>> getMinMax(const Channel& channel,const int& begin=-1,const int& end=-1)
{
  int tick_max{0};
  int tick_min{0};
  double min{std::numeric_limits<double>::max()};
  double max{std::numeric_limits<double>::min()};
  std::size_t begin_{0};
  std::size_t end_{channel.Data.size()};
  if(begin>0) begin_=begin;
  if(end!=-1&&end<=channel.Data.size()) end_= end;
  for(std::size_t j = begin_; j != end_; ++j)
  {
    if(channel.Data[j]>max)
    {
      max=channel.Data[j];
      tick_max=j;
    }
    if(channel.Data[j]<min)
    {
      min=channel.Data[j];
      tick_min=j;
    }
  }
  return std::pair<std::pair<double,int>,std::pair<double,int>>(std::pair<double,int>(min,tick_min),std::pair<double,int>(max,tick_max));
}

double getAbsMax(const Channel& channel)
{
  double max{std::numeric_limits<double>::min()};
  for(std::size_t j = 0; j != channel.Data.size(); ++j)
  {
    if(std::fabs(channel.Data[j])>max) max=std::fabs(channel.Data[j]);
  }
  return max;
}

void Normalise(Channel& channel,const double& max)
{
  for(std::size_t j = 0; j != channel.Data.size(); ++j)
  {
    channel.Data[j]=channel.Data[j]/max;
  }
}

std::pair<std::pair<double, double>, std::pair<double, double>> MeanSTD(const Channel& channel, const std::pair<double, double>& window_signal = std::pair<double, double>{99999999, -999999},
                                                                        const std::pair<double, double>& window_noise = std::pair<double, double>{99999999, -999999})
{
  double meanwindows{0};
  double sigmawindows{0};
  int    binusedwindows{0};
  double meannoise{0};
  double sigmanoise{0};
  int    binusednoise{0};
  for(std::size_t j = 0; j != channel.Data.size(); ++j)
  {
    ////////////////////////////////
    if(j >= window_noise.first && j <= window_noise.second)
    {
      binusednoise++;
      meannoise += channel.Data[j];
    }
    if(j >= window_signal.first && j <= window_signal.second)
    {
      binusedwindows++;
      meanwindows += channel.Data[j];
    }
  }
  meannoise /= binusednoise;
  meanwindows /= binusedwindows;
  for(std::size_t j = 0; j != channel.Data.size(); ++j)
  {
    ////////////////////////////////
    if(j >= window_noise.first && j <= window_noise.second)
    {
      sigmanoise += (channel.Data[j] - meannoise) * (channel.Data[j] - meannoise);
    }
    if(j >= window_signal.first && j <= window_signal.second)
    {
      sigmawindows += (channel.Data[j] - meanwindows) * (channel.Data[j] - meanwindows);
    }
  }
  sigmanoise   = std::sqrt(sigmanoise / (binusednoise-1));
  sigmawindows = std::sqrt(sigmawindows / (binusedwindows-1));


  std::pair<double, double> noise(meannoise, sigmanoise);
  std::pair<double, double> signal(meanwindows, sigmawindows);
  return std::pair<std::pair<double, double>, std::pair<double, double>>(noise, signal);
}

class EventViewer
{
public:
  EventViewer(const std::string& name="",const std::string& title="",const bool& create=true) : m_Canvas(new TCanvas(create))
  {
    m_Canvas->SetName(name.c_str());
    m_Canvas->SetTitle(title.c_str());
    m_Canvas->SetFillStyle(4000);
    m_Canvas->SetCanvasSize(m_CanvasX,m_CanvasY);
    m_Canvas->SetWindowPosition(m_PositionCanvasX,m_PositionCanvasY);
    m_Canvas->cd();
    m_Pad = new TPad("","",0.01,0.01,0.99,0.95);
    m_PaveLabel = new TPaveLabel(0.01, 0.96, 0.99, 0.99,m_PaveTitle.c_str());
    m_PaveLabel->Draw();
    m_Pad->Draw();
  }
  ~EventViewer()=default;
  void setPaveLabel(const std::string& title)
  {
    m_PaveLabel->SetLabel(title.c_str());
    m_PaveLabel->Draw();
  }
  void setCanvasName(const std::string& title)
  {
    m_Canvas->SetName(title.c_str());
  }
  void setCanvasTitle(const std::string& title)
  {
    m_Canvas->SetTitle(title.c_str());
  }
  void divide(const int& div)
  {
    m_Pad->Divide(1,div,0,0);
  }
  void cd(const int& id)
  {
    m_Pad->cd(id);
  }
  void cdNext()
  {
    m_cd++;
    m_Pad->cd(m_cd);
  }
  void reset()
  {
    m_cd=0;
  }
  void saveAs(const std::string& filename)
  {
    m_Canvas->SaveAs(filename.c_str());
  }
  static void setPeriod(const double& period)
  {
    m_Period=period;
  }
  void createWaveForm(const Analysis::Channels& channels,const Channel& channel)
  {
    int number{channels.getChannelByNumber(channel.Group*8+channel.Number).getID()};
    std::cout<<"Creating "<<number<<std::endl;
    m_ChannelPlot[number] = TH1F(("Waveform_"+std::to_string(number)).c_str(),";Time (ns);Signal (mV)", channel.Data.size(), 0, channel.Data.size());
    m_ChannelPlot[number].Clear();
    for(std::size_t i = 0; i != channel.Data.size(); ++i) m_ChannelPlot[number].Fill(i, channel.Data[i]);
    m_ChannelPlot[number].SetLineColor(16);
    m_ChannelPlot[number].GetXaxis()->SetRangeUser(0, channel.Data.size());
    m_ChannelPlot[number].GetXaxis()->SetLimits(0.,channel.Data.size()*m_Period);
  }
  void UnderlineSignalRegion(const int& channel, const Color_t& color,const double& min,const double& max)
  {
    TH1F* h1c = static_cast<TH1F*>(m_ChannelPlot[channel].Clone());
    h1c->SetLineColor(color);
    h1c->GetXaxis()->SetRange(min,max);
    h1c->Draw("HISTsame");
    //delete h1c;
  }
  TH1F& getPlot(const int& channel)
  {
    return m_ChannelPlot[channel];
  }
private:
  TCanvas* m_Canvas{nullptr};
  TPad* m_Pad{nullptr};
  TPaveLabel* m_PaveLabel{nullptr};
  std::map<int,TH1F> m_ChannelPlot;
  std::string m_PaveTitle;
  static int m_CanvasX;
  static int m_CanvasY;
  static int m_PositionCanvasX;
  static int m_PositionCanvasY;
  static double m_Period;
  int m_cd{0};
};

double EventViewer::m_Period =1.0;
int EventViewer::m_CanvasX =1200;
int EventViewer::m_CanvasY =1500;
int EventViewer::m_PositionCanvasX =0;
int EventViewer::m_PositionCanvasY =0;

int GetTickTrigger(const Channel& channel,const double& percent,const Polarity& polarity)
{
  std::pair<std::pair<double,int>,std::pair<double,int>> min_max=getMinMax(channel);
  int value{0};
  for(std::size_t j = 0; j != channel.Data.size(); ++j)
  {
    if(polarity==Polarity::Positive && channel.Data[j] > percent*min_max.second.first)
    {
      value=j;
      break;
    }
    else if(polarity==Polarity::Negative && channel.Data[j] < percent*min_max.first.first)
    {
      value=j;
      break;
    }
  }
  return value;
}

int main(int argc, char** argv)
{
  SetStyle();
  gROOT->ForceStyle();
  ROOT::EnableThreadSafety();
  ROOT::EnableImplicitMT(5);
  std::istringstream Results;

  try
  {


  gErrorIgnoreLevel={kWarning};
  CLI::App    app{"Analysis"};

  std::string path{"./"};
  app.add_option("--path", path, "Path where to search the files.")->required()->check(CLI::ExistingPath);

  std::string save{"./Results"};
  app.add_option("--saveAs", save, "File to save the results.")->required();

  //Add CLI::ExistingFile
  std::vector<std::string> files;
  app.add_option("-f,--files", files, "Name of the file(s) to process.")->required();

  int NbrEvents{0};
  app.add_option("-e,--events", NbrEvents, "Number of event to process.")->check(CLI::PositiveNumber);

  std::string nameTree{"Tree"};
  app.add_option("-t,--tree", nameTree, "Name of the TTree.");

  std::pair<double, double> SignalWindow;
  app.add_option("-s,--signal", SignalWindow, "Width of the signal windows, delay between signal and trigger.")->required()->type_size(2);

  std::pair<double, double> NoiseWindow;
  app.add_option("-n,--noise", NoiseWindow, "Noise window.")->required()->type_size(2);

  std::pair<double, double> NoiseWindowAfter;
  app.add_option("--noiseAfter", NoiseWindowAfter, "Noise window after.")->required()->type_size(2);

  double NbrSigmaNoise{5.0};
  app.add_option("--sigmaNoise", NbrSigmaNoise, "NbrSigmaNoise.");

  int NumberChambers{0};
  app.add_option("-c,--chambers", NumberChambers, "Number of chamber(s).")->check(CLI::PositiveNumber)->required();

  //From MaftyNaveyuErin
  //By default we used only 8 channels so keep this behaviour
  std::vector<int> analysedchannels{0,1,2,3,4,5,6,7};
  app.add_option("--analysed", analysedchannels,"Channels want to be analysed.");

  //By defaut all 8 channels are in one chamber
  std::vector<int> distribution{0,0,0,0,0,0,0,0};
  app.add_option("-d,--distribution", distribution, "Channel is in wich chamber start at 0 and -1 if not connected.")->required();

  //By defaut all 8 channels have Negative polarity
  std::vector<int> polarity{-1,-1,-1,-1,-1,-1,-1,-1};
  app.add_option("-p,--polarity", polarity, "Polarity of the signal Positive,+,Negative,-.")->required();

  std::vector<int> triggers{8,17,26,35};
  app.add_option("--triggers", triggers, "Channels used as trigger 8,17,26,35 by default.");

  bool PlotTriggers{true};
  app.add_option("--plot_triggers", PlotTriggers, "Plot the triggers.");

  double NbrSigma{5.0};
  app.add_option("--sigma", NbrSigma, "Number of sigma above the mean noise.");

  bool dontPlotNoiseLines{false};
  app.add_option("--dontPlotNoiseLines", dontPlotNoiseLines,"Disable the Noise Lines on the plots.");

  bool dontPlotSignalLines{false};
  app.add_option("--dontPlotSignalLines", dontPlotSignalLines,"Disable the Signal Lines on the plots (mean and RMS).");

  bool plotIndividualChannels{false};
  app.add_option("--plotIndividualChannels", plotIndividualChannels,"Plot each channel individually.");

  double scalefactor{1.0};
  app.add_option("--scaleFactor", plotIndividualChannels,"Factor to divide the multiplicity.");

  try
  {
    app.parse(argc, argv);
  }
  catch(const CLI::ParseError& e)
  {
    return app.exit(e);
  }
  std::vector<std::string> line;
  /*std::string arguments{"#"};
   l ine.clear();             *                                                                                            *
   for(std::size_t i=0;i!=argc;++i)
   {
   arguments+=" ";
   arguments+=argv[i];
  }
  line.push_back(arguments);
  documents.SetRow(-1,line);*/

  line={"HV","Efficiency","Error Efficiency","Efficiency Corrected","Error Efficiency Corrected","Multiplicity"};
  std::vector<rapidcsv::Document> documents;
  std::vector<std::istringstream> Results;
  std::vector<int> Indexes;
  for(std::size_t i =0 ;i!=NumberChambers;++i)
  {
    Indexes.push_back(0);
    Results.push_back(std::istringstream());
    documents.push_back(rapidcsv::Document(Results[i],rapidcsv::LabelParams(0,-1)));
    documents[i].SetRow(-1,line);
  }

  std::size_t found = path.find_last_of("/\\");
  path = path.substr(0,found);
  std::vector<std::string> path_file;
  for(std::size_t i=0;i!=files.size();++i)
  {
    path_file.push_back(path+"/"+files[i]);
  }


  //Perform some check to avoid problem
  if(analysedchannels.size()==distribution.size() && distribution.size() == polarity.size());
  else throw std::runtime_error("analysed, distribution and polarity don't hve the smae number or argument !");

  Analysis::Channels channels;
  for(std::size_t ch=0; ch!=analysedchannels.size();++ch)
  {
    static int ID{0};
    //If channel is a trigger then add 1 (the user want the next channel not the trigger)
    if(std::find(triggers.begin(), triggers.end(), ch)!=triggers.end()) ID++;
    channels.insert(ID,ch,distribution[ch],static_cast<Polarity>(polarity[ch]));
    ID++;
  }

  channels.print();

  std::map<int,EventViewer> eventViewers;
  //Create the graph for chambers
  for(std::size_t i=0;i!=NumberChambers;++i)
  {
    eventViewers[i]=EventViewer();
    eventViewers[i].divide(channels.getNumberChannelActivatedForChamber(i));
  }

  TCanvas can2("","",0,0,800,600);
  for(std::size_t file=0;file!=path_file.size();++file)
  {
    TH1D total("Tick Distribution","Tick Distribution",1024,0,1024);
    TH1D delta_t("delta_T","delta_T",100,0,10);
    TH1D delta_T_not_event("delta_T_not_even","delta_T_not_even",100,0,10);
    TH1D delta_T_noisy("delta_T_noisy","delta_T_noisy",100,0,10);
    total.Clear();
    delta_t.Clear();
    delta_T_not_event.Clear();
    delta_T_noisy.Clear();

  //Open The file
    TFile fileIn(path_file[file].c_str());
  // Create Directory
    std::string folder{"Results/"+std::string(fs::path(path_file[file]).stem())};
  fs::create_directories(folder+"/Events");
  fs::create_directories(folder+"/Others");

  if(PlotTriggers) fs::create_directories(folder+"/Triggers");
  //Map for TGraph


  //Keep the ticks of each triggers
  std::map<int,int> trigger_ticks;
  std::map<int,TH1D> ticks_distribution;
  for(std::size_t i=0;i!=triggers.size();++i)
  {
    trigger_ticks[triggers[i]]=0;
    ticks_distribution[triggers[i]]=TH1D("Tick Distribution","Tick Distribution",1024,0,1024);
  }

  TTree* Run{nullptr};
  try
  {
    if(fileIn.IsZombie())
    {
      throw std::runtime_error(fmt::format("File {} Not Opened",path_file[file]));
    }
    Run = static_cast<TTree*>(fileIn.Get(nameTree.c_str()));
    if(Run == nullptr || Run->IsZombie())
    {
      throw std::runtime_error("Problem Opening TTree \"Tree\" !!!");
    }
  }
  catch(const std::runtime_error& error)
  {
    fmt::print(fg(fmt::color::red) | fmt::emphasis::bold,"{}\n",error.what());
    continue;
  }

  std::map<int,TH1D> mins;
  for(auto channel : channels.get())
  {
    mins[channel.first]=TH1D("min position distribution","min position distribution",1024,0,1024);
  }

  NbrEvents={NbrEventToProcess(NbrEvents,Run->GetEntries())};
  //channels.print();
  Event* event{nullptr};

  bool   hasseensomething{false};
  if(Run->SetBranchAddress("Events", &event))
  {
    throw std::runtime_error("Error while SetBranchAddress !!!");
  }

  // Initialize multiplicity
  std::vector<float> Multiplicity;
  std::vector<int> goodStack;
  std::vector<int> goodStackCorrected;
  std::vector<bool> goods;
  for(std::size_t i=0;i!=NumberChambers;++i)
  {
    Multiplicity.push_back(0.);
    goodStack.push_back(0.);
    goodStackCorrected.push_back(0.);
    goods.push_back(false);
  }


  int event_skip1{-1};
  int event_skip2{-1};
  int total_event{0};
  for(Long64_t evt = 0; evt < NbrEvents; ++evt)
  {


    std::map<int,std::pair<float,float>> MinMaxChamber;
    for(std::size_t i=0;i!=NumberChambers;++i)
    {
      MinMaxChamber[i]=std::pair<float,float>(std::numeric_limits<float>::max(),std::numeric_limits<float>::min());
    }

    for(std::map<int,EventViewer>::iterator it= eventViewers.begin(); it!=eventViewers.end();++it)
    {
      it->second.reset();
      it->second.setPaveLabel(fmt::format("Event {}",evt));
    }

    BoxedText(fg(fmt::color::orange) | fmt::emphasis::bold,fmt::format("Event {}",evt));

    event->clear();
    Run->GetEntry(evt);

    //std::vector<TH1F> Plots(event->Channels.size());
    float min{std::numeric_limits<float>::max()};
    float max{std::numeric_limits<float>::min()};
    // First loop on triggers
    for(unsigned int ch = 0; ch != event->Channels.size(); ++ch)
    {
      EventViewer::setPeriod(event->Period_ns);
      if(std::find(triggers.begin(),triggers.end(),ch)!=triggers.end())
      {
        //ToVolt(event->Channels[ch]); //CHANGE THIS
        SupressBaseLine(event->Channels[ch]);
        double max=getAbsMax(event->Channels[ch]);
        //Normalise(event->Channels[ch],max);
        int tick=GetTickTrigger(event->Channels[ch],0.20,Polarity::Negative);
        trigger_ticks[ch]=tick;
        /*if(PlotTriggers)
        {
          can.Clear();
          TH1F toto=CreateAndFillWaveform(evt, event->Channels[ch], "Waveform");
          toto.Draw("HIST");
          TLine event_min;
          event_min.SetLineColor(15);
          event_min.SetLineWidth(1);
          event_min.SetLineStyle(2);
          event_min.DrawLine(tick,-1.,tick,1.);
          event_min.DrawLine(0,-0.2,1024,-0.2);
          ticks_distribution[ch].Fill(tick);
          can.SaveAs((folder+"/Triggers"+"/Event"+std::to_string(evt)+"_Trigger"+std::to_string(ch)+".pdf").c_str(),"Q");
        }*/
      }
      else
      {
        if(!channels.hasToBeAnalysed(ch)) continue;  // Data for channel X is in file but i dont give a *** to analyse it !
        ToVolt(event->Channels[ch]);
        SupressBaseLine(event->Channels[ch]);
        std::pair<std::pair<double,int>,std::pair<double,int>> min_max_all=getMinMax(event->Channels[ch]);

        if(MinMaxChamber[channels.getChannel(ch).getOnChamber()].first>min_max_all.first.first) MinMaxChamber[channels.getChannel(ch).getOnChamber()].first = min_max_all.first.first;
        if(MinMaxChamber[channels.getChannel(ch).getOnChamber()].second<min_max_all.second.first) MinMaxChamber[channels.getChannel(ch).getOnChamber()].second=min_max_all.second.first;
      }
    }

    double delta_t_last{0};
    double delta_t_new{0};
    for(unsigned int ch = 0; ch != event->Channels.size(); ++ch)
    {
      if(!channels.hasToBeAnalysed(ch)) continue;  // Data for channel X is in file but i dont give a *** to analyse it !
      //ToVolt(event->Channels[ch]);
      eventViewers[channels.getChannel(ch).getOnChamber()].cdNext();


      if(ch==0)
      {
        delta_t_new = event->Channels[ch].TriggerTimeTag;
        if(evt!=0)
        {
          delta_t.Fill((delta_t_new-delta_t_last)*8.5e-9);
        }
      }

      BoxedText(fg(fmt::color::white) | fmt::emphasis::bold,fmt::format("Channel {}",channels.getChannel(ch).getNumber()));

      double max=getAbsMax(event->Channels[ch]);
      int realChannel=channels.getChannelByNumber(channels.getChannel(ch).getNumber()).getID();

      std::cout<<"************"<<ch<<"  "<<realChannel<<"***********";
      eventViewers[channels.getChannel(ch).getOnChamber()].createWaveForm(channels,event->Channels[ch]);
      double RangeUsermin{MinMaxChamber[channels.getChannel(ch).getOnChamber()].first*1.05};
      double RangeUsermax{MinMaxChamber[channels.getChannel(ch).getOnChamber()].second*1.05};
      eventViewers[channels.getChannel(ch).getOnChamber()].getPlot(realChannel).GetYaxis()->SetRangeUser(RangeUsermin,RangeUsermax);
      eventViewers[channels.getChannel(ch).getOnChamber()].getPlot(realChannel).Draw("HIST");

      ///BAD PLEASE FIX THIS !!!
      std::pair<int,int> SignalWindow2;
      SignalWindow2.first=trigger_ticks[findWichTrigger(ch,triggers)]-SignalWindow.second-SignalWindow.first/2;
      SignalWindow2.second=trigger_ticks[findWichTrigger(ch,triggers)]-SignalWindow.second+SignalWindow.first/2;
      std::pair<std::pair<double, double>, std::pair<double, double>> meanstd  = MeanSTD(event->Channels[ch], SignalWindow2, NoiseWindow);
      std::pair<std::pair<double, double>, std::pair<double, double>> meanstdAfter  = MeanSTD(event->Channels[ch], SignalWindow2, NoiseWindowAfter);

      if(meanstdAfter.first.second*1.0/meanstd.first.second >= NbrSigmaNoise)
      {
        event_skip1=evt;
        event_skip2=evt+1;
      }

      std::pair<std::pair<double,int>,std::pair<double,int>> min_max_all=getMinMax(event->Channels[ch]);
      mins[ch].Fill(trigger_ticks[findWichTrigger(ch,triggers)]-min_max_all.first.second);
      total.Fill(trigger_ticks[findWichTrigger(ch,triggers)]-min_max_all.first.second);

      std::pair<std::pair<double,int>,std::pair<double,int>> min_max=getMinMax(event->Channels[ch],SignalWindow2.first,SignalWindow2.second);
      // std::cout<<"Event "<<evt<<" Channel "<<ch<<"/n";
      // std::cout<<" Mean : "<<meanstd.first<<" STD :
      // "<<meanstd.second<<std::endl;
      // selected with be updated each time we make some selection... For now
      // it's the same as Waveform one but in Red !!!
      // TH1D selected = CreateSelectionPlot(waveform);
      float value;
      if(channels.getChannel(ch).getSignPolarity()==-1) value = min_max.first.first;
      else value = min_max.second.first;

      if(std::fabs(value-meanstd.second.first) > NbrSigma * meanstd.first.second) hasseensomething = true;
      else hasseensomething = false;


      eventViewers[channels.getChannel(ch).getOnChamber()].UnderlineSignalRegion(realChannel,hasseensomething ? 8 : 46,SignalWindow2.first,SignalWindow2.second);
      CenterXText(hasseensomething ? fg(fmt::color::green) : fg(fmt::color::red) | fmt::emphasis::bold,fmt::format("Mean signal region : {:05.4f}+-{:05.4f} min = {:05.4f}, Mean noise region : {:05.4f}+-{:05.4f}, Selection criteria {:05.4f} sigmas ({:05.4f}), Condition to fullfill {:05.4f}>{:05.4f}",meanstd.second.first,meanstd.second.second,min_max.first.first,meanstd.first.first,meanstd.first.second,NbrSigma,NbrSigma * meanstd.first.second,(min_max.first.first-meanstd.first.first)*channels.getChannel(ch).getSignPolarity(),NbrSigma * meanstd.first.second));
      if(hasseensomething == true)
      {
        goods[channels.getChannel(ch).getOnChamber()] =true;
        Multiplicity[channels.getChannel(ch).getOnChamber()]++;
      }

      TLine event_min;
      // Signal Region
      event_min.SetLineColor(30);
      event_min.SetLineWidth(2);
      event_min.SetLineStyle(10);
      event_min.DrawLine(SignalWindow2.first*event->Period_ns,RangeUsermin,SignalWindow2.first*event->Period_ns,RangeUsermax);
      event_min.DrawLine(SignalWindow2.second*event->Period_ns,RangeUsermin,SignalWindow2.second*event->Period_ns,RangeUsermax);

      if(!dontPlotNoiseLines)
      {
        // Noise before
        event_min.SetLineColor(42);
        event_min.SetLineWidth(2);
        event_min.SetLineStyle(10);
        event_min.DrawLine(NoiseWindow.first*event->Period_ns,RangeUsermin,NoiseWindow.first*event->Period_ns,RangeUsermax);
        event_min.DrawLine(NoiseWindow.second*event->Period_ns,RangeUsermin,NoiseWindow.second*event->Period_ns,RangeUsermax);

        // Noise After
        event_min.SetLineColor(42);
        event_min.SetLineWidth(2);
        event_min.SetLineStyle(10);
        event_min.DrawLine(NoiseWindowAfter.first*event->Period_ns,RangeUsermin,NoiseWindowAfter.first*event->Period_ns,RangeUsermax);
        event_min.DrawLine(NoiseWindowAfter.second*event->Period_ns,RangeUsermin,NoiseWindowAfter.second*event->Period_ns,RangeUsermax);

        //Mean noise before
        event_min.SetLineColor(45);
        event_min.SetLineWidth(2);
        event_min.SetLineStyle(9);
        event_min.DrawLine(NoiseWindow.first*event->Period_ns,meanstd.first.first,NoiseWindow.second*event->Period_ns,meanstd.first.first);

        // N*Sigma noise before
        event_min.SetLineColor(45);
        event_min.SetLineWidth(2);
        event_min.SetLineStyle(8);
        event_min.DrawLine(NoiseWindow.first*event->Period_ns,meanstd.first.first-NbrSigma * meanstd.first.second,NoiseWindow.second*event->Period_ns,meanstd.first.first-NbrSigma * meanstd.first.second);
        event_min.DrawLine(NoiseWindow.first*event->Period_ns,meanstd.first.first+NbrSigma * meanstd.first.second,NoiseWindow.second*event->Period_ns,meanstd.first.first+NbrSigma * meanstd.first.second);

        //Mean noise after
        event_min.SetLineColor(45);
        event_min.SetLineWidth(2);
        event_min.SetLineStyle(9);
        event_min.DrawLine(NoiseWindowAfter.first*event->Period_ns,meanstdAfter.first.first,NoiseWindowAfter.second*event->Period_ns,meanstdAfter.first.first);

        // N*Sigma noise after
        event_min.SetLineColor(45);
        event_min.SetLineWidth(2);
        event_min.SetLineStyle(8);
        event_min.DrawLine(NoiseWindowAfter.first*event->Period_ns,meanstdAfter.first.first-NbrSigmaNoise * meanstdAfter.first.second,NoiseWindowAfter.second*event->Period_ns,meanstdAfter.first.first-NbrSigmaNoise * meanstdAfter.first.second);
        event_min.DrawLine(NoiseWindowAfter.first*event->Period_ns,meanstdAfter.first.first+NbrSigmaNoise * meanstdAfter.first.second,NoiseWindowAfter.second*event->Period_ns,meanstdAfter.first.first+NbrSigmaNoise * meanstdAfter.first.second);
    }

      // Mean Signal
      event_min.SetLineColor(8);
      event_min.SetLineWidth(2);
      event_min.SetLineStyle(9);
      event_min.DrawLine(SignalWindow2.first*event->Period_ns,meanstd.second.first,SignalWindow2.second*event->Period_ns,meanstd.second.first);

      // Mean Signal -+ N*Sigma noise before
      event_min.SetLineColor(8);
      event_min.SetLineWidth(2);
      event_min.SetLineStyle(8);
      event_min.DrawLine(SignalWindow2.first*event->Period_ns, meanstd.second.first-NbrSigma * meanstd.first.second, SignalWindow2.second*event->Period_ns, meanstd.second.first-NbrSigma * meanstd.first.second);
      event_min.DrawLine(SignalWindow2.first*event->Period_ns, meanstd.second.first+NbrSigma * meanstd.first.second, SignalWindow2.second*event->Period_ns, meanstd.second.first+NbrSigma * meanstd.first.second);

      event_min.Draw();

      TGraph* gr = new TGraph(1);
      gr->SetPoint(0,min_max.first.second,min_max.first.first);
      gr->SetMarkerStyle(51);
      gr->Draw("PSAME");

      TArrow *ar3{nullptr};
      if(!dontPlotNoiseLines)
      {
        ar3 = new TArrow(NoiseWindow.first*event->Period_ns,meanstd.first.first,NoiseWindow.first*event->Period_ns,meanstd.first.first-NbrSigma * meanstd.first.second,0.005,"<|>");
        ar3->SetAngle(40);
        ar3->SetLineWidth(2);
        ar3->Draw();
      }

      TArrow *ar4 = new TArrow(SignalWindow2.first*event->Period_ns,meanstd.second.first,SignalWindow2.first*event->Period_ns,meanstd.second.first-NbrSigma * meanstd.first.second,0.005,"<|>");
      ar4->SetAngle(40);
      ar4->SetLineWidth(2);
      ar4->Draw();

      TLatex latex;
      latex.SetTextSize(0.02);
      latex.SetTextAlign(13);  //align at top
      latex.DrawLatex(SignalWindow2.first+1*event->Period_ns,meanstd.second.first-NbrSigma * meanstd.first.second/2,(fmt::format("{:02.1f}",NbrSigma)+"#times#sigma_{Noise}").c_str());

      TLatex latex2;
      latex2.SetTextSize(0.02);
      latex2.SetTextAlign(13);  //align at top
      latex2.DrawLatex(NoiseWindow.first+1*event->Period_ns,meanstdAfter.first.first-NbrSigma * meanstd.first.second/2,(fmt::format("{:02.1f}",NbrSigma)+"#times#sigma_{Noise}").c_str());

      if(plotIndividualChannels)
      {
        can2.cd();
        eventViewers[channels.getChannel(ch).getOnChamber()].getPlot(realChannel).Draw("HIST");
        eventViewers[channels.getChannel(ch).getOnChamber()].getPlot(realChannel).GetYaxis()->SetRangeUser(min_max_all.first.first*1.05,min_max_all.second.first*1.05);
        eventViewers[channels.getChannel(ch).getOnChamber()].UnderlineSignalRegion(realChannel,hasseensomething ? 8 : 46,SignalWindow2.first,SignalWindow2.second);

        TLine event_min;
        // Signal Region
        event_min.SetLineColor(46);//30
        event_min.SetLineWidth(2);
        event_min.SetLineStyle(10);
        event_min.DrawLine(SignalWindow2.first*event->Period_ns,RangeUsermin,SignalWindow2.first*event->Period_ns,RangeUsermax);
        event_min.DrawLine(SignalWindow2.second*event->Period_ns,RangeUsermin,SignalWindow2.second*event->Period_ns,RangeUsermax);

        if(!dontPlotSignalLines)
        {
          // Mean Signal
          event_min.SetLineColor(8);
          event_min.SetLineWidth(2);
          event_min.SetLineStyle(9);
          event_min.DrawLine(SignalWindow2.first*event->Period_ns,meanstd.second.first,SignalWindow2.second*event->Period_ns,meanstd.second.first);

          // Mean Signal -+ N*Sigma noise before
          event_min.SetLineColor(8);
          event_min.SetLineWidth(2);
          event_min.SetLineStyle(8);
          event_min.DrawLine(SignalWindow2.first*event->Period_ns, meanstd.second.first-NbrSigma * meanstd.first.second, SignalWindow2.second*event->Period_ns, meanstd.second.first-NbrSigma * meanstd.first.second);
          event_min.DrawLine(SignalWindow2.first*event->Period_ns, meanstd.second.first+NbrSigma * meanstd.first.second, SignalWindow2.second*event->Period_ns, meanstd.second.first+NbrSigma * meanstd.first.second);
          TLatex latex;
          latex.SetTextSize(0.02);
          latex.SetTextAlign(13);  //align at top
          latex.DrawLatex(SignalWindow2.first+5,meanstd.second.first-NbrSigma * meanstd.first.second/2,(fmt::format("{:02.1f}",NbrSigma)+"#times#sigma_{Noise}").c_str());
          ar4->Draw();
          gr->Draw("PSAME");
        }
        event_min.Draw();

        if(!dontPlotNoiseLines)
        {
          ar3->Draw();
        }
        std::string filename = folder+"/Events"+"/Event"+std::to_string(evt)+"chamber"+std::to_string(channels.getChannel(ch).getOnChamber())+"channel"+std::to_string(ch)+".png";
        can2.SaveAs(filename.c_str());
      }

    }

    for(std::map<int,EventViewer>::iterator it= eventViewers.begin(); it!=eventViewers.end();++it)
    {
      std::string filename = folder+"/Events"+"/Event"+std::to_string(evt)+"chamber"+std::to_string(it->first)+".png";
      it->second.saveAs(filename.c_str());
    }

    for(std::size_t nub =0 ;nub!=goods.size();++nub)
    {
      if(goods[nub] == true)
      {
        if(event_skip2!=evt)
        {
          goodStackCorrected[nub]++;

        }
        goodStack[nub]++;
        goods[nub] = false;
      }
      else
      {
        delta_T_not_event.Fill((delta_t_new-delta_t_last)*8.5e-9);
      }
    }


    if(event_skip2!=evt)
    {
      total_event++;
    }
    delta_t_new=delta_t_last;
    Clear();
  }
  for(std::map<int,TH1D>::iterator it= ticks_distribution.begin();it!= ticks_distribution.end();++it)
  {
    can2.Clear();
    it->second.Draw();
    can2.SaveAs((folder+"/Others"+"/Tick_Distribution_"+std::to_string(it->first)+".pdf").c_str(),"Q");
  }
  for(auto min : mins)
  {
    can2.Clear();
    min.second.GetXaxis()->SetNdivisions(510);
    min.second.Draw();
    can2.SaveAs((folder+"/Others"+"/minimum_position_distribution"+std::to_string(min.first)+".pdf").c_str(),"Q");
  }
  can2.Clear();
  total.GetXaxis()->SetNdivisions(510);
  total.Draw();
  can2.SaveAs((folder+"/Others"+"/minimum_position_distribution_total.pdf").c_str(),"Q");

  can2.Clear();
  delta_t.GetXaxis()->SetNdivisions(510);
  delta_t.Draw();
  can2.SaveAs((folder+"/DeltaT.pdf").c_str(),"Q");

  can2.Clear();
  delta_T_not_event.GetXaxis()->SetNdivisions(510);
  delta_T_not_event.Draw();
  can2.SaveAs((folder+"/delta_T_not_event.pdf").c_str(),"Q");

  can2.Clear();
  delta_T_noisy.GetXaxis()->SetNdivisions(510);
  delta_T_noisy.Draw();
  can2.SaveAs((folder+"/delta_T_noisy.pdf").c_str(),"Q");


  for(std::size_t chamber = 0; chamber != NumberChambers ; ++chamber)
  {
    float efficiency=goodStack[chamber] * 1.00 / (NbrEvents * scalefactor);
    float efficiency_corrected=goodStackCorrected[chamber] * 1.00 / (total_event * scalefactor);
    std::cout << "Chamber efficiency " << efficiency << " +-" <<std::sqrt(efficiency*(1-efficiency)/NbrEvents)<<" with signal "<<goodStack[chamber]<<" total event "<< NbrEvents<<" Multiplicity"<< Multiplicity[chamber]/goodStack[chamber]<< std::endl;
    std::cout << "Chamber efficiency corrected " << efficiency_corrected << " +-" <<std::sqrt(efficiency_corrected*(1-efficiency_corrected)/total_event)<<" with signal "<<goodStackCorrected[chamber]<<" total event "<< total_event <<std::endl;

    std::cout<< "Number event analysed " << total_event*100.0/NbrEvents <<std::endl;

    std::vector<float> line;
    std::string value;
    std::size_t found = files[file].find("V.root");
    std::string HV = files[file].substr(0,found);
    line.push_back(std::stof(HV));
    line.push_back(efficiency);
    line.push_back(std::sqrt(efficiency*(1-efficiency)/NbrEvents));
    line.push_back(efficiency_corrected);
    line.push_back(std::sqrt(efficiency_corrected*(1-efficiency_corrected)/total_event));
    line.push_back(Multiplicity[chamber]/goodStack[chamber]);

    documents[chamber].SetRow(Indexes[chamber],line);
    Indexes[chamber]++;
  }
  if(event != nullptr) delete event;
  if(Run != nullptr) delete Run;
  if(fileIn.IsOpen()) fileIn.Close();


  for(std::size_t document=0; document!=documents.size();++document)
  {
    documents[document].Save((save+"_Chamber"+std::to_string(document)+".csv").c_str());
  }

  }
  }
  catch(const std::exception& e)
  {
    std::cout<<e.what()<<std::endl;
  }

}
