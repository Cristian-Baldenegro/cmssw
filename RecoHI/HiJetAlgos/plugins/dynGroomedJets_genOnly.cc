
/*
    Copy of dynGroomedJets_genOnly.cc 
    but with vector<reco::GenJet> input instead of vector<pat::Jet>
*/

#include <memory>
#include <vector>
#include <cmath>
#include <map>
#include <random>

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/global/EDProducer.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/InputTag.h"

#include "DataFormats/Common/interface/View.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"
#include "DataFormats/PatCandidates/interface/PackedCandidate.h"

#include "fastjet/AreaDefinition.hh"
#include "fastjet/ClusterSequence.hh"
#include "fastjet/ClusterSequenceArea.hh"
#include "fastjet/contrib/SoftDrop.hh"

#include "RecoJets/JetProducers/interface/JetSpecific.h"

#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/BTauReco/interface/JetTag.h"
#include "DataFormats/BTauReco/interface/ShallowTagInfo.h"
#include "CommonTools/UtilAlgos/interface/DeltaR.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include "DataFormats/BTauReco/interface/SecondaryVertexTagInfo.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/JetReco/interface/Jet.h"
#include "DataFormats/PatCandidates/interface/Jet.h"

#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/PatCandidates/interface/PackedGenParticle.h"
#include "AnalysisDataFormats/TrackInfo/interface/TrackToGenParticleMap.h"

#include "HeavyIonsAnalysis/JetAnalysis/interface/HiInclusiveJetAnalyzer.h"


template <class T>
class dynGroomedJets_genOnly : public edm::global::EDProducer<> {
public:
  explicit dynGroomedJets_genOnly(const edm::ParameterSet&);
  // ~dynGroomedJets_genOnly() override = default;
  ~dynGroomedJets_genOnly() {}

  static void fillDescriptions(edm::ConfigurationDescriptions&);

private:
  void produce(edm::StreamID, edm::Event&, const edm::EventSetup&) const override;

  std::pair<bool,bool> IterativeDeclustering(std::vector<fastjet::PseudoJet>, 
                             fastjet::PseudoJet *, fastjet::PseudoJet *, 
                             std::vector<fastjet::PseudoJet>&, 
                             std::vector<fastjet::PseudoJet>&,
                             reco::PFCandidate) const;
  reco::BasicJet ConvertFJ2BasicJet(fastjet::PseudoJet *, 
                                    std::vector<fastjet::PseudoJet>, 
                                    edm::Handle<std::vector<reco::PFCandidate>>, 
                                    const edm::EventSetup&) const;
  reco::BasicJet ConvertFJ2BasicJet(fastjet::PseudoJet *, 
                                    std::vector<fastjet::PseudoJet>, 
                                    edm::Handle<edm::View<pat::PackedCandidate>>, 
                                    const edm::EventSetup&) const;

  
  typedef std::tuple<std::vector<fastjet::PseudoJet>, std::vector<reco::PFCandidate>, reco::PFCandidate> jetConstituentsPseudoHFTuple;
  jetConstituentsPseudoHFTuple aggregateHFGen(const T&, const std::vector<reco::GenParticle>) const;

  // ------------- member data ----------------------------
  edm::EDGetTokenT<std::vector<T>> genJetSrc_;
  edm::EDGetTokenT<std::vector<reco::GenParticle>> taggedGenParticleSrc_;

  bool writeConstits_;
  bool doLateKt_;
  bool chargedOnly_;
  
  double zcut_;
  double beta_;
  double dynktcut_;
  double ktcut_;
  double rParam_;
  double ptCut_;

  bool aggregateHF_;
};

