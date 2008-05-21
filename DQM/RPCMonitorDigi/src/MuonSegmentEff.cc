// -*- C++ -*-
//
// Package:    MuonSegmentEff
// Class:      MuonSegmentEff
// 
/**\class MuonSegmentEff MuonSegmentEff.cc dtcscrpc/MuonSegmentEff/src/MuonSegmentEff.cc

 Description: <one line class summary>

 Implementation:
     <Notes on implementation>
*/
//
// Original Author:  Camilo Carrillo (Uniandes)
//         Created:  Tue Oct  2 16:57:49 CEST 2007
// $Id: MuonSegmentEff.cc,v 1.21 2008/05/19 17:42:07 carrillo Exp $
//
//

#include "DQM/RPCMonitorDigi/interface/MuonSegmentEff.h"

// system include files
#include <memory>

// user include files

#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/Framework/interface/ESHandle.h"

#include <DataFormats/RPCDigi/interface/RPCDigiCollection.h>

#include <Geometry/RPCGeometry/interface/RPCGeometry.h>
#include <Geometry/RPCGeometry/interface/RPCGeomServ.h>
#include <Geometry/DTGeometry/interface/DTGeometry.h>
#include <Geometry/CSCGeometry/interface/CSCGeometry.h>

#include <DataFormats/DTRecHit/interface/DTRecSegment4DCollection.h>
#include <DataFormats/CSCRecHit/interface/CSCSegmentCollection.h>

#include <Geometry/CommonDetUnit/interface/GeomDet.h>
#include <Geometry/Records/interface/MuonGeometryRecord.h>
#include <Geometry/CommonTopologies/interface/RectangularStripTopology.h>
#include <Geometry/CommonTopologies/interface/TrapezoidalStripTopology.h>

#include <cmath>
#include "TFile.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TCanvas.h"
#include "TAxis.h"





class DTStationIndex{
public: 
  DTStationIndex():_region(0),_wheel(0),_sector(0),_station(0){}
  DTStationIndex(int region, int wheel, int sector, int station) : 
    _region(region),
    _wheel(wheel),
    _sector(sector),
    _station(station){}
  ~DTStationIndex(){}
  int region() const {return _region;}
  int wheel() const {return _wheel;}
  int sector() const {return _sector;}
  int station() const {return _station;}
  bool operator<(const DTStationIndex& dtind) const{
    if(dtind.region()!=this->region())
      return dtind.region()<this->region();
    else if(dtind.wheel()!=this->wheel())
      return dtind.wheel()<this->wheel();
    else if(dtind.sector()!=this->sector())
      return dtind.sector()<this->sector();
    else if(dtind.station()!=this->station())
      return dtind.station()<this->station();
    return false;
  }
private:
  int _region;
  int _wheel;
  int _sector;
  int _station; 
};


class CSCStationIndex{
public:
  CSCStationIndex():_region(0),_station(0),_ring(0),_chamber(0){}
  CSCStationIndex(int region, int station, int ring, int chamber):
    _region(region),
    _station(station),
    _ring(ring),
    _chamber(chamber){}
  ~CSCStationIndex(){}
  int region() const {return _region;}
  int station() const {return _station;}
  int ring() const {return _ring;}
  int chamber() const {return _chamber;}
  bool operator<(const CSCStationIndex& cscind) const{
    if(cscind.region()!=this->region())
      return cscind.region()<this->region();
    else if(cscind.station()!=this->station())
      return cscind.station()<this->station();
    else if(cscind.ring()!=this->ring())
      return cscind.ring()<this->ring();
    else if(cscind.chamber()!=this->chamber())
      return cscind.chamber()<this->chamber();
    return false;
  }

private:
  int _region;
  int _station;
  int _ring;  
  int _chamber;
};


void MuonSegmentEff::beginJob(const edm::EventSetup&)
{

}




