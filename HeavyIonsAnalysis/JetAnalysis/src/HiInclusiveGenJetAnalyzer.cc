/*
  Based on the jet response analyzer
  Modified by Matt Nguyen, November 2010
*/

#include "HeavyIonsAnalysis/JetAnalysis/interface/HiInclusiveGenJetAnalyzer.h"
#include "DataFormats/Common/interface/View.h"
#include "DataFormats/JetReco/interface/CaloJetCollection.h"
#include "DataFormats/JetReco/interface/GenJetCollection.h"
#include "DataFormats/Math/interface/deltaPhi.h"
#include "DataFormats/Math/interface/deltaR.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "SimDataFormats/GeneratorProducts/interface/GenEventInfoProduct.h"
#include "SimDataFormats/GeneratorProducts/interface/HepMCProduct.h"
#include "DataFormats/PatCandidates/interface/PackedCandidate.h"
#include "DataFormats/PatCandidates/interface/PackedGenParticle.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"
#include "fastjet/contrib/Njettiness.hh"
#include "fastjet/AreaDefinition.hh"
#include "fastjet/ClusterSequence.hh"
#include "fastjet/ClusterSequenceArea.hh"
#include "fastjet/contrib/SoftDrop.hh"
#include "AnalysisDataFormats/TrackInfo/interface/TrackToGenParticleMap.h"
#include "JetMETCorrections/Modules/interface/JetResolution.h"
#include "CondFormats/JetMETObjects/interface/JetCorrectionUncertainty.h"
#include "TRandom.h"

using namespace std;
using namespace edm;
using namespace reco;

HiInclusiveGenJetAnalyzer::HiInclusiveGenJetAnalyzer(const edm::ParameterSet& iConfig) {

  genJetsToken_ = consumes<edm::View<reco::GenJet>>(iConfig.getParameter<edm::InputTag>("genJets"));
  genPtMin_ = iConfig.getUntrackedParameter<double>("genPtMin", 10);

  jetFlavourInfosToken_ = consumes<reco::JetFlavourInfoMatchingCollection>( iConfig.getParameter<edm::InputTag>("jetFlavourInfos") );
  genParticlesToken_ = consumes<edm::View<reco::GenParticle>>(iConfig.getParameter<edm::InputTag>("genParticles"));

  doSubJetsNew_ = iConfig.getUntrackedParameter<bool>("doSubJetsNew", false);
  if (doSubJetsNew_) {
    groomedGenJetsToken_ = consumes<edm::View<reco::Jet>>(iConfig.getUntrackedParameter<edm::InputTag>("groomedGenJets", edm::InputTag("ak4GenJetsNoNu")));
    pseudoHFGenToken_ = consumes<edm::View<reco::PFCandidate>>(iConfig.getUntrackedParameter<edm::InputTag>("pseudoHF", edm::InputTag("dynGroomedGenJets", "pseudoHF")));
  }

  trkPtCut_ = iConfig.getUntrackedParameter<double>("trkPtCut", 1.);
}

HiInclusiveGenJetAnalyzer::~HiInclusiveGenJetAnalyzer() {}

void HiInclusiveGenJetAnalyzer::beginRun(const edm::Run& run, const edm::EventSetup& es) {}