template <class T>
dynGroomedJets_genOnly<T>::dynGroomedJets_genOnly(const edm::ParameterSet& iConfig) {
  // Get configuration parameters
  writeConstits_ = iConfig.getParameter<bool>("writeConstits");
  doLateKt_ = iConfig.getParameter<bool>("doLateKt");
  chargedOnly_ = iConfig.getParameter<bool>("chargedOnly");
  aggregateHF_ = iConfig.getParameter<bool>("aggregateHF"); 
  
  ptCut_ = iConfig.getParameter<double>("ptCut");
  zcut_ = iConfig.getParameter<double>("zcut");
  beta_ = iConfig.getParameter<double>("beta");
  dynktcut_ = iConfig.getParameter<double>("dynktcut");
  ktcut_ = iConfig.getParameter<double>("ktcut");
  rParam_ = iConfig.getParameter<double>("rParam");
  
  // Get tokens
  genJetSrc_ = consumes<std::vector<T>>(iConfig.getParameter<edm::InputTag>("genJetSrc"));
  taggedGenParticleSrc_ = consumes<std::vector<reco::GenParticle>>(iConfig.getParameter<edm::InputTag>("taggedGenParticleSrc"));

  std::string alias = (iConfig.getParameter<edm::InputTag>("genJetSrc")).label();
  produces<std::vector<reco::BasicJet>>().setBranchAlias(alias);
  produces<std::vector<reco::BasicJet>>("SubJets").setBranchAlias(alias);
  produces<std::vector<reco::PFCandidate>>("pseudoHF");
}