MuonSegmentEff::MuonSegmentEff(const edm::ParameterSet& iConfig)
{

  std::map<RPCDetId, int> buff;
  counter.clear();
  counter.reserve(3);
  counter.push_back(buff);
  counter.push_back(buff);
  counter.push_back(buff);
  totalcounter.clear();
  totalcounter.reserve(3);
  totalcounter[0]=0;
  totalcounter[1]=0;
  totalcounter[2]=0;

  incldt=iConfig.getUntrackedParameter<bool>("incldt",true);
  incldtMB4=iConfig.getUntrackedParameter<bool>("incldtMB4",true);
  inclcsc=iConfig.getUntrackedParameter<bool>("inclcsc",true);
  MinimalResidual= iConfig.getUntrackedParameter<double>("MinimalResidual",2.);
  widestripRB4=iConfig.getUntrackedParameter<double>("widestripRB4",4.);
  MinCosAng=iConfig.getUntrackedParameter<double>("MinCosAng",0.9999);
  MaxD=iConfig.getUntrackedParameter<double>("MaxD",20.);
  muonRPCDigis=iConfig.getUntrackedParameter<std::string>("muonRPCDigis","muonRPCDigis");
  cscSegments=iConfig.getUntrackedParameter<std::string>("cscSegments","cscSegments");
  dt4DSegments=iConfig.getUntrackedParameter<std::string>("dt4DSegments","dt4DSegments");
  rejected=iConfig.getUntrackedParameter<std::string>("rejected","rejected.txt");
  rollseff=iConfig.getUntrackedParameter<std::string>("rollseff","rollseff.txt");
  GlobalRootLabel= iConfig.getUntrackedParameter<std::string>("GlobalRootFileName","GlobalEfficiencyFromTrack.root");
  wh  = iConfig.getUntrackedParameter<int>("wheel",0);


  std::cout<<rejected<<std::endl;
  std::cout<<rollseff<<std::endl;
  
  ofrej.open(rejected.c_str());
  ofeff.open(rollseff.c_str());
  oftwiki.open("tabletotwiki.txt");

  // Giuseppe
  nameInLog = iConfig.getUntrackedParameter<std::string>("moduleLogName", "RPC_Eff");
  EffSaveRootFile  = iConfig.getUntrackedParameter<bool>("EffSaveRootFile", true); 
  EffSaveRootFileEventsInterval  = iConfig.getUntrackedParameter<int>("EffEventsInterval", 10000); 
  EffRootFileName  = iConfig.getUntrackedParameter<std::string>("EffRootFileName", "CMSRPCEff.root"); 
  //Interface
  dbe = edm::Service<DQMStore>().operator->();
  _idList.clear(); 

  //GLOBAL
  fOutputFile  = new TFile(GlobalRootLabel.c_str(), "RECREATE" );

  hGlobalRes = new TH1F("GlobalResiduals","Global RPC Residuals",1000,-10.,10.);
  hGlobalResY = new TH1F("GlobalResidualsY","Global RPC Residuals Y",500,-100.,100);
  
  EffGlob1 = new TH1F("GlobEfficiencySec1","Eff. vs. roll",20,0.5,20.5);
  EffGlob2 = new TH1F("GlobEfficiencySec2","Eff. vs. roll",20,0.5,20.5);
  EffGlob3 = new TH1F("GlobEfficiencySec3","Eff. vs. roll",20,0.5,20.5);
  EffGlob4 = new TH1F("GlobEfficiencySec4","Eff. vs. roll",20,0.5,20.5);
  EffGlob5 = new TH1F("GlobEfficiencySec5","Eff. vs. roll",20,0.5,20.5);
  EffGlob6 = new TH1F("GlobEfficiencySec6","Eff. vs. roll",20,0.5,20.5);
  EffGlob7 = new TH1F("GlobEfficiencySec7","Eff. vs. roll",20,0.5,20.5);
  EffGlob8 = new TH1F("GlobEfficiencySec8","Eff. vs. roll",20,0.5,20.5);
  EffGlob9 = new TH1F("GlobEfficiencySec9","Eff. vs. roll",20,0.5,20.5);
  EffGlob10 = new TH1F("GlobEfficiencySec10","Eff. vs. roll",20,0.5,20.5);
  EffGlob11 = new TH1F("GlobEfficiencySec11","Eff. vs. roll",20,0.5,20.5);
  EffGlob12 = new TH1F("GlobEfficiencySec12","Eff. vs. roll",20,0.5,20.5);

}


