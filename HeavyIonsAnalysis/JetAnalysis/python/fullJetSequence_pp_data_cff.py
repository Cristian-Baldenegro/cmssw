import FWCore.ParameterSet.Config as cms

from HeavyIonsAnalysis.JetAnalysis.rerecoJets_cff import *
from HeavyIonsAnalysis.JetAnalysis.rerecoTracks_cff import *

from HeavyIonsAnalysis.JetAnalysis.jets.ak4CaloJetSequence_pp_data_cff import *

from HeavyIonsAnalysis.JetAnalysis.jets.ak3PFJetSequence_pp_data_cff import *
from HeavyIonsAnalysis.JetAnalysis.jets.ak4PFJetSequence_pp_data_cff import *
from HeavyIonsAnalysis.JetAnalysis.jets.ak5PFJetSequence_pp_data_cff import *

ak4PFXpatJets = cms.EDFilter("PatJetXSelector",
                             src = cms.InputTag("ak4PFpatJetsWithBtagging"),
                             offPV = cms.InputTag("offlinePrimaryVertices"),
                             cut = cms.string("pt > 5.0 && abs(rapidity()) < 3.")
                         )

ak4PFJetSequence.remove(ak4PFJetAnalyzer)
ak4PFJetSequence*=ak4PFXpatJets
ak4PFJetSequence*=ak4PFJetAnalyzer
ak4PFJetAnalyzer.jetTag = "ak4PFXpatJets"


ak3PFXpatJets = cms.EDFilter("PatJetXSelector",
                             src = cms.InputTag("ak3PFpatJetsWithBtagging"),
                             offPV = cms.InputTag("offlinePrimaryVertices"),
                             cut = cms.string("pt > 5.0 && abs(rapidity()) < 3.")
)

ak3PFJetSequence.remove(ak3PFJetAnalyzer)
ak3PFJetSequence*=ak3PFXpatJets
ak3PFJetSequence*=ak3PFJetAnalyzer
ak3PFJetAnalyzer.jetTag = "ak3PFXpatJets"


jetSequence = cms.Sequence(
    # ak4CaloJets +
    
    ak3PFJets +
    ak4PFJets +

    highPurityTracks +

    #ak4CaloJetSequence +

    ak3PFJetSequence +
    ak4PFJetSequence
)