template <class T>
void dynGroomedJets_genOnly<T>::produce(edm::StreamID, edm::Event& iEvent, const edm::EventSetup& iSetup) const {
  // std::cout << "In dynGroomedJets_genOnly" << std::endl;

  auto jetCollection = std::make_unique<reco::BasicJetCollection>();
  auto subjetCollection = std::make_unique<reco::BasicJetCollection>();
  auto pseudoHFCollection = std::make_unique<std::vector<reco::PFCandidate>>();

  // This will store the handle for the subjets after we write them
  edm::OrphanHandle<std::vector<reco::BasicJet>> subjetHandleAfterPut;

  // this is the mapping of subjet to hard jet
  std::vector<std::vector<int>> indices;

  // this is the list of hardjet 4-momenta
  std::vector<math::XYZTLorentzVector> p4_hardJets;

  // this is the hardjet areas
  std::vector<double> area_hardJets;
  std::vector<bool> isHardest;
  std::vector<bool> leadingHF;

  edm::Handle<std::vector<T>> genJets;
  iEvent.getByToken(genJetSrc_, genJets);

  edm::Handle<std::vector<reco::GenParticle>> taggedGenParticles;
  iEvent.getByToken(taggedGenParticleSrc_, taggedGenParticles);

  // std::cout << "taggedGenParticles->size()=" << taggedGenParticles->size() << std::endl;
  for (size_t i=0; i<taggedGenParticles->size(); i++)
    // if (taggedGenParticles->at(i).status()>1) 
    //   std::cout << "taggedGenParticles->at(i).status() = " << taggedGenParticles->at(i).status() << std::endl;

  indices.resize(genJets->size());

  int jetIndex = 0;
  for (const T& jet : *genJets) { 
    // std::cout << "new jet with pt: " << jet.pt() << std::endl;

    p4_hardJets.push_back(math::XYZTLorentzVector(jet.px(), jet.py(), jet.pz(), jet.energy()));
    area_hardJets.push_back(jet.jetArea());

    std::vector<fastjet::PseudoJet> jetConstituents = {};

    fastjet::PseudoJet *subFJ1 = new fastjet::PseudoJet();
    fastjet::PseudoJet *subFJ2 = new fastjet::PseudoJet();
    std::vector<fastjet::PseudoJet> constit1;
    std::vector<fastjet::PseudoJet> constit2;

    // if (jet.hadronFlavour() == 5) {
    //   std::cout << "b jet, pt = " << jet.pt() << std::endl;
    // } else {
    //   std::cout << "light jet, pt = " << jet.pt() << std::endl;
    // }

    // std::cout << "before aggregation" << std::endl;
    // for (auto constit : jet.getJetConstituents()) {
    //     std::cout << "\t" << constit->pt() << std::endl;
    // }


    reco::PFCandidate pseudoHF;
    if (aggregateHF_) {
    //   std::cout << "originally n=" << jet.getJetConstituents().size() << std::endl;
      auto tempTuple = aggregateHFGen(jet, *taggedGenParticles);
      jetConstituents = std::get<0>(tempTuple);
      pseudoHF = std::get<2>(tempTuple);
      pseudoHFCollection->push_back(pseudoHF);
    //   std::cout << "after aggregation n=" << jetConstituents.size() << std::endl;
    } else {
      std::vector<edm::Ptr<reco::Candidate>> constituents = {}; 
      constituents = jet.getJetConstituents();
      for (edm::Ptr<reco::Candidate> constituent : constituents) {
        if ((chargedOnly_) && (constituent->charge() == 0)) continue;
        if (constituent->pt() < ptCut_) continue;
        jetConstituents.push_back(fastjet::PseudoJet(constituent->px(), constituent->py(), constituent->pz(), constituent->energy()));
      }
    }

    // std::cout << "after aggregation" << std::endl;
    // for (auto constit : jetConstituents) {
    //     std::cout << "\t" << constit.pt() << std::endl;
    // }

    // Iterative declustering
    std::pair<bool, bool> isHardestHasLeadingHF = IterativeDeclustering(jetConstituents, subFJ1, subFJ2, constit1, constit2, pseudoHF);
    isHardest.push_back(isHardestHasLeadingHF.first);
    leadingHF.push_back(isHardestHasLeadingHF.second);    

    // Convert fastjets to basicjets 
    edm::Handle<std::vector<reco::PFCandidate>> pfcands; // dummy
    reco::BasicJet subjet1, subjet2;
    subjet1 = ConvertFJ2BasicJet(subFJ1, constit1, pfcands, iSetup);
    subjet2 = ConvertFJ2BasicJet(subFJ2, constit2, pfcands, iSetup);
    
    if (subjet1.pt() > 1.0e-3) {
      indices[jetIndex].push_back(subjetCollection->size());
      if (subFJ1->has_area()) subjet1.setJetArea(subFJ1->area());
      subjetCollection->push_back(subjet1);
    }
    if (subjet2.pt() > 1.0e-3) {
      indices[jetIndex].push_back(subjetCollection->size());
      if (subFJ2->has_area()) subjet2.setJetArea(subFJ2->area());
      subjetCollection->push_back(subjet2);
    }

    jetIndex++;
  } // end jet loop

  // put subjets into event record
  subjetHandleAfterPut = iEvent.put(move(subjetCollection), "SubJets"); 

  // Now create the hard jets with ptr's to the subjets as constituents
  std::vector<math::XYZTLorentzVector>::const_iterator ip4 = p4_hardJets.begin(), ip4Begin = p4_hardJets.begin(), ip4End = p4_hardJets.end();
  
  for (; ip4 != ip4End; ++ip4) {
    int p4_index = ip4 - ip4Begin;
    std::vector<int>& ind = indices[p4_index];
    std::vector<edm::Ptr<reco::Candidate>> i_hardJetConstituents;

    // Add the subjets to the hard jet
    for (std::vector<int>::const_iterator isub = ind.begin(); isub != ind.end(); ++isub) {
      edm::Ptr<reco::Candidate> candPtr(subjetHandleAfterPut, *isub, false);
      i_hardJetConstituents.push_back(candPtr);
    }
    
    reco::Particle::Point point(0, 0, 0);
    //cout<<" size of i_hardJetConstituents "<<i_hardJetConstituents.size()<<endl;
    reco::BasicJet toput(*ip4, point, i_hardJetConstituents);
    // ---- hijack jet area to get the hf in leading prong flag
    // if (isHardest[ip4-ip4Begin]) toput.setJetArea(0.8);
    // else toput.setJetArea(0.4);
    if (leadingHF[ip4-ip4Begin]) toput.setJetArea(0.8);
    else toput.setJetArea(0.4);
    jetCollection->push_back(toput);
  }

  //cout << "jetCollection size: " << jetCollection->size() << endl; 
  iEvent.put(move(jetCollection));
  iEvent.put(move(pseudoHFCollection), "pseudoHF");  
}

template <class T>
reco::BasicJet dynGroomedJets_genOnly<T>::ConvertFJ2BasicJet(fastjet::PseudoJet *fj, 
                                                     std::vector<fastjet::PseudoJet> constit, 
                                                     edm::Handle<std::vector<reco::PFCandidate>> pfcands, 
                                                     const edm::EventSetup& iSetup) const
{
  math::XYZTLorentzVector p4(fj->px(), fj->py(), fj->pz(), fj->e());  
  reco::Particle::Point point(0, 0, 0);
  std::vector<edm::Ptr<reco::Candidate>> constituents;
  reco::BasicJet basicjet;
  writeSpecific(basicjet, p4, point, constituents, iSetup);
  return basicjet;
}