MuonSegmentEff::~MuonSegmentEff()
{
  // effres->close();
  //delete effres;
  
  fOutputFile->WriteTObject(hGlobalRes);
  fOutputFile->WriteTObject(hGlobalResY);
  
  fOutputFile->WriteTObject(EffGlob1);
  fOutputFile->WriteTObject(EffGlob2);
  fOutputFile->WriteTObject(EffGlob3);
  fOutputFile->WriteTObject(EffGlob4);
  fOutputFile->WriteTObject(EffGlob5);
  fOutputFile->WriteTObject(EffGlob6);
  fOutputFile->WriteTObject(EffGlob7);
  fOutputFile->WriteTObject(EffGlob8);
  fOutputFile->WriteTObject(EffGlob9);
  fOutputFile->WriteTObject(EffGlob10);
  fOutputFile->WriteTObject(EffGlob11);
  fOutputFile->WriteTObject(EffGlob12);

  fOutputFile->Close();
  edm::LogInfo (nameInLog) <<"Beginning DQMMonitorDigi " ;
  if(EffSaveRootFile) dbe->showDirStructure();
}



void MuonSegmentEff::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup)
{
  using namespace edm;
  
  std::map<RPCDetId, int> buff;

  char layerLabel[128];
  char meIdRPC [128];
  char meIdDT [128];
  char meRPC [128];
  char meIdCSC [128];

  std::cout<<"New Event "<<iEvent.id().event()<<std::endl;

  std::cout <<"\t Getting the RPC Geometry"<<std::endl;
  edm::ESHandle<RPCGeometry> rpcGeo;
  iSetup.get<MuonGeometryRecord>().get(rpcGeo);
  
  std::cout <<"\t Getting the RPC Digis"<<std::endl;
  edm::Handle<RPCDigiCollection> rpcDigis;
  iEvent.getByLabel(muonRPCDigis, rpcDigis);
  
  std::map<DTStationIndex,std::set<RPCDetId> > rollstoreDT;
  std::map<CSCStationIndex,std::set<RPCDetId> > rollstoreCSC;    
  
  for (TrackingGeometry::DetContainer::const_iterator it=rpcGeo->dets().begin();it<rpcGeo->dets().end();it++){
    if( dynamic_cast< RPCChamber* >( *it ) != 0 ){
      RPCChamber* ch = dynamic_cast< RPCChamber* >( *it ); 
      std::vector< const RPCRoll*> roles = (ch->rolls());
      for(std::vector<const RPCRoll*>::const_iterator r = roles.begin();r != roles.end(); ++r){
	RPCDetId rpcId = (*r)->id();
	int region=rpcId.region();
	
	if(region==0&&(incldt||incldtMB4)){
	  //std::cout<<"--Filling the barrel"<<rpcId<<std::endl;
	  int wheel=rpcId.ring();
	  int sector=rpcId.sector();
	  int station=rpcId.station();
	  DTStationIndex ind(region,wheel,sector,station);
	  std::set<RPCDetId> myrolls;
	  if (rollstoreDT.find(ind)!=rollstoreDT.end()) myrolls=rollstoreDT[ind];
	  myrolls.insert(rpcId);
	  rollstoreDT[ind]=myrolls;
	}
	else if(inclcsc){
	  //std::cout<<"--Filling the EndCaps!"<<rpcId<<std::endl;
	  int region=rpcId.region();
          int station=rpcId.station();
          int ring=rpcId.ring();
          int cscring=ring;
          int cscstation=station;
	  RPCGeomServ rpcsrv(rpcId);
	  int rpcsegment = rpcsrv.segment();
	  int cscchamber = rpcsegment;
          if((station==2||station==3)&&ring==3){//Adding Ring 3 of RPC to the CSC Ring 2
            cscring = 2;
          }
	  if((station==4)&&(ring==2||ring==3)){//RE4 have just ring 1
            cscstation=3;
            cscring=2;
          }
          CSCStationIndex ind(region,cscstation,cscring,cscchamber);
          std::set<RPCDetId> myrolls;
	  if (rollstoreCSC.find(ind)!=rollstoreCSC.end()){
            myrolls=rollstoreCSC[ind];
          }
          
          myrolls.insert(rpcId);
          rollstoreCSC[ind]=myrolls;
        }
      }
    }
  }
  

  if(incldt){
#include "dtpart.inl"
  }
  
  if(incldtMB4){
#include "rb4part.inl"
  }
  
  if(inclcsc){
#include "cscpart.inl"
  }
}


