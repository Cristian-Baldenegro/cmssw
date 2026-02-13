#ifndef MNguyen_HiInclusiveGenJetAnalyzer_inclusiveJetAnalyzer_
#define MNguyen_HiInclusiveGenJetAnalyzer_inclusiveJetAnalyzer_

// system include files
#include <memory>
#include <string>
#include <iostream>

// ROOT headers
#include "TTree.h"

// user include files
#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "DataFormats/PatCandidates/interface/Jet.h"
#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "SimDataFormats/GeneratorProducts/interface/GenEventInfoProduct.h"
#include "SimDataFormats/GeneratorProducts/interface/HepMCProduct.h"
#include "DataFormats/PatCandidates/interface/PackedCandidate.h"
#include "DataFormats/PatCandidates/interface/PackedGenParticle.h"
#include "fastjet/contrib/Njettiness.hh"
#include "AnalysisDataFormats/TrackInfo/interface/TrackToGenParticleMap.h"
#include "DataFormats/PatCandidates/interface/PackedGenParticle.h"

#include "SimDataFormats/JetMatching/interface/JetFlavourInfo.h"
#include "SimDataFormats/JetMatching/interface/JetFlavourInfoMatching.h"

#include "CondFormats/JetMETObjects/interface/JetCorrectorParameters.h"
#include "CondFormats/JetMETObjects/interface/FactorizedJetCorrector.h"


/**\class HiInclusiveGenJetAnalyzer

    Copied from HiInclusiveJetAnalyzer 
    And kept only gen jet stuff
    Lida Kalipoliti

*/



class HiInclusiveGenJetAnalyzer : public edm::EDAnalyzer {
public:
  explicit HiInclusiveGenJetAnalyzer(const edm::ParameterSet&);
  ~HiInclusiveGenJetAnalyzer() override;
  void analyze(const edm::Event&, const edm::EventSetup&) override;
  void beginRun(const edm::Run& r, const edm::EventSetup& c) override;
  void beginJob() override;

private:
  
  template <typename jetType>
  int getGroomedJetIndex(const jetType& jet, const edm::View<reco::Jet> groomedJetsV) const;

  edm::EDGetTokenT<edm::View<reco::GenJet>> genJetsToken_;
  edm::EDGetTokenT<edm::View<reco::GenParticle>> genParticlesToken_;
  edm::EDGetTokenT<edm::View<reco::Jet>> groomedGenJetsToken_;
  edm::EDGetTokenT<edm::View<reco::PFCandidate>> pseudoHFGenToken_;

  // b and c hadrons
  edm::EDGetTokenT<reco::JetFlavourInfoMatchingCollection> jetFlavourInfosToken_;

  double genPtMin_;
  bool doSubJetsNew_;
  double trkPtCut_;

  TTree* t;
  edm::Service<TFileService> fs1;

  static const int MAXJETS = 1000;

  struct JRA {
    int nref=0;
    int run=0;
    int evt=0;
    int lumi=0;

    int ngen=0;
    float genpt[MAXJETS] = {0};
    float geneta[MAXJETS] = {0};
    float genphi[MAXJETS] = {0};
    float genm[MAXJETS] = {0};
    float geny[MAXJETS] = {0};

    // jet true flavour tagging
    int genParFlav[MAXJETS]={0};
    int genHadFlav[MAXJETS]={0};
    int genNbHad[MAXJETS]={0};
    int genNcHad[MAXJETS]={0};
    int genNbPar[MAXJETS]={0};
    int genNcPar[MAXJETS]={0};

    // reconstructed charged-part of B
    float genmB[MAXJETS]={0};
    float genBpt[MAXJETS]={0};
    float genBntracks[MAXJETS]={0};
    float genptCh[MAXJETS]={0};
    int genNtrk[MAXJETS]={0};

    // subjets
    int gsjt1HasHF[MAXJETS] = {0};
    float gsjt1Pt[MAXJETS] = {0};
    float gsjt1Eta[MAXJETS] = {0};
    float gsjt1Phi[MAXJETS] = {0};
    float gsjt1E[MAXJETS] = {0};
    float gsjt1Y[MAXJETS] = {0};
    float gsjt1Pz[MAXJETS] = {0};
    float gsjt2Pt[MAXJETS] = {0};
    float gsjt2Eta[MAXJETS] = {0};
    float gsjt2Phi[MAXJETS] = {0};
    float gsjt2E[MAXJETS] = {0};
    float gsjt2Y[MAXJETS] = {0};
    float gsjt2Pz[MAXJETS] = {0};
  };

  JRA jets_;
};

#endif