template <class T>
reco::BasicJet dynGroomedJets_genOnly<T>::ConvertFJ2BasicJet(fastjet::PseudoJet *fj, 
                                                     std::vector<fastjet::PseudoJet> constit, 
                                                     edm::Handle<edm::View<pat::PackedCandidate>> pfcands, 
                                                     const edm::EventSetup& iSetup) const
{
  math::XYZTLorentzVector p4(fj->px(), fj->py(), fj->pz(), fj->e());  
  reco::Particle::Point point(0, 0, 0);
  std::vector<edm::Ptr<reco::Candidate>> constituents;
  reco::BasicJet basicjet;
  writeSpecific(basicjet, p4, point, constituents, iSetup);

  return basicjet;
}

template <class T>
std::pair<bool, bool> dynGroomedJets_genOnly<T>::IterativeDeclustering(std::vector<fastjet::PseudoJet> jetConstituents, 
                                              fastjet::PseudoJet *sub1, fastjet::PseudoJet *sub2, 
                                              std::vector<fastjet::PseudoJet> &constit1, std::vector<fastjet::PseudoJet> &constit2,
                                              reco::PFCandidate pseudoHF) const
{
  //  std::cout << "--- Declustering --- " << std::endl;
  // Iterative declustering for any type of jet
  // given its constituents
  // returns true/false about whether the selected split is the hardest

  bool flagSubjet=false;
  bool isHardest=false;
  bool flagHF=false;
  double kt1=-1;
  double nsplit=0;
  double nsel=0;
  double nsdin=-1;
  double jet_radius_ca = 1.0;
  fastjet::JetDefinition jet_def(fastjet::genkt_algorithm, jet_radius_ca, 0, static_cast <fastjet::RecombinationScheme>(0), fastjet::Best);

  // Return false if no constituents
  if (jetConstituents.size() == 0) return std::pair<bool, bool>(false, false);

  // Reclustering jet constituents with new algorithm                                                                                          
  try
    {
      fastjet::ClusterSequence csiter(jetConstituents, jet_def);
      std::vector<fastjet::PseudoJet> output_jets = csiter.inclusive_jets(0);
      output_jets = sorted_by_pt(output_jets);
      
      
      fastjet::PseudoJet jj = output_jets[0];
      fastjet::PseudoJet j1;
      fastjet::PseudoJet j2;                                                                                                               
      fastjet::PseudoJet j1first;
      fastjet::PseudoJet j2first;

      while (jj.has_parents(j1, j2)) {
        if (j1.perp() < j2.perp()) std::swap(j1,j2);
        
        double delta_R = j1.delta_R(j2);
        double cut = zcut_ * pow(delta_R / rParam_, beta_);
        double z = j2.perp() / (j1.perp() + j2.perp());
        double kt = j2.perp() * delta_R;
        bool passCut = (z > cut);
        if (doLateKt_) passCut = (kt > ktcut_);
        if (passCut && (doLateKt_ || !flagSubjet) ) {
          flagSubjet = true;
          j1first = j1;
          j2first = j2;
          *sub1 = j1first;
          *sub2 = j2first;
          nsdin = nsplit;

          flagHF = false;
          if (aggregateHF_) {
            // if aggregateHF look for B particle
            std::vector<fastjet::PseudoJet> j1constituents = j1.constituents();
            double e = 1e-3;
            // std::cout << "\tparticles in leading prong:" << std::endl;
            for (size_t icon = 0; icon < j1constituents.size(); icon++) {
              fastjet::PseudoJet constit = j1constituents[icon];
              // std::cout << "\t\t-pt = " << constit.pt() << std::endl;
              // pseudoJet -> packedCandidate 
              reco::Candidate::PolarLorentzVector tempVector(0., 0., 0., 0.);
              tempVector.SetPt(constit.pt());
              tempVector.SetEta(constit.eta());
              tempVector.SetPhi(constit.phi());
              tempVector.SetM(constit.m());
              pat::PackedCandidate packedConstit;
              packedConstit.setP4(tempVector);    
              
            
              if (std::abs(packedConstit.pt() - pseudoHF.pt()) > e) continue;
              flagHF = true;
              // std::cout << "\t --->HF in leading prong" << std::endl;
              break;
            }
          }

          // if (!flagHF) {
          //   std::cout << "\tparticles in subleading prong:" << std::endl;
          //   std::vector<fastjet::PseudoJet> j2constituents = j2.constituents();
          //   for (size_t icon = 0; icon < j2constituents.size(); icon++) {
          //     fastjet::PseudoJet constit = j2constituents[icon];
          //     std::cout << "\t\t-m = " << constit.m() << std::endl;
          //     if (std::abs(constit.m() - pseudoHF.mass()) > e) continue;

          //     flagHF = true;
          //     std::cout << "\t --->HF in subleading prong" << std::endl;
          //     break;
          //   }
          // }

          // if (!flagHF) std::cout << "\tHF is NOT in the leading prong" << std::endl;
        }
        // if (!passCut) {
        //   std::cout << "\tsplit didn't pass the cut" << std::endl;
        // }
        double dyn= z * (1-z) * j2.perp() * pow(delta_R / rParam_, dynktcut_);
        
        if (dyn > kt1) {
          nsel = nsplit;
          kt1 = dyn;
        }
        nsplit = nsplit + 1;
        jj = j1;
      }
      
      if (!flagSubjet) *sub1 = output_jets[0];

      if (sub1->has_constituents()) constit1 = sub1->constituents(); 
      if (sub2->has_constituents()) constit2 = sub2->constituents(); 
      if (nsel == nsdin) isHardest = true; 
    } catch (fastjet::Error) {} //return -1; }

  
  return std::pair<bool, bool>(isHardest, flagHF);
}

