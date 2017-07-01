import FWCore.ParameterSet.Config as cms

ctppsStraightTrackAligner = cms.EDAnalyzer("CTPPSStraightTrackAligner",
    verbosity = cms.untracked.uint32(0),
    factorizationVerbosity = cms.untracked.uint32(0),

    # ---------- input and event selection ----------

    tagStripHits = cms.InputTag("ctppsFastLocalSimulation"),
    tagDiamondHits = cms.InputTag("ctppsFastLocalSimulation"),
    tagPixelHits = cms.InputTag("ctppsFastLocalSimulation"),

    # list of RPs for which the alignment parameters shall be optimized
    rpIds = cms.vuint32(),

    # list of planes to be excluded from processing
    excludePlanes = cms.vuint32(),

    # maximum number of selected events
    maxEvents = cms.int32(-1),  # -1 means unlimited


    # ---------- event selection ----------

    # TODO: describe
    maxResidualToSigma = cms.double(3),
    minimumHitsPerProjectionPerRP = cms.uint32(4),

    # skip events with hits in both top and bottom RPs
    removeImpossible = cms.bool(True),

    # minimum required number of units active
    requireNumberOfUnits = cms.uint32(2),

    # require combination of top+horizontal or bottom+horizontal RPs
    requireOverlap = cms.bool(False),

    # require at least 3 RPs active when track in the horizontal-vertical overlap
    requireAtLeast3PotsInOverlap = cms.bool(True),

    # list of RP sets that are accepted irrespective of the "require" settings
    #     the sets should be separated by ";"
    #     within each set, RP ids are separated by ","
    #     example: "103,104;120,121"
    additionalAcceptedRPSets = cms.string(""),

    # track fit quality requirements
    cutOnChiSqPerNdf = cms.bool(True),
    chiSqPerNdfCut = cms.double(10),

    # track angular requirements
    maxTrackAx = cms.double(1E6),
    maxTrackAy = cms.double(1E6),


    # ---------- constraints ----------

    # choices: homogeneous, fixedDetectors, standard
    constraintsType = cms.string("standard"),

    useExtendedRotZConstraint = cms.bool(True),
    useZeroThetaRotZConstraint = cms.bool(False),
    useExtendedShZConstraints = cms.bool(True),
    useExtendedRPShZConstraint = cms.bool(True),
    oneRotZPerPot = cms.bool(False),
    useEqualMeanUMeanVRotZConstraint = cms.bool(False),
    
    # C^T A values (i.e. not theta values)
    homogeneousConstraints = cms.PSet(
      ShR_values = cms.vdouble(0., 0., 0., 0.),
      ShZ_values = cms.vdouble(0., 0., 0., 0.),
      RotZ_values = cms.vdouble(0., 0.),
      RPShZ_values = cms.vdouble(0., 0.)
    ),
    
    # values in um and m rad
    fixedDetectorsConstraints = cms.PSet(
      ShR = cms.PSet(
        ids = cms.vuint32(1220, 1221, 1228, 1229),
        values = cms.vdouble(0, 0, 0, 0),
      ),
      ShZ = cms.PSet(
        ids = cms.vuint32(1200, 1201, 1208, 1209),
        values = cms.vdouble(0, 0, 0, 0),
      ),
      RotZ = cms.PSet(
        ids = cms.vuint32(1200, 1201),
        values = cms.vdouble(0, 0),
      ),
      RPShZ = cms.PSet(
        ids = cms.vuint32(1200), # number of any plane in the chosen RP
        values = cms.vdouble(0),
      ),
    ),

    standardConstraints = cms.PSet(
        units = cms.vuint32(1, 21)
    ),


    # ---------- solution parameters ----------

    # a characteristic z in mm
    z0 = cms.double(0.0),

    # what to be resolved
    resolveShR = cms.bool(True),
    resolveShZ = cms.bool(False),
    resolveRotZ = cms.bool(False),
    resolveRPShZ = cms.bool(False),

    # suitable value for station alignment
    singularLimit = cms.double(1E-8),


    # ---------- algorithm configuration ----------

    # available algorithms: Ideal, Jan
    algorithms = cms.vstring("Jan"),
    
    IdealResult = cms.PSet(
      useExtendedConstraints = cms.bool(True)
    ),

    JanAlignmentAlgorithm = cms.PSet(
      weakLimit = cms.double(1E-6),
      stopOnSingularModes = cms.bool(True),
      buildDiagnosticPlots = cms.bool(True),
    ),


    # ---------- output configuration ----------

    saveIntermediateResults = cms.bool(True),
    taskDataFileName = cms.string(''),

    buildDiagnosticPlots = cms.bool(True),
    diagnosticsFile = cms.string(''),

    fileNamePrefix = cms.string(''),
    cumulativeFileNamePrefix = cms.string('cumulative_results_'),
    expandedFileNamePrefix = cms.string('cumulative_expanded_results_'),
    factoredFileNamePrefix = cms.string('cumulative_factored_results_'),
    preciseXMLFormat = cms.bool(False)
)