void HiInclusiveGenJetAnalyzer::beginJob() {
  t = fs1->make<TTree>("t", "gen jets");

  t->Branch("run", &jets_.run, "run/I");
  t->Branch("evt", &jets_.evt, "evt/I");
  t->Branch("lumi", &jets_.lumi, "lumi/I");

  t->Branch("ngen", &jets_.ngen, "ngen/I");
  t->Branch("genpt", jets_.genpt, "genpt[ngen]/F");
  t->Branch("geneta", jets_.geneta, "geneta[ngen]/F");
  t->Branch("geny", jets_.geny, "geny[ngen]/F");
  t->Branch("genphi", jets_.genphi, "genphi[ngen]/F");
  t->Branch("genm", jets_.genm, "genm[ngen]/F");

  t->Branch("genNtrk", &jets_.genNtrk, "genNtrk[ngen]/I");
  t->Branch("genptCh", jets_.genptCh, "genptCh[ngen]/F");

  t->Branch("genNbHad", jets_.genNbHad, "genNbHad[ngen]/I");
  t->Branch("genNcHad", jets_.genNcHad, "genNcHad[ngen]/I");
  t->Branch("genNbPar", jets_.genNbPar, "genNbPar[ngen]/I");
  t->Branch("genNcPar", jets_.genNcPar, "genNcPar[ngen]/I");

  if (doSubJetsNew_) {
    t->Branch("gsjt1HasHF", jets_.gsjt1HasHF, "gsjt1HasHF[ngen]/I");
    t->Branch("gsjt1Pt", jets_.gsjt1Pt, "gsjt1Pt[ngen]/F");
    t->Branch("gsjt1Eta", jets_.gsjt1Eta, "gsjt1Eta[ngen]/F");
    t->Branch("gsjt1Phi", jets_.gsjt1Phi, "gsjt1Phi[ngen]/F");
    t->Branch("gsjt1E", jets_.gsjt1E, "gsjt1E[ngen]/F");
    t->Branch("gsjt1Y", jets_.gsjt1Y, "gsjt1Y[ngen]/F");
    t->Branch("gsjt1Pz", jets_.gsjt1Pz, "gsjt1Pz[ngen]/F");
    t->Branch("gsjt2Pt", jets_.gsjt2Pt, "gsjt2Pt[ngen]/F");
    t->Branch("gsjt2Eta", jets_.gsjt2Eta, "gsjt2Eta[ngen]/F");
    t->Branch("gsjt2Phi", jets_.gsjt2Phi, "gsjt2Phi[ngen]/F");
    t->Branch("gsjt2E", jets_.gsjt2E, "gsjt2E[ngen]/F");
    t->Branch("gsjt2Y", jets_.gsjt2Y, "gsjt2Y[ngen]/F");
    t->Branch("gsjt2Pz", jets_.gsjt2Pz, "gsjt2Pz[ngen]/F");

    t->Branch("genmB", jets_.genmB, "genmB[ngen]/F");
    t->Branch("genBpt", jets_.genBpt, "genBpt[ngen]/F");
    t->Branch("genBntracks", jets_.genBntracks, "genBntracks[ngen]/F");
  }
}

