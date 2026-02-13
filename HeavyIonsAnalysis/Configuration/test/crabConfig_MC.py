#file path will be:
#outLFNDirBase/inputDataset/requestName/time_tag/...
from CRABClient.UserUtilities import config
config = config()
config.General.transferOutputs = True
config.General.requestName = 'bjets_ppref_trueAggregation_v3'

config.JobType.psetName = 'forest_miniAOD_106X_MC_withAggregation.py'
config.JobType.pluginName = 'Analysis'
config.Data.inputDataset = '/QCD_pThat-15_bJet_TuneCP5_5p02TeV-pythia8/RunIISummer20UL17pp5TeVMiniAODv2-106X_mc2017_realistic_forppRef5TeV_v3-v3/MINIAODSIM'
#config.Data.inputDataset = '/QCD_pThat-15_Dijet_TuneCP5_5p02TeV-pythia8/RunIISummer20UL17pp5TeVMiniAODv2-106X_mc2017_realistic_forppRef5TeV_v3-v3/MINIAODSIM'
config.JobType.outputFiles = ['HiForestMiniAOD.root']

config.Data.publication = False
config.Data.totalUnits = -1
config.Data.unitsPerJob = 10000
config.Data.splitting = 'EventAwareLumiBased'
config.JobType.maxMemoryMB = 2000
#config.Data.totalUnits = 2050
#config.Data.unitsPerJob = 1
#config.Data.splitting = 'Automatic'
#config.Data.splitting = 'FileBased'

config.Data.outLFNDirBase = '/store/user/jmijusko/bjets_pp/MC_Pythia/'
config.section_('Site')
config.Site.storageSite = 'T2_CH_CERN'

