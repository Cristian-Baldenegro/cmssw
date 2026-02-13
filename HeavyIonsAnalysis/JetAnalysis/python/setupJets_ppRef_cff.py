import FWCore.ParameterSet.Config as cms

def candidateBtaggingMiniAOD(process, isMC = True, jetPtMin = 15, jetCorrLevels = ['L2Relative', 'L3Absolute'], doBtagging = False, labelR = "0", doAggregation = False):
    # DeepNtuple settings
    jetR = 0.1*float(labelR)
    if labelR == "0": jetR = 0.4

    jetCorrectionsAK4 = ('AK4PFchs' if labelR == "0" else 'AK'+labelR+'PFchs', jetCorrLevels, 'None')


    tmva_variables = ["trkIp3dSig", "trkIp2dSig", "trkDistToAxis",
                      "svtxdls", "svtxdls2d", "svtxm", "svtxmcorr",
                      "svtxnormchi2", "svtxNtrk", "svtxTrkPtOverSv",
                      "jtpt"]


    if doBtagging:

        bTagInfos = ['pfParticleNetAK4TagInfos']
        bTagDiscriminators = ['pfParticleNetAK4JetTags:probpu', 'pfParticleNetAK4JetTags:probb', 'pfParticleNetAK4JetTags:probg',
                              'pfParticleNetAK4JetTags:probcc', 'pfParticleNetAK4JetTags:probbb', 'pfParticleNetAK4JetTags:probc',
                              'pfParticleNetAK4JetTags:probuds', 'pfParticleNetAK4JetTags:probundef', 'pfJetProbabilityBJetTags']

    else: 
        bTagInfos = ['None']
        bTagDiscriminators = ['None']

    # Create gen-level information
    if isMC:
        from PhysicsTools.PatAlgos.mcMatchLayer0.jetFlavourId_cff import patJetPartonsLegacy

        process.allPartons = patJetPartonsLegacy.clone(
            src = "prunedGenParticles"
        )

        # Define generator level jets without neutrinos
        from RecoJets.JetProducers.ak4GenJets_cfi import ak4GenJets

        process.packedGenParticlesForJetsNoNu = cms.EDFilter("CandPtrSelector",
            src = cms.InputTag("packedGenParticles"),
            cut = cms.string("abs(pdgId) != 12 && abs(pdgId) != 14 && abs(pdgId) != 16")
        )

        setattr(process,"ak"+labelR+"GenJetsReclusterNoNu",
                ak4GenJets.clone(
                    src = 'packedGenParticlesForJetsNoNu',
                    rParam = jetR
                )
        )

        setattr(process,"ak"+labelR+"aggregatedGenJetsNoNu",
                ak4GenJets.clone(
                    src = 'aggregatedGenLevel',
                    rParam = jetR
                )
        )

         # We need to be careful not to override the previous genTask in case several different jet radii are defined in the forest configuration file
        if hasattr(process, "genTask"):
            process.genTask.add( getattr(process,"ak"+labelR+"GenJetsReclusterNoNu"))
        else:
            process.genTask = cms.Task( process.allPartons,  getattr(process,"ak"+labelR+"aggregatedGenJetsNoNu"), process.packedGenParticlesForJetsNoNu, getattr(process,"ak"+labelR+"GenJetsReclusterNoNu"))


    # Create unsubtracted reco jets

    #from PhysicsTools.PatAlgos.producersLayer1.jetProducer_cff import ak4PFJets
    from RecoJets.JetProducers.ak4PFJets_cfi import ak4PFJets
    setattr(process, "ak"+labelR+"PFUnsubJets", 
            ak4PFJets.clone(
                src = 'packedPFCandidates',
                jetPtMin = 5.,  # set lower than subtracted version
                rParam = jetR
            )
    )
    
    if isMC:
        from PhysicsTools.JetMCAlgos.HadronAndPartonSelector_cfi import selectedHadronsAndPartons
        from PhysicsTools.JetMCAlgos.AK4PFJetsMCFlavourInfos_cfi import ak4JetFlavourInfos
        process.selectedHadronsAndPartons = selectedHadronsAndPartons.clone(particles = "prunedGenParticles")
        setattr(process,"ak"+labelR+"PFFlavourInfos",
                ak4JetFlavourInfos.clone(
                    jets = "ak"+labelR+"PFJetsCHS",
                    partons = "selectedHadronsAndPartons:algorithmicPartons",
                    hadronFlavourHasPriority = True,
                    rParam = jetR
                )
        )
        process.genTask.add(process.selectedHadronsAndPartons)
        process.genTask.add(getattr(process,"ak"+labelR+"PFFlavourInfos"))

    matchedGenJets = ""
    if isMC:
        if labelR == "0": matchedGenJets = "slimmedGenJets"
        else: matchedGenJets = "ak"+labelR+"GenJetsReclusterNoNu"


    svSource = cms.InputTag("slimmedSecondaryVertices")

    from PhysicsTools.PatAlgos.tools.jetTools import addJetCollection
    addJetCollection(
        process,
        postfix            = "UnsubJets",
        labelName          = str("AK"+labelR+"PF"),
        jetSource          = cms.InputTag("ak"+labelR+"PFUnsubJets"),
        algo               = "ak", #name of algo must be in this format
        rParam             = jetR,
        pvSource           = cms.InputTag("offlineSlimmedPrimaryVertices"),
        pfCandidates       = cms.InputTag("packedPFCandidates"),
        svSource           = svSource,
        muSource           = cms.InputTag("slimmedMuons"),
        elSource           = cms.InputTag("slimmedElectrons"),
        getJetMCFlavour    = isMC,
        genJetCollection   = cms.InputTag(matchedGenJets),
        #genParticles       = cms.InputTag("hiSignalGenParticles" if isMC else ""),
        genParticles       = cms.InputTag("prunedGenParticles" if isMC else ""),
        jetCorrections     = ('AK4PFchs' if labelR=='0' else 'AK'+labelR+'PFchs',) + jetCorrectionsAK4[1:],
    )

    getattr(process,"patJetsAK"+labelR+"PFUnsubJets").useLegacyJetMCFlavour = False


    process.patAlgosToolsTask.add(getattr(process,"ak"+labelR+"PFUnsubJets"))


    # Create CHS subtracted reco jets
    from PhysicsTools.PatAlgos.tools.jetTools import addJetCollection
    addJetCollection(
        process,
        postfix            = "",
        labelName          = "AK"+labelR+"PFCHS",
        jetSource          = cms.InputTag("ak"+labelR+"PFJetsCHS"),
        algo               = "ak", #name of algo must be in this format
        rParam             = jetR,
        pvSource           = cms.InputTag("offlineSlimmedPrimaryVertices"),
        pfCandidates       = cms.InputTag("packedPFCandidates"),
        svSource           = svSource,
        muSource           = cms.InputTag("slimmedMuons"),
        elSource           = cms.InputTag("slimmedElectrons"),
        getJetMCFlavour    = isMC,
        genJetCollection   = cms.InputTag(matchedGenJets),
        #genParticles       = cms.InputTag("hiSignalGenParticles" if isMC else ""),
        genParticles       = cms.InputTag("prunedGenParticles" if isMC else ""),
        jetCorrections     = jetCorrectionsAK4,
    )
    if labelR == "0": getattr(process,"patJetsAK"+labelR+"PFCHS").embedPFCandidates = True # not working with CHS jet reclustering


    ##
    ## pokusaj
    setattr(process,"pfImpactParameterTagInfos",
            cms.EDProducer("CandIPProducer",
                           candidates = cms.InputTag("packedPFCandidates"),
                           computeGhostTrack = cms.bool(True),
                           computeProbabilities = cms.bool(True),
                           ghostTrackPriorDeltaR = cms.double(0.03),
                           jetDirectionUsingGhostTrack = cms.bool(False),
                           jetDirectionUsingTracks = cms.bool(False),
                           jets = cms.InputTag("ak"+labelR+"PFJetsCHS"),
                           maxDeltaR = cms.double(jetR),
                           maximumChiSquared = cms.double(5.0),
                           maximumLongitudinalImpactParameter = cms.double(17.0),
                           maximumTransverseImpactParameter = cms.double(0.2),
                           minimumNumberOfHits = cms.int32(0),
                           minimumNumberOfPixelHits = cms.int32(1),
                           minimumTransverseMomentum = cms.double(1.0),
                           primaryVertex = cms.InputTag("offlineSlimmedPrimaryVertices"),
                           useTrackQuality = cms.bool(False)
                       )
        )


    from RecoBTag.SecondaryVertex.pfInclusiveSecondaryVertexFinderTagInfos_cfi import pfInclusiveSecondaryVertexFinderTagInfos
    setattr(process,"pfInclusiveSecondaryVertexFinderTagInfos",
            pfInclusiveSecondaryVertexFinderTagInfos.clone(
                trackIPTagInfos = cms.InputTag("pfImpactParameterTagInfos"),
                extSVCollection = "slimmedSecondaryVertices"
            )
    )

    getattr(process,"patJetsAK"+labelR+"PFCHS").addTagInfos = True
    getattr(process,"patJetsAK"+labelR+"PFCHS").addBTagInfo = True
    getattr(process,"patJetsAK"+labelR+"PFCHS").tagInfoSources = cms.VInputTag("pfImpactParameterTagInfos","pfInclusiveSecondaryVertexFinderTagInfos")
    getattr(process,"patJetsAK"+labelR+"PFCHS").addDiscriminators = False
    process.patAlgosToolsTask.add(getattr(process,"pfImpactParameterTagInfos"))
    process.patAlgosToolsTask.add(getattr(process,"pfInclusiveSecondaryVertexFinderTagInfos"))



    ## aggregation
    if doAggregation:
        process.load("RecoHI.HiJetAlgos.TrackToGenParticleMapProducer_cfi")
        getattr(process,"TrackToGenParticleMapProducer").jetSrc = "patJetsAK"+labelR+"PFCHS"
        process.TrackToGenParticleMapProducer.genParticleSrc = cms.InputTag("HFdecayProductTagger", "patPackedGenParticles")
        process.TrackToGenParticleMapProducer.chargedOnly = True
        process.genJetSequence += process.TrackToGenParticleMapProducer
        ## Creates the genConstitToGenParticleMap and trackToGenParticleMap

        process.load("RecoHI.HiJetAlgos.aggregatedPFCollection_cfi")

        process.aggregatedGenLevel  = process.aggregatedPFCands.clone(
            chargedOnly = cms.bool(False),
            aggregateHF = cms.bool(True),
            jetSrc = cms.InputTag("patJetsAK"+labelR+"PFCHS"),
            constitSrc = cms.InputTag("packedGenParticles"),
            doGenJets = cms.bool(True),
            aggregateWithTruthInfo = cms.bool(True),
            aggregateWithCuts = cms.bool(False),
            aggregateWithTMVA = cms.bool(False),
            candToGenParticleMap = cms.InputTag("TrackToGenParticleMapProducer", "genConstitToGenParticleMap"),
        )

        process.aggregatedRecoLevel = process.aggregatedPFCands.clone(
            aggregateHF = cms.bool(True),
            chargedOnly = cms.bool(False),
            jetSrc = cms.InputTag("patJetsAK"+labelR+"PFCHS"),
            constitSrc = cms.InputTag("packedPFCandidates"),
            doGenJets = cms.bool(False),
            domatch = cms.bool(False),
            aggregateWithTruthInfo = cms.bool(True),
            aggregateWithCuts = cms.bool(False),
            aggregateWithTMVA = cms.bool(False),
            tmva_path = cms.FileInPath("RecoHI/HiJetAlgos/data/TMVAClassification_BDTG.weights.xml"),
            tmva_variables =  cms.vstring(tmva_variables),
            candToGenParticleMap = cms.InputTag("TrackToGenParticleMapProducer", "trackToGenParticleMap"),
        )

        process.aggregatedRecoLevel.ipTagInfoLabel = "pfImpactParameter"
        process.aggregatedRecoLevel.svTagInfoLabel = "pfInclusiveSecondaryVertexFinder"

        process.patAlgosToolsTask.add(getattr(process,"aggregatedRecoLevel"))
        if isMC:
            process.patAlgosToolsTask.add(getattr(process,"aggregatedGenLevel"))



    #Create CHS subtracted reco jets with aggregation

    from PhysicsTools.PatAlgos.tools.jetTools import addJetCollection
    addJetCollection(
        process,
        postfix            = "",
        labelName          = "AK"+labelR+"PFCHSAggr",
        jetSource          = cms.InputTag("ak"+labelR+"PFJetsCHSAggr"),
        algo               = "ak", #name of algo must be in this format
        rParam             = jetR,
        pvSource           = cms.InputTag("offlineSlimmedPrimaryVertices"),
        #pfCandidates       = cms.InputTag("packedPFCandidates"),
        pfCandidates       = cms.InputTag("aggregatedRecoLevel" if doAggregation else "packedPFCandidates"),
        svSource           = svSource,
        muSource           = cms.InputTag("slimmedMuons"),
        elSource           = cms.InputTag("slimmedElectrons"),
        getJetMCFlavour    = isMC,
        #genJetCollection   = cms.InputTag(matchedGenJets),
        genJetCollection   = cms.InputTag("ak"+labelR+"aggregatedGenJetsNoNu" if doAggregation else matchedGenJets),
        #genParticles       = cms.InputTag("hiSignalGenParticles" if isMC else ""),
        genParticles       = cms.InputTag("prunedGenParticles" if isMC else ""),
        jetCorrections     = jetCorrectionsAK4,
    )


    if not isMC:
        for label in ["patJetsAK"+labelR+"PFUnsubJets", "patJetsAK"+labelR+"PFCHS", "patJetsAK"+labelR+"PFCHSAggr"]:
            getattr(process, label).addGenJetMatch = False
            getattr(process, label).addGenPartonMatch = False
            getattr(process, label).embedGenJetMatch = False
            getattr(process, label).embedGenPartonMatch = False
            getattr(process, label).genJetMatch = ""
            getattr(process, label).genPartonMatch = ""

    # left here for reference in case we want to move reclustering here
    #from CommonTools.ParticleFlow.pfCHS_cff import pfCHS
    from HeavyIonsAnalysis.JetAnalysis.pfCHS_cff import pfCHS 
    process.pfCHS = pfCHS.clone()
    from RecoJets.JetProducers.ak4PFJets_cfi import ak4PFJetsCHS
    setattr(process,"ak"+labelR+"PFJetsCHS",
            ak4PFJetsCHS.clone(
                src = "pfCHS",
                jetPtMin = jetPtMin,
                rParam = jetR
            )
    )
    setattr(process,"ak"+labelR+"PFJetsCHSAggr",
            #ak4PFJetsCHS.clone(
            ak4PFJets.clone(
                src = 'aggregatedRecoLevel' if doAggregation else 'pfCHS',
#                src = 'pfCHS',
                jetPtMin = jetPtMin,
                rParam = jetR
            )
    )

    for mod in ["pfCHS","ak"+labelR+"PFJetsCHS", "ak"+labelR+"PFJetsCHSAggr"]:
        process.patAlgosToolsTask.add(getattr(process, mod))


    # Create b-tagging sequence ----------------
    from PhysicsTools.PatAlgos.tools.jetTools import updateJetCollection
    updateJetCollection(
        process,
        labelName = "AK"+labelR+"PFCHSBtag",
        #jetSource = cms.InputTag("slimmedJets" if labelR == "0" else "patJetsAK"+labelR+"PFCHSAggr"), 
        jetSource = cms.InputTag("slimmedJets" if labelR == "0" else "patJetsAK"+labelR+"PFCHS"),   # For b-tagging need non-aggregated jets
        jetCorrections = jetCorrectionsAK4,
        pfCandidates = cms.InputTag('packedPFCandidates'),
        pvSource = cms.InputTag("offlineSlimmedPrimaryVertices"),
        svSource = svSource,
        muSource = cms.InputTag('slimmedMuons'),
        elSource = cms.InputTag('slimmedElectrons'),
        btagInfos = bTagInfos,
        btagDiscriminators = bTagDiscriminators,
        explicitJTA = False
    )

    '''
    setattr(process,"unsubUpdatedPatJetsAK"+labelR+"PFCHSAggr",
            cms.EDProducer("JetMatcherDR",
                           source = cms.InputTag("patJetsAK"+labelR+"PFCHSAggr"),
                           matched = cms.InputTag("patJetsAK"+labelR+"PFUnsubJets")
                       )
        )
    process.patAlgosToolsTask.add(getattr(process,"unsubUpdatedPatJetsAK"+labelR+"PFCHSAggr"))
    '''
    getattr(process,"pfParticleNetAK4TagInfosAK"+labelR+"PFCHSBtag").jet_radius = jetR

    
    getattr(process,"patJetsAK"+labelR+"PFCHSAggr").addTagInfos = True
    getattr(process,"patJetsAK"+labelR+"PFCHSAggr").addBTagInfo = True
    getattr(process,"patJetsAK"+labelR+"PFCHSAggr").tagInfoSources = cms.VInputTag("pfImpactParameterTagInfos","pfInclusiveSecondaryVertexFinderTagInfos")
    getattr(process,"patJetsAK"+labelR+"PFCHSAggr").addDiscriminators = False
    #process.patAlgosToolsTask.add(getattr(process,"pfImpactParameterTagInfos"))
    #process.patAlgosToolsTask.add(getattr(process,"pfInclusiveSecondaryVertexFinderTagInfos"))
    '''
    # Match with unsubtracted jets
    setattr(process,"unsubAK"+labelR+"JetMap",
            getattr(process,"unsubUpdatedPatJetsAK"+labelR+"PFCHS").clone(
                source = "selectedUpdatedPatJetsAK"+labelR+"PFCHS"
            )
        )

    process.patAlgosToolsTask.add(getattr(process,"unsubAK"+labelR+"JetMap"))
    '''

    # Add extra b tagging algos
    from RecoBTag.ImpactParameter.pfJetProbabilityBJetTags_cfi import pfJetProbabilityBJetTags
    setattr(process,"pfJetProbabilityBJetTagsAK"+labelR+"PFCHSBtag",
           # pfJetProbabilityBJetTags.clone(tagInfos = ["pfImpactParameterTagInfosAK"+labelR+"PFCHSBtag", "pfInclusiveSecondaryVertexFinderTagInfosAK"+labelR+"PFCHSBtag"])
            pfJetProbabilityBJetTags.clone(tagInfos = ["pfImpactParameterTagInfosAK"+labelR+"PFCHSBtag"])

        )

    if doBtagging:
        process.patAlgosToolsTask.add(getattr(process,"pfJetProbabilityBJetTagsAK"+labelR+"PFCHSBtag"))
    
    # Associate to forest sequence
    if isMC:
        process.forest.associate(process.genTask)
    process.forest.associate(process.patAlgosToolsTask)