void 
MuonSegmentEff::endJob() {
  
  int index1=0,index2=0,index3=0,index4=0,index5=0,index6=0,index7=0,index8=0,index9=0,index10=0,index11=0,index12=0;

  std::map<RPCDetId, int> pred = counter[0];
  std::map<RPCDetId, int> obse = counter[1];
  std::map<RPCDetId, int> reje = counter[2];
  std::map<RPCDetId, int>::iterator irpc;

  oftwiki <<"|  RPC Name  |  Observed  |  Predicted  |  Efficiency %  |  Error %  |";
     
  for (irpc=pred.begin(); irpc!=pred.end();irpc++){
    RPCDetId id=irpc->first;
    
    RPCGeomServ RPCname(id);
    std::string nameRoll = RPCname.name();
    std::string wheel;
    std::string rpc;
    std::string partition;
    
    int p=pred[id]; 
    int o=obse[id]; 
    int r=reje[id]; 
    assert(p==o+r);
    
    //-----------------------Fillin Global Histogram----------------------------------------

    
    if(p!=0 && id.ring()==wh){
      float ef = float(o)/float(p); 
      float er = sqrt(ef*(1.-ef)/float(p));
      if(ef>0.){
	if(id.sector()==1  ){
	  index1++;
	  char cam[128];	
	  sprintf(cam,"%s",nameRoll.c_str());
	  TString camera = (TString)cam;
	  
	  EffGlob1->SetBinContent(index1,ef*100.);
	  EffGlob1->SetBinError(index1,er*100.);
	  
	  EffGlob1->GetXaxis()->SetBinLabel(index1,camera);
	  EffGlob1->GetXaxis()->LabelsOption("v");
	}
	if(id.sector()==2  ){
	  index2++;
	  char cam[128];	
	  sprintf(cam,"%s",nameRoll.c_str());
	  TString camera = (TString)cam;
	  
	  EffGlob2->SetBinContent(index2,ef*100.);
	  EffGlob2->SetBinError(index2,er*100.);
	  
	  EffGlob2->GetXaxis()->SetBinLabel(index2,camera);
	  EffGlob2->GetXaxis()->LabelsOption("v");
	}
	if(id.sector()==3  ){
	  index3++;
	  char cam[128];	
	  sprintf(cam,"%s",nameRoll.c_str());
	  TString camera = (TString)cam;
	  
	  EffGlob3->SetBinContent(index3,ef*100.);
	  EffGlob3->SetBinError(index3,er*100.);
	  
	  EffGlob3->GetXaxis()->SetBinLabel(index3,camera);
	  EffGlob3->GetXaxis()->LabelsOption("v");
	}
	if(id.sector()==4  ){
	  index4++;
	  char cam[128];	
	  sprintf(cam,"%s",nameRoll.c_str());
	  TString camera = (TString)cam;
	  
	  EffGlob4->SetBinContent(index4,ef*100.);
	  EffGlob4->SetBinError(index4,er*100.);
	  
	  EffGlob4->GetXaxis()->SetBinLabel(index4,camera);
	  EffGlob4->GetXaxis()->LabelsOption("v");
	}
	if(id.sector()==5  ){
	  index5++;
	  char cam[128];	
	  sprintf(cam,"%s",nameRoll.c_str());
	  TString camera = (TString)cam;
	  
	  EffGlob5->SetBinContent(index5,ef*100.);
	  EffGlob5->SetBinError(index5,er*100.);
	  
	  EffGlob5->GetXaxis()->SetBinLabel(index5,camera);
	  EffGlob5->GetXaxis()->LabelsOption("v");
	}
	if(id.sector()==6  ){
	  index6++;
	  char cam[128];	
	  sprintf(cam,"%s",nameRoll.c_str());
	  TString camera = (TString)cam;
	  
	  EffGlob6->SetBinContent(index6,ef*100.);
	  EffGlob6->SetBinError(index6,er*100.);
	  
	  EffGlob6->GetXaxis()->SetBinLabel(index6,camera);
	  EffGlob6->GetXaxis()->LabelsOption("v");
	}
	if(id.sector()==7  ){
	  index7++;
	  char cam[128];	
	  sprintf(cam,"%s",nameRoll.c_str());
	  TString camera = (TString)cam;
	  
	  EffGlob7->SetBinContent(index7,ef*100.);
	  EffGlob7->SetBinError(index7,er*100.);
	  
	  EffGlob7->GetXaxis()->SetBinLabel(index7,camera);
	  EffGlob7->GetXaxis()->LabelsOption("v");
	}
	if(id.sector()==8  ){
	  index8++;
	  char cam[128];	
	  sprintf(cam,"%s",nameRoll.c_str());
	  TString camera = (TString)cam;
	  
	  EffGlob8->SetBinContent(index8,ef*100.);
	  EffGlob8->SetBinError(index8,er*100.);
	  
	  EffGlob8->GetXaxis()->SetBinLabel(index8,camera);
	  EffGlob8->GetXaxis()->LabelsOption("v");
	}
	if(id.sector()==9  ){
	  index9++;
	  char cam[128];	
	  sprintf(cam,"%s",nameRoll.c_str());
	  TString camera = (TString)cam;
	  
	  EffGlob9->SetBinContent(index9,ef*100.);
	  EffGlob9->SetBinError(index9,er*100.);
	  
	  EffGlob9->GetXaxis()->SetBinLabel(index9,camera);
	  EffGlob9->GetXaxis()->LabelsOption("v");
	}
	if(id.sector()==10  ){
	  index10++;
	  char cam[128];	
	  sprintf(cam,"%s",nameRoll.c_str());
	  TString camera = (TString)cam;
	  
	  EffGlob10->SetBinContent(index10,ef*100.);
	  EffGlob10->SetBinError(index10,er*100.);
	  
	  EffGlob10->GetXaxis()->SetBinLabel(index10,camera);
	  EffGlob10->GetXaxis()->LabelsOption("v");
	}
	if(id.sector()==11  ){
	  index11++;
	  char cam[128];	
	  sprintf(cam,"%s",nameRoll.c_str());
	  TString camera = (TString)cam;
	  
	  EffGlob11->SetBinContent(index11,ef*100.);
	  EffGlob11->SetBinError(index11,er*100.);
	  
	  EffGlob11->GetXaxis()->SetBinLabel(index11,camera);
	  EffGlob11->GetXaxis()->LabelsOption("v");
	}
	if(id.sector()==12  ){
	  index12++;
	  char cam[128];	
	  sprintf(cam,"%s",nameRoll.c_str());
	  TString camera = (TString)cam;
	  
	  EffGlob12->SetBinContent(index12,ef*100.);
	  EffGlob12->SetBinError(index12,er*100.);
	  
	  EffGlob12->GetXaxis()->SetBinLabel(index12,camera);
	  EffGlob12->GetXaxis()->LabelsOption("v");
	}

	//histoMean->Fill(ef*100.);
      }
    }
   
    //-----------------------Fillin Global Histogram----------------------------------------

    if(p!=0){
      float ef = float(o)/float(p); 
      float er = sqrt(ef*(1.-ef)/float(p));
      std::cout <<"\n "<<id<<"\t Predicted "<<p<<"\t Observed "<<o<<"\t Eff = "<<ef*100.<<" % +/- "<<er*100.<<" %";
      ofeff <<"\n "<<id<<"\t Predicted "<<p<<"\t Observed "<<o<<"\t Eff = "<<ef*100.<<" % +/- "<<er*100.<<" %";
      RPCGeomServ RPCname(id);
      oftwiki <<"\n |  "<<RPCname.name()<<"  |  "<<o<<"  |  "<<p<<"  |  "<<ef*100.<<"  |  "<<er*100<<"  |";
      //if(ef<0.8){
      //std::cout<<"\t \t Warning!";
      //ofeff<<"\t \t Warning!";
      //} 
    }
    else{
      std::cout<<"No predictions in this file p=0"<<std::endl;
      ofeff<<"No predictions in this file p=0"<<std::endl;
    }
  }
  if(totalcounter[0]!=0){
    float tote = float(totalcounter[1])/float(totalcounter[0]);
    float totr = sqrt(tote*(1.-tote)/float(totalcounter[0]));
  
    std::cout <<"\n\n \t \t TOTAL EFFICIENCY \t Predicted "<<totalcounter[0]<<"\t Observed "<<totalcounter[1]<<"\t Eff = "<<tote*100.<<"\t +/- \t"<<totr*100.<<" %"<<std::endl;
    std::cout <<totalcounter[1]<<" "<<totalcounter[0]<<" flagcode"<<std::endl;
    
    ofeff <<"\n\n \t \t TOTAL EFFICIENCY \t Predicted "<<totalcounter[0]<<"\t Observed "<<totalcounter[1]<<"\t Eff = "<<tote*100.<<"\t +/- \t"<<totr*100.<<" %"<<std::endl;
    ofeff <<totalcounter[1]<<" "<<totalcounter[0]<<" flagcode"<<std::endl;

  }
  else{
    std::cout<<"No predictions in this file = 0!!!"<<std::endl;
    ofeff <<"No predictions in this file = 0!!!"<<std::endl;
  }

  std::vector<std::string>::iterator meIt;
  int k = 0;

  if(EffSaveRootFile==true){
    for(meIt = _idList.begin(); meIt != _idList.end(); ++meIt){
      k++;

      char detUnitLabel[128];

      char meIdRPC [128];
      char meIdDT [128];
      char effIdRPC_DT [128];

      char meIdRPC_2D [128];
      char meIdDT_2D [128];
      char effIdRPC_DT_2D [128];

    
      sprintf(detUnitLabel ,"%s",(*meIt).c_str());
    
      std::cout<<"Creating Efficiency Root File #"<<k<<std::endl;

      sprintf(meIdRPC,"RPCDataOccupancyFromDT_%s",detUnitLabel);
      sprintf(meIdRPC_2D,"RPCDataOccupancy2DFromDT_%s",detUnitLabel);

      sprintf(meIdDT,"ExpectedOccupancyFromDT_%s",detUnitLabel);
      sprintf(meIdDT_2D,"ExpectedOccupancy2DFromDT_%s",detUnitLabel);

      sprintf(effIdRPC_DT,"EfficienyFromDTExtrapolation_%s",detUnitLabel);
      sprintf(effIdRPC_DT_2D,"EfficienyFromDT2DExtrapolation_%s",detUnitLabel);

      std::map<std::string, MonitorElement*> meMap=meCollection[*meIt];

      for(unsigned int i=1;i<=100;++i){
	if(meMap[meIdDT]->getBinContent(i) != 0){
	  float eff = meMap[meIdRPC]->getBinContent(i)/meMap[meIdDT]->getBinContent(i);
	  float erreff = sqrt(eff*(1-eff)/meMap[meIdDT]->getBinContent(i));
	  meMap[effIdRPC_DT]->setBinContent(i,eff*100.);
	  meMap[effIdRPC_DT]->setBinError(i,erreff*100.);
	}
      }
      for(unsigned int i=1;i<=100;++i){
	for(unsigned int j=1;j<=200;++j){
	  if(meMap[meIdDT_2D]->getBinContent(i,j) != 0){
	    float eff = meMap[meIdRPC_2D]->getBinContent(i,j)/meMap[meIdDT_2D]->getBinContent(i,j);
	    float erreff = sqrt(eff*(1-eff)/meMap[meIdDT_2D]->getBinContent(i,j));
	    meMap[effIdRPC_DT_2D]->setBinContent(i,j,eff*100.);
	    meMap[effIdRPC_DT_2D]->setBinError(i,j,erreff*100.);
	  }
	}
      }
      ///CSC

    
      char meRPC [128];
      char meIdCSC [128];
      char effIdRPC_CSC [128];

      char meRPC_2D [128];
      char meIdCSC_2D [128];
      char effIdRPC_CSC_2D [128];

      sprintf(detUnitLabel ,"%s",(*meIt).c_str());

      sprintf(meRPC,"RPCDataOccupancyFromCSC_%s",detUnitLabel);
      sprintf(meRPC_2D,"RPCDataOccupancy2DFromCSC_%s",detUnitLabel);

      sprintf(meIdCSC,"ExpectedOccupancyFromCSC_%s",detUnitLabel);
      sprintf(meIdCSC_2D,"ExpectedOccupancy2DFromCSC_%s",detUnitLabel);

      sprintf(effIdRPC_CSC,"EfficienyFromCSCExtrapolation_%s",detUnitLabel);
      sprintf(effIdRPC_CSC_2D,"EfficienyFromCSC2DExtrapolation_%s",detUnitLabel);

      //std::map<std::string, MonitorElement*> meMap=meCollection[*meIt];

      for(unsigned int i=1;i<=100;++i){
      
	if(meMap[meIdCSC]->getBinContent(i) != 0){
	  float eff = meMap[meRPC]->getBinContent(i)/meMap[meIdCSC]->getBinContent(i);
	  float erreff = sqrt(eff*(1-eff)/meMap[meIdCSC]->getBinContent(i));
	  meMap[effIdRPC_CSC]->setBinContent(i,eff*100.);
	  meMap[effIdRPC_CSC]->setBinError(i,erreff*100.);
	}
      }
      for(unsigned int i=1;i<=100;++i){
	for(unsigned int j=1;j<=200;++j){
	  if(meMap[meIdCSC_2D]->getBinContent(i,j) != 0){
	    float eff = meMap[meRPC_2D]->getBinContent(i,j)/meMap[meIdCSC_2D]->getBinContent(i,j);
	    float erreff = sqrt(eff*(1-eff)/meMap[meIdCSC_2D]->getBinContent(i,j));
	    meMap[effIdRPC_CSC_2D]->setBinContent(i,j,eff*100.);
	    meMap[effIdRPC_CSC_2D]->setBinError(i,j,erreff*100.);
	  }
	}
      }
    }
  
    //Giuseppe
  
    dbe->save(EffRootFileName);
    std::cout<<"Saving RootFile"<<std::endl;
  }  
  ofeff.close();
  oftwiki.close();
  ofrej.close();
}

