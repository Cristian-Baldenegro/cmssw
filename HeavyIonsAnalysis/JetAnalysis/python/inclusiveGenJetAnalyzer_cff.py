import FWCore.ParameterSet.Config as cms

inclusiveGenJetAnalyzer = cms.EDAnalyzer(
    "HiInclusiveGenJetAnalyzer",
    genJets = cms.InputTag("ak4GenJetsNoNu"),
    genParticles = cms.InputTag("genParticles"),
    jetFlavourInfos = cms.InputTag("jetFlavourInfosAK4PFJets")
)