// Function to aggregate gen particles coming from B decays into pseudo-B's 
template <class T>
typename dynGroomedJets_genOnly<T>::jetConstituentsPseudoHFTuple dynGroomedJets_genOnly<T>::aggregateHFGen(
    const T& genJet, const std::vector<reco::GenParticle> taggedGenParticles
    ) const
{
  // std::cout << "Aggregating HF at gen level" << std::endl;

  // Input and output particle collections
  std::vector<edm::Ptr<reco::Candidate>> inputJetConstituents = genJet.getJetConstituents();
  std::vector<fastjet::PseudoJet> outputJetConstituents = {};
  std::vector<reco::PFCandidate> droppedTracks = {}; // this will remain empty
  reco::PFCandidate outputPseudoHF;

  // Particle collection to aggregate into pseudo-Bs
  std::map<int, std::vector<edm::Ptr<reco::Candidate>>> hfConstituentsMap;
  reco::Candidate::PolarLorentzVector totalPseudoHF(0., 0., 0., 0.);

  // Go over jet constituents
  for (edm::Ptr<reco::Candidate> constit : inputJetConstituents) {
    if(chargedOnly_ && constit->charge() == 0) continue;
    if(constit->pt() < ptCut_) continue;
  
    // find gen particle in tagged gen particles 
    const reco::GenParticle *matchGenParticle = nullptr;
    double eps = 1e-3;
    // std::cout << "particle of interest: pt=" << constit->pt() << ", eta=" << constit->eta() << ", phi=" << constit->phi()<<std::endl;
    for (size_t ip=0; ip<taggedGenParticles.size(); ip++) {
      // reco::GenParticle tempParticle(*constit);
      // pat::PackedGenParticle packedConstit(tempParticle, reco::GenParticleRef());
      const reco::GenParticle *taggedGenParticle = &(taggedGenParticles.at(ip));

      // std::cout << "\ttagged gen particle: pt=" << taggedGenParticle->pt() << ", eta=" << taggedGenParticle->eta() << ", phi=" << taggedGenParticle->phi()<<std::endl;

      if (std::abs(constit->pt()-taggedGenParticle->pt())>eps) continue;
      if (std::abs(constit->eta()-taggedGenParticle->eta())>eps) continue;
      if (std::abs(constit->phi()-taggedGenParticle->phi())>eps) continue;

      // std::cout << "\ttagged gen particle: status=" << taggedGenParticle->status() <<std::endl;


      matchGenParticle = taggedGenParticle;
      // std::cout << "yes match" << std::endl;
      break;
    }

    int status = 0;
    if (matchGenParticle) {
        status = matchGenParticle->status();
        // std::cout << "yes match" << std::endl;
    }
    else {
        // std::cout << "no match" << std::endl;
        continue;
    }
    // std::cout << "before crash?" << std::endl;

    // Add particle to output collection or from HF map
    if (status == 1) {
      fastjet::PseudoJet outConstit(constit->px(), constit->py(), constit->pz(), constit->energy());
      outputJetConstituents.push_back(outConstit);
      // std::cout << "before crash?" << std::endl;
    } else if (status >= 100) {
      // std::cout << "\t\t particle with status " << status <<" and pt " << constit->pt() <<std::endl;
      hfConstituentsMap[status].push_back(constit);
    }
    // std::cout << "before crash?" << std::endl;
  } // end constit loop 
  // std::cout << "out of the loop" << std::endl;

  // Aggregate particles coming from HF decays into pseudo-B/D's 
  // and add them to the collection
  for (auto it = hfConstituentsMap.begin(); it != hfConstituentsMap.end(); ++it) {
    // std::cout << "B with code " << it->first << " has " << (it->second).size() << " constituents" << std::endl;
    reco::Candidate::PolarLorentzVector pseudoHF(0., 0., 0., 0.);
    for (edm::Ptr<reco::Candidate> hfConstituent : it->second) {
      reco::Candidate::PolarLorentzVector productLorentzVector(0., 0., 0., 0.);
      productLorentzVector.SetPt(hfConstituent->pt());
      productLorentzVector.SetEta(hfConstituent->eta());
      productLorentzVector.SetPhi(hfConstituent->phi());
      productLorentzVector.SetM(hfConstituent->mass());
      pseudoHF += productLorentzVector;
      totalPseudoHF += productLorentzVector;
      //std::cout << "hf product lorentz vector: " << productLorentzVector << std::endl;

      reco::PFCandidate daughter;
      daughter.setCharge(hfConstituent->charge());
      daughter.setP4(productLorentzVector);
      outputPseudoHF.addDaughter(daughter);
    }
    // std::cout << "added HF with pt=" << pseudoHF.Pt() << " to collection" << std::endl;
    fastjet::PseudoJet outPseudoHFConstit(pseudoHF.Px(), pseudoHF.Py(), pseudoHF.Pz(), pseudoHF.E());
    outputJetConstituents.push_back(outPseudoHFConstit);
  } // end map loop
  // std::cout << "totalPseudoHF pt=" << totalPseudoHF.Pt() << std::endl;
  // std::cout << "\tgenJet has " << hfConstituentsMap.size() << " B's " << std::endl;

  outputPseudoHF.setP4(totalPseudoHF);
  // std::cout << "vector mass " << totalPseudoHF.M() << " candidate mass " << outputPseudoHF.mass() << std::endl;
  // std::cout << "\tjet constituents after aggregation" << std::endl;
  // for (fastjet::PseudoJet constit : outputJetConstituents) {
  //   std::cout << "\t\t-m=" << constit.m() << std::endl;
  // }
  return dynGroomedJets_genOnly<T>::jetConstituentsPseudoHFTuple {outputJetConstituents, droppedTracks, outputPseudoHF};
}

template <class T>
void dynGroomedJets_genOnly<T>::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  desc.setComment("Dynamically groomed gen jets");

  // Input collections
  desc.add<edm::InputTag>("genJetSrc", edm::InputTag("ak4GenJetsNoNu"));
  desc.add<edm::InputTag>("genParticleSrc", edm::InputTag("genParticles"));
  desc.add<edm::InputTag>("taggedGenParticleSrc", edm::InputTag("taggedGenParticles", "recoGenParticles"));

  // Configuration parameters
  desc.add<bool>("writeConstits", false);
  desc.add<bool>("doLateKt", false);
  desc.add<bool>("chargedOnly", false);
  
  desc.add<double>("zcut", 0.1);
  desc.add<double>("beta", 0.0);
  desc.add<double>("dynktcut", 1.0);
  desc.add<double>("ktcut", 1.0);
  desc.add<double>("rParam", 0.4);
  desc.add<double>("ptCut", 1.);

  desc.add<bool>("aggregateHF", false);

  descriptions.add("dynGroomedGenJets", desc);
}

using dynGroomedGenJets = dynGroomedJets_genOnly<reco::GenJet>;

// define this as a plug-in
DEFINE_FWK_MODULE(dynGroomedGenJets);
