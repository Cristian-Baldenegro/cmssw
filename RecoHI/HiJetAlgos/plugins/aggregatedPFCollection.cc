
/* 
 
 Aggregate B hadrons in PF Collection.
  
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
#include "CommonTools/MVAUtils/interface/TMVAEvaluator.h"


class aggregatedPFCollection : public edm::global::EDProducer<> {
public:
    explicit aggregatedPFCollection(const edm::ParameterSet&);
    ~aggregatedPFCollection() override = default;

    void produce(edm::StreamID, edm::Event&, const edm::EventSetup&) const override;
    static void fillDescriptions(edm::ConfigurationDescriptions&);

private:
  
  
    // ------------- member data ----------------------------
    edm::EDGetTokenT<pat::JetCollection> jetSrc_;
    edm::EDGetTokenT<pat::JetCollection> matchTag_; 
    edm::EDGetTokenT<std::vector<pat::PackedCandidate>> constitSrc_;
    edm::EDGetTokenT<edm::View<pat::PackedCandidate>>  packedConstitSrc_;
    edm::EDGetTokenT<reco::TrackToGenParticleMap> candToGenParticleMapToken_;
    edm::EDGetTokenT<reco::GenParticleCollection> genParticlesToken_;
    edm::EDGetTokenT<std::vector<reco::Vertex>> primaryVerticesToken_;
    edm::Handle<std::vector<reco::Vertex>> primaryVertices;  
    std::unique_ptr<TMVAEvaluator> tmvaTagger;

    bool isMC_;
    bool writeConstits_;
    bool doGenJets_;
    bool chargedOnly_;
    bool domatch_;
  
    double rParam_;
    double ptCut_; // For tracks in aggregation
    double trkInefRate_; // 0 by default
    double jetPtCut_;// skip low pT jets
    double jetEtaCut_ = 3.0;// skip forward jets

    bool aggregateHF_;
    bool withTruthInfo_;
    bool withCuts_;
    bool withTMVA_;
    edm::FileInPath tmva_path_;
    std::vector<std::string> tmva_variable_names_;
    std::vector<std::string> tmva_spectator_names_;
    std::string ipTagInfoLabel_;
    std::string svTagInfoLabel_;
};

aggregatedPFCollection::aggregatedPFCollection(const edm::ParameterSet& iConfig) {
    // Get configuration parameters
    isMC_ = iConfig.getParameter<bool>("isMC");
    chargedOnly_ = iConfig.getParameter<bool>("chargedOnly");
    aggregateHF_ = iConfig.getParameter<bool>("aggregateHF"); 
    doGenJets_ = iConfig.getParameter<bool>("doGenJets");
    domatch_ = iConfig.getParameter<bool>("domatch");  
    ptCut_ = iConfig.getParameter<double>("ptCut");
    jetPtCut_ = iConfig.getParameter<double>("jetPtCut");
    trkInefRate_ = iConfig.getParameter<double>("trkInefRate");

    if (aggregateHF_) {
        withTruthInfo_ = iConfig.getParameter<bool>("aggregateWithTruthInfo");
        withCuts_ = iConfig.getParameter<bool>("aggregateWithCuts");
        withTMVA_ = iConfig.getParameter<bool>("aggregateWithTMVA");

        if (withTMVA_) {
            tmva_path_ = iConfig.getParameter<edm::FileInPath>("tmva_path");
            tmva_variable_names_ = iConfig.getParameter<std::vector<std::string>>("tmva_variables");
            tmva_spectator_names_ = iConfig.getParameter<std::vector<std::string>>("tmva_spectators");
        }
    } 

    // Get labels
    ipTagInfoLabel_ = iConfig.getParameter<std::string>("ipTagInfoLabel");
    svTagInfoLabel_ = iConfig.getParameter<std::string>("svTagInfoLabel");
  
    // Get tokens
    jetSrc_ = consumes<pat::JetCollection>(iConfig.getParameter<edm::InputTag>("jetSrc"));
    matchTag_ = consumes<pat::JetCollection>(iConfig.getParameter<edm::InputTag>("matchTag"));
    constitSrc_ = consumes<std::vector<pat::PackedCandidate>>(iConfig.getParameter<edm::InputTag>("constitSrc"));
    packedConstitSrc_ = consumes<edm::View<pat::PackedCandidate>>(iConfig.getParameter<edm::InputTag>("constitSrc"));
    primaryVerticesToken_ = consumes<std::vector<reco::Vertex>>(iConfig.getUntrackedParameter<edm::InputTag>("primaryVertices", edm::InputTag("offlineSlimmedPrimaryVertices")));

    if (aggregateHF_ && isMC_) candToGenParticleMapToken_ = consumes<reco::TrackToGenParticleMap>(iConfig.getParameter<edm::InputTag>("candToGenParticleMap"));

    // Initialize objects 
    if (aggregateHF_ && withTMVA_) {
        tmvaTagger = std::make_unique<TMVAEvaluator>();
        tmvaTagger->initialize("Color:Silent:Error",
                               "BDTG",
                                tmva_path_.fullPath(),
                                tmva_variable_names_,
                                tmva_spectator_names_,
                                false,
                                false);
    }

    produces<pat::PackedCandidateCollection>();
//    produces<pat::PackedCandidateCollection>("newPFCollectionHFgen");
//    produces<pat::PackedCandidateCollection>("newPFCollectionHFreco");

}

void aggregatedPFCollection::produce(edm::StreamID, edm::Event& iEvent, const edm::EventSetup& iSetup) const {

    auto newPFCandCollection = std::make_unique<pat::PackedCandidateCollection>();

    edm::Handle<pat::JetCollection> jets;
    iEvent.getByToken(jetSrc_, jets);    

    // Matched jets for taginfos and TMVA aggregation 
    edm::Handle<pat::JetCollection> matchedjets;
    iEvent.getByToken(matchTag_, matchedjets);

    // -- For aggregation -- //
    edm::Handle<reco::TrackToGenParticleMap> candToGenParticleMap;

    if (aggregateHF_ && isMC_) {
        iEvent.getByToken(candToGenParticleMapToken_, candToGenParticleMap);
    } 
  
    edm::Handle<std::vector<reco::Vertex>> primaryVertices;
    iEvent.getByToken(primaryVerticesToken_, primaryVertices);

    std::vector<fastjet::PseudoJet> jetConstituents = {};

    for(unsigned int j = 0; j < jets->size(); ++j){

        const pat::Jet& jet = (*jets)[j];

        if ( jet.pt() < jetPtCut_ ) continue;
        if ( abs(jet.eta()) > jetEtaCut_ ) continue;

        if (aggregateHF_) {
      
            //std::cout << " doGenJets = " << doGenJets_  << " is MC = " << isMC_ << std::endl;
            if (doGenJets_ && isMC_) {

                pat::PackedCandidate outputPseudoHF;
                const reco::GenJet *genJet = jet.genJet();

                // Particle collection to aggregate into pseudo-Bs
                //std::vector<edm::Ptr<reco::Candidate>> inputJetConstituents = genJet->getJetConstituents();
                std::map<int, std::vector<edm::Ptr<reco::Candidate>>> hfConstituentsMap;
                reco::Candidate::PolarLorentzVector totalPseudoHF(0., 0., 0., 0.);

                //int HFjet = 0;
                // Go over gen particles

                if (genJet) {
                    for (const edm::Ptr<reco::Candidate> &constit : (*genJet).getJetConstituents()) {
   
                        if(chargedOnly_ && constit->charge() == 0) continue;

                        bool isNeutrino = (constit->pdgId() == 12); // nue
                             isNeutrino &= (constit->pdgId() == 14); // numu
                             isNeutrino &= (constit->pdgId() == 16); // nutau
                             isNeutrino &= (constit->pdgId() == 18); // nutau'
                        if (isNeutrino) {
                            std::cout << "found a neutrino" << std::endl;
                            continue;
                        }

                        if (constit->charge() == 0 or constit->pt() < ptCut_) {   // Neutrals and low pT tracks are not used in aggregation, let's fill them directly in the collection
                            pat::PackedCandidate constituentsPF;
                            constituentsPF.setCharge(constit->charge());
                            constituentsPF.setP4(constit->p4());
                            constituentsPF.setPdgId(constit->pdgId());
                            newPFCandCollection->push_back(constituentsPF);
                            continue;
                        }

                        //std::cout << "---- Got gen level constit with charge " <<  constit->charge() << std::endl;

                        // Get status of matched gen particle
                        int status = 1;
                        if ((*candToGenParticleMap).find(constit) != (*candToGenParticleMap).end()) {
                            edm::Ptr<pat::PackedGenParticle> matchGenParticle = (*candToGenParticleMap).at(constit);
                            status = matchGenParticle->status();
                            //std::cout << "---- constit status in the gen map " << status << std::endl;               
                        }
                        // Add particle to output collection or from HF map
                        if (status >= 100) {
                            hfConstituentsMap[status].push_back(constit);
                        }

                        else if (status == 1) {
                            //add for PF candidate collection output
                            pat::PackedCandidate constituentsPF;
                            constituentsPF.setPdgId(211);
                            constituentsPF.setCharge(constit->charge());
                            constituentsPF.setP4(constit->p4());
                            newPFCandCollection->push_back(constituentsPF);
                        }

                    } // end constit loop 

                    // Aggregate particles coming from HF decays into pseudo-B/D's and add them to the collection
                    for (auto it = hfConstituentsMap.begin(); it != hfConstituentsMap.end(); ++it) {
                        for (edm::Ptr<reco::Candidate> hfConstituent : it->second) {
                            reco::Candidate::PolarLorentzVector productLorentzVector(0., 0., 0., 0.);
                            productLorentzVector.SetPt(hfConstituent->pt());
                            productLorentzVector.SetEta(hfConstituent->eta());
                            productLorentzVector.SetPhi(hfConstituent->phi());
                            productLorentzVector.SetM(hfConstituent->mass());
                            totalPseudoHF += productLorentzVector;                           
                            pat::PackedCandidate daughter;
                            daughter.setP4(productLorentzVector);
//                            outputPseudoHF.addDaughter(daughter);
                        }
                    } // end map loop
                    

                    if (hfConstituentsMap.size() > 0) {
                        outputPseudoHF.setP4(totalPseudoHF);
                        outputPseudoHF.setPdgId(211);
                        outputPseudoHF.setMass(totalPseudoHF.mass()*(-1));
                        outputPseudoHF.setCharge(-5);
                        newPFCandCollection->push_back(outputPseudoHF);
                    }
                }
            }
                
            else {          

                int matchIndex = -1;
                if (domatch_) {   // 
                    double drMin = 100;
                    for (unsigned int imatch = 0; imatch < matchedjets->size(); ++imatch) {
                        const pat::Jet& mjet = (*matchedjets)[imatch];
                        double dr = deltaR(jet, mjet);
                        if (dr < drMin) {
                            drMin = dr;
                            matchIndex = imatch;
                        }
                    }
                }
 
                reco::TrackToGenParticleMap recoMap = isMC_ ? *candToGenParticleMap : reco::TrackToGenParticleMap();            
                const pat::Jet& mjet = (domatch_ ? (*matchedjets)[matchIndex] : jet);
                std::vector<edm::Ptr<reco::Candidate>> inputJetConstituents = jet.getJetConstituents();
                pat::PackedCandidate outputPseudoHF;

                // Particle collection to aggregate into pseudo-Bs
                std::map<int, std::vector<edm::Ptr<reco::Candidate>>> hfConstituentsMap;
                reco::Candidate::PolarLorentzVector totalPseudoHF(0., 0., 0., 0.);

                // Grab the IP and SV tag info from the jet
                const reco::CandIPTagInfo *ipTagInfo = mjet.tagInfoCandIP(ipTagInfoLabel_.c_str());
                const std::vector<reco::btag::TrackIPData> ipData = ipTagInfo->impactParameterData();
                const std::vector<edm::Ptr<reco::Candidate>> ipTracks = ipTagInfo->selectedTracks();
                const reco::CandSecondaryVertexTagInfo *svTagInfo = mjet.tagInfoCandSecondaryVertex(svTagInfoLabel_.c_str());


                for (const edm::Ptr<reco::Candidate> &constit : mjet.getJetConstituents()) {

                    if (chargedOnly_ && constit->charge() == 0) continue;
                    if (constit->charge() == 0 or constit->pt() < ptCut_) {   // Neutrals and low pT tracks are not used in aggregation, let's fill them directly in the collection
                        pat::PackedCandidate constituentsPF;
                        constituentsPF.setCharge(constit->charge());
                        constituentsPF.setP4(constit->p4());
                        constituentsPF.setPdgId(constit->pdgId());
                        newPFCandCollection->push_back(constituentsPF);
                        continue;
                    }
          
                    // Look for particle in ipTracks
                    auto itIPTrack = std::find(ipTracks.begin(), ipTracks.end(), constit);
                    if (itIPTrack == ipTracks.end()) {
                        pat::PackedCandidate constituentsPF;
                        constituentsPF.setCharge(constit->charge());
                        constituentsPF.setP4(constit->p4());
                        constituentsPF.setPdgId(constit->pdgId());
                        newPFCandCollection->push_back(constituentsPF);
                        continue;
                    }
             
                    int status = 1;
                    if (isMC_ && withTruthInfo_) {
                        if (recoMap.find(constit) != recoMap.end()) {
                            edm::Ptr<pat::PackedGenParticle> matchGenParticle = recoMap.at(constit);
                            status = matchGenParticle->status();
                        }

                    }

                    else {
                        // Initialize values 
                        const double missing_value = -1000000.;
                        float ip3dSig = missing_value;
                        float ip2dSig = missing_value;
                        float distanceToJetAxis = missing_value;
                        //bool isLepton = false;
                        bool inSV = false;
                        float svtxdls = missing_value;
                        float svtxdls2d = missing_value;
                        float svtxm = missing_value;
                        float svtxmcorr = missing_value;
                        float svtxNtrk = missing_value;
                        float svtxnormchi2 = missing_value;
                        float svtxTrkPtOverSv = missing_value;
                        float jtpt = jet.pt();

                        // Get IP info 
                        int trkIPIndex = itIPTrack - ipTracks.begin();
                        const reco::btag::TrackIPData trkIPdata = ipData[trkIPIndex];
                        ip3dSig = trkIPdata.ip3d.significance();
                        ip2dSig = trkIPdata.ip2d.significance();
                        distanceToJetAxis = trkIPdata.distanceToJetAxis.value();
                        int pdg = constit->pdgId();
                        //isLepton = (std::abs(pdg) == 11) || (std::abs(pdg) == 13);

                        // if nan go back to missing_value
                        if (ip3dSig != ip3dSig) ip3dSig = missing_value;
                        if (ip2dSig != ip2dSig) ip2dSig = missing_value;
                        if (distanceToJetAxis != distanceToJetAxis) distanceToJetAxis = missing_value;

                        // Get SV info
                        for (uint ivtx = 0; ivtx < svTagInfo->nVertices(); ivtx++) {
                            std::vector<edm::Ptr<reco::Candidate>> isvTracks = svTagInfo->vertexTracks(ivtx);
                            auto itSVTrack = std::find(isvTracks.begin(), isvTracks.end(), constit);
                            if (itSVTrack == isvTracks.end()) continue;

                            inSV = true;
                            svtxNtrk = (float) svTagInfo->nVertexTracks(ivtx);
                            Measurement1D m1D = svTagInfo->flightDistance(ivtx, 0);
                            svtxdls = m1D.significance();

                            Measurement1D m2D = svTagInfo->flightDistance(ivtx, 2);
                            svtxdls2d = m2D.significance();

                            const reco::VertexCompositePtrCandidate svtx = svTagInfo->secondaryVertex(ivtx);
                            svtxm = svtx.p4().mass();

                            double svtxpt = svtx.p4().pt();
                            svtxTrkPtOverSv = constit->pt() / svtxpt;
    
                            //mCorr=srqt(m^2+p^2sin^2(th)) + p*sin(th) -> http://arxiv.org/pdf/1504.07670v1.pdf
                            double sinth = svtx.p4().Vect().Unit().Cross((svTagInfo->flightDirection(ivtx)).unit()).Mag2();
                            sinth = sqrt(sinth);
                            double underRoot = std::pow(svtxm, 2) + (std::pow(svtxpt, 2) * std::pow(sinth, 2));
                            svtxmcorr = std::sqrt(underRoot) + (svtxpt * sinth);

                            svtxnormchi2 = svtx.vertexNormalizedChi2();
                            svtxTrkPtOverSv = constit->pt() / svtxpt;

                            break;
                        } // end vtx loop
                        if (withCuts_) {
                            if (inSV || (ip3dSig > 2.5)) {
                                status = 100;
                            }
                        }
                        else if (withTMVA_) {
                            // [TODO]: create a map of all possible variables and 
                            // then make the input only include the variables from 
                            // tmva_variable_names_
                            std::map<std::string, float> inputs;
                            inputs["trkIp3dSig"] = ip3dSig;
                            inputs["trkIp2dSig"] = ip2dSig;
                            inputs["trkDistToAxis"] = distanceToJetAxis;
                            inputs["svtxdls"] = svtxdls;
                            inputs["svtxdls2d"] = svtxdls2d;
                            inputs["svtxm"] = svtxm;
                            inputs["svtxmcorr"] = svtxmcorr;
                            inputs["svtxnormchi2"] = svtxnormchi2;
                            inputs["svtxNtrk"] = svtxNtrk;
                            inputs["svtxTrkPtOverSv"] = svtxTrkPtOverSv;
                            inputs["jtpt"] = jtpt;

                            float prediction = -99.;

                            prediction = tmvaTagger->evaluate(inputs);
                            if (prediction > -0.3) {
                                status = 100;
                            }
                        } // endif withTMVA_
                    } 

                    // Add particle to output collection or from HF map
                    if (status == 1) {
                        fastjet::PseudoJet outConstit(constit->px(), constit->py(), constit->pz(), constit->energy());
                        //add for PF candidate collection output
                        pat::PackedCandidate constituentsPF;
                        constituentsPF.setCharge(constit->charge());
                        constituentsPF.setP4(constit->p4());
                        constituentsPF.setPdgId(constit->pdgId());
                        newPFCandCollection->push_back(constituentsPF);
                    }
                    else if (status >= 100) {
                        hfConstituentsMap[status].push_back(constit);
                    }    
                } // end jet constituents loop
   
                // Aggregate particles coming from HF decays into pseudo-B/C's  and add them to the collection
                for (auto itTrackFromHF = hfConstituentsMap.begin(); itTrackFromHF != hfConstituentsMap.end(); itTrackFromHF++) {
                    for (edm::Ptr<reco::Candidate> hfConstituent : itTrackFromHF->second) {                
                        reco::Candidate::PolarLorentzVector productLorentzVector(0., 0., 0., 0.);
                        productLorentzVector.SetPt(hfConstituent->pt());
                        productLorentzVector.SetEta(hfConstituent->eta());
                        productLorentzVector.SetPhi(hfConstituent->phi());

                        // Constituent masses are random for electrons and sometimes for others. Let's set them manually.            
                        if (abs(hfConstituent->pdgId()) == 11) productLorentzVector.SetM(0.000511169);   // electron
                        else if (abs(hfConstituent->pdgId()) == 13) productLorentzVector.SetM(0.105652); // muon
                        else if (abs(hfConstituent->pdgId()) == 211) productLorentzVector.SetM(0.139526); // pion
                        else productLorentzVector.SetM(hfConstituent->mass());
            
                        totalPseudoHF += productLorentzVector;          
                        pat::PackedCandidate daughter;
                        daughter.setP4(productLorentzVector);
//                        outputPseudoHF.addDaughter(daughter);
                    }
                } // end tracks from B loop

                if (hfConstituentsMap.size() > 0) {
                    outputPseudoHF.setP4(totalPseudoHF);
                    outputPseudoHF.setPdgId(211); // Charged hadron
                    outputPseudoHF.setCharge(-5); // Set charge to avoid problems with chargedOnly selections
                    outputPseudoHF.setMass(totalPseudoHF.mass()*(-1));
                    newPFCandCollection->push_back(outputPseudoHF);
                }
            } 
        }

        // This part is directly from the pp analysis, not used at the moment
        else {
            // std::cout << "\tNot aggregating" << std::endl;
            std::vector<edm::Ptr<reco::Candidate>> constituents = {}; 
            if (doGenJets_ && isMC_) {
                const reco::GenJet *genJet = jet.genJet();
                if (genJet) constituents = genJet->getJetConstituents();
            } 
            else {
                constituents = jet.getJetConstituents();
            }

            for (edm::Ptr<reco::Candidate> constituent : constituents) {
                if ((chargedOnly_) && (constituent->charge() == 0)) continue;
                if (constituent->pt() < ptCut_) continue;
                jetConstituents.push_back(fastjet::PseudoJet(constituent->px(), constituent->py(), constituent->pz(), constituent->energy()));
            }
        }
    } // end jet loop

    //iEvent.put(std::move(newPFCandCollectionHF));
    iEvent.put(std::move(newPFCandCollection));
 
 
}


void aggregatedPFCollection::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  desc.setComment("Aggregated PF Collection");

  // Input collections
  desc.add<edm::InputTag>("jetSrc", edm::InputTag("slimmedJets"));
  desc.add<edm::InputTag>("matchTag", edm::InputTag("slimmedJets"));
  desc.add<edm::InputTag>("constitSrc", edm::InputTag("packedPFCandidates"));
  desc.add<edm::InputTag>("candToGenParticleMap", edm::InputTag("TrackToGenParticleMapProducer"));

  // Configuration parameters
  desc.add<bool>("isMC", true);
  desc.add<bool>("chargedOnly", false);
  
  desc.add<double>("ptCut", 1.);
  desc.add<double>("jetPtCut", 50.);
  desc.add<double>("trkInefRate", 0.);

  desc.add<bool>("doGenJets", false);
  desc.add<bool>("domatch", true);

  desc.add<bool>("aggregateHF", true);
  desc.add<bool>("aggregateWithTruthInfo", true);
  desc.add<bool>("aggregateWithCuts", false);
  desc.add<bool>("aggregateWithTMVA", false);
  desc.add<edm::FileInPath>("tmva_path", edm::FileInPath("RecoHI/HiJetAlgos/data/TMVAClassification_BDTG.weights.xml"));
  //  desc.add<edm::FileInPath>("tmva_path", edm::FileInPath(""));

  desc.add<std::vector<std::string>>("tmva_variables", {});
  desc.add<std::vector<std::string>>("tmva_spectators", {});
  // Tag info labels
  desc.add<std::string>("ipTagInfoLabel", "pfImpactParameter");
  desc.add<std::string>("svTagInfoLabel", "pfInclusiveSecondaryVertexFinder");

  descriptions.add("aggregatedPFCands", desc);
}

using aggregatedPFCands = aggregatedPFCollection;

// define this as a plug-in
DEFINE_FWK_MODULE(aggregatedPFCands);