void HiInclusiveGenJetAnalyzer::analyze(const Event& iEvent, const EventSetup& iSetup) {
  int event = iEvent.id().event();
  int run = iEvent.id().run();
  int lumi = iEvent.id().luminosityBlock();

  jets_.run = run;
  jets_.evt = event;
  jets_.lumi = lumi;

  LogDebug("HiInclusiveGenJetAnalyzer") << "START event: " << event << " in run " << run << endl;

  edm::Handle<edm::View<reco::GenJet>> genJets; 
  iEvent.getByToken(genJetsToken_, genJets);

  edm::Handle<reco::JetFlavourInfoMatchingCollection> jetFlavourInfos;
  iEvent.getByToken(jetFlavourInfosToken_, jetFlavourInfos);

  edm::Handle<edm::View<reco::Jet>> groomedGenJets; 
  edm::Handle<edm::View<reco::PFCandidate>> pseudoHFGenCollection; 
  if (doSubJetsNew_) {
    iEvent.getByToken(groomedGenJetsToken_, groomedGenJets);
    iEvent.getByToken(pseudoHFGenToken_, pseudoHFGenCollection);
  }

  edm::Handle<edm::View<reco::GenParticle>> genParticles;
  iEvent.getByToken(genParticlesToken_, genParticles);

  // edm::Handle<reco::JetFlavourInfoMatchingCollection> jetFlavourInfos;
  // iEvent.getByToken(jetFlavourInfosToken_, jetFlavourInfos );

  // FILL JRA TREE
  jets_.ngen = 0;

  for (unsigned int igen = 0; igen < genJets->size(); igen++) {
    const reco::GenJet& genJet = (*genJets)[igen];
    if (genJet.pt() < genPtMin_) continue;
    jets_.genpt[jets_.ngen] = genJet.pt();
    jets_.geneta[jets_.ngen] = genJet.eta();
    jets_.genphi[jets_.ngen] = genJet.phi();
    jets_.genm[jets_.ngen] = genJet.mass();
    jets_.geny[jets_.ngen] = genJet.eta();

    // ---------- GEN JET FLAVOR -----------
    for (const JetFlavourInfoMatching& jetFlavourInfoMatching : *jetFlavourInfos) {
      if (deltaR(genJet.p4(), jetFlavourInfoMatching.first->p4()) > 1e-6) continue;

      JetFlavourInfo jetInfo = jetFlavourInfoMatching.second;
      const GenParticleRefVector &bHadronsInJet = jetInfo.getbHadrons();
      const GenParticleRefVector &cHadronsInJet = jetInfo.getcHadrons();
      jets_.genNbHad[jets_.ngen] = bHadronsInJet.size();
      jets_.genNcHad[jets_.ngen] = cHadronsInJet.size();

      const GenParticleRefVector &partonsInJet = jetInfo.getPartons();

      int nb=0;
      int nc=0;
      // bool hasBfromGSP = false;
      // bool hasCfromGSP = false;

      for (GenParticleRefVector::const_iterator it = partonsInJet.begin(); it != partonsInJet.end(); ++it) {
        int parFlav = (*it)->pdgId();
        // const Candidate* c = (*it).get();

        if(abs(parFlav)==5){
          nb++;
          // if(isFromGSP(c)) hasBfromGSP = true;
        } else if(abs(parFlav)==4) {
          nc++;
          // if(isFromGSP(c)) hasCfromGSP = true;
        }
      }

      jets_.genNbPar[jets_.ngen] = nb;
      jets_.genNcPar[jets_.ngen] = nc;
      //  jets_.jtHasGSPB[jets_.ngen] = hasBfromGSP;
      //  jets_.jtHasGSPC[jets_.ngen] = hasCfromGSP;

      break;
    } // loop over flavor infos 

    // ---------------- CHARGED JET PT ---------
    jets_.genptCh[jets_.ngen] = 0.;
    jets_.genNtrk[jets_.ngen] = 0;

    reco::Candidate::PolarLorentzVector chGenJet(0., 0., 0., 0.);
    for (auto genConstit : genJet.getJetConstituents()) {
      if (genConstit->pt() < trkPtCut_) continue;
      if (genConstit->charge() == 0) continue;

      // just to be safe
      bool isNeutrino = (genConstit->pdgId() == 12); // nue
      isNeutrino &= (genConstit->pdgId() == 14); // numu
      isNeutrino &= (genConstit->pdgId() == 16); // nutau
      isNeutrino &= (genConstit->pdgId() == 18); // nutau'
      if (isNeutrino) continue;

      reco::Candidate::PolarLorentzVector constitV(0., 0., 0., 0.);
      constitV.SetPt(genConstit->pt());
      constitV.SetEta(genConstit->eta());
      constitV.SetPhi(genConstit->phi());
      constitV.SetM(genConstit->mass());
      chGenJet += constitV;
    
      jets_.genNtrk[jets_.ngen] += 1;
    } // end loop over gen jet constituents
    jets_.genptCh[jets_.ngen] = chGenJet.pt();

    // ---------------- SUBJETS ----------------
    jets_.gsjt1HasHF[jets_.ngen] = -1;

    jets_.gsjt1E[jets_.ngen] = -1;
    jets_.gsjt1Y[jets_.ngen] = -999;
    jets_.gsjt1Pz[jets_.ngen] = -999;
    jets_.gsjt1Pt[jets_.ngen] = -1;
    jets_.gsjt1Eta[jets_.ngen] = -999;
    jets_.gsjt1Phi[jets_.ngen] = -999;
    
    jets_.gsjt2Pt[jets_.ngen] = -1;
    jets_.gsjt2Eta[jets_.ngen] = -999;
    jets_.gsjt2Phi[jets_.ngen] = -999;
    jets_.gsjt2E[jets_.ngen] = -1;
    jets_.gsjt2Y[jets_.ngen] = -999;
    jets_.gsjt2Pz[jets_.ngen] = -999;

    jets_.genmB[jets_.ngen] = -1.;
    jets_.genBpt[jets_.ngen] = -1;
    jets_.genBntracks[jets_.ngen] = -1;

    if (doSubJetsNew_) {
      int iGroomedGenJet = getGroomedJetIndex(genJet, *groomedGenJets);
      if (iGroomedGenJet > -1) {
        const reco::Jet& groomedGenJet = (*groomedGenJets)[iGroomedGenJet];
        if (groomedGenJet.numberOfDaughters() > 0) {
          const Candidate & gsjt1 = *groomedGenJet.daughter(0);       
          jets_.gsjt1E[jets_.ngen] = gsjt1.energy();
          jets_.gsjt1Y[jets_.ngen] = gsjt1.y();
          jets_.gsjt1Pz[jets_.ngen] = gsjt1.pz();
          jets_.gsjt1Pt[jets_.ngen] = gsjt1.pt();
          jets_.gsjt1Eta[jets_.ngen] = gsjt1.eta();
          jets_.gsjt1Phi[jets_.ngen] = gsjt1.phi();

          if (groomedGenJet.jetArea() > 0.5) jets_.gsjt1HasHF[jets_.ngen] = 1;
          else jets_.gsjt1HasHF[jets_.ngen] = 0;
        }

        if (groomedGenJet.numberOfDaughters() > 1) {
          const Candidate & gsjt2 = *groomedGenJet.daughter(1);
          jets_.gsjt2E[jets_.ngen] = gsjt2.energy();
          jets_.gsjt2Y[jets_.ngen] = gsjt2.y();
          jets_.gsjt2Pz[jets_.ngen] = gsjt2.pz();
          jets_.gsjt2Pt[jets_.ngen] = gsjt2.pt();
          jets_.gsjt2Eta[jets_.ngen] = gsjt2.eta();
          jets_.gsjt2Phi[jets_.ngen] = gsjt2.phi();
        }
        
        if (pseudoHFGenCollection->size() > 0) {
          reco::PFCandidate pseudoBGen = (*pseudoHFGenCollection)[iGroomedGenJet];
          jets_.genmB[jets_.ngen] = pseudoBGen.mass();
          jets_.genBpt[jets_.ngen] = pseudoBGen.pt();
          jets_.genBntracks[jets_.ngen] = pseudoBGen.numberOfDaughters();
        //   for (size_t i=0; i<pseudoBGen.numberOfDaughters();i++) {
        //     reco::Candidate *daughter = pseudoBGen.daughter(i);
        //     std::cout << "\tdaughter charge=" << daughter->charge() << std::endl;
        //   }
        }
      } // end if groomedGenJet match exists
    } // end doSubjetsNew_ 
  
    // std::cout << "jets_.genNtrk[jets_.ngen]=" << jets_.genNtrk[jets_.ngen] << std::endl;
    jets_.ngen++;
  } // end loop over gen jets
  t->Fill();
  //memset(&jets_,0,sizeof jets_);
  jets_ = {0};
} // end analyze

//--------------------------------------------------------------------------------------------------

template <typename jetType>
int HiInclusiveGenJetAnalyzer::getGroomedJetIndex(const jetType& jet, const edm::View<reco::Jet> groomedJetsV) const {
  //Find closest soft-dropped gen jet
  double drMin = 100;
  int imatch = -1;
  for (unsigned int i = 0; i < groomedJetsV.size(); ++i) {
    const reco::Jet& mjet = groomedJetsV[i];

    double dr = deltaR(jet, mjet);
    if (dr < drMin) {
      imatch = i;
      drMin = dr;
    }
  }
  return imatch;
}

DEFINE_FWK_MODULE(HiInclusiveGenJetAnalyzer);
