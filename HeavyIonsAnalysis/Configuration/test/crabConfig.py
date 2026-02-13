from WMCore.Configuration import Configuration
config = Configuration()


### General ###s
config.section_('General')
config.General.requestName = 'dijet_syncPbPbV2_R2'
config.General.workArea = 'crab_projects'
config.General.transferOutputs = True
config.General.transferLogs = True

#### JobType ####
config.section_('JobType')
config.JobType.pluginName = "Analysis"
config.JobType.psetName = "forest_miniAOD_106X_MC_withAggregation.py"
config.JobType.maxMemoryMB = 4000
config.JobType.allowUndistributedCMSSW = True
# config.JobType.numCores = 8
#config.JobType.sendExternalFolder = True # To load libxgboost.so

#### Data ####
config.section_("Data")

## ttbar 13 tev
# config.Data.inputDataset = "/TTTo2L2Nu_TuneCP5_13TeV-powheg-pythia8/RunIISummer20UL17MiniAODv2-106X_mc2017_realistic_v9-v1/MINIAODSIM"

## qcd pp ref
config.Data.inputDataset = "/QCD_pThat-15_Dijet_TuneCP5_5p02TeV-pythia8/RunIISummer20UL17pp5TeVMiniAODv2-106X_mc2017_realistic_forppRef5TeV_v3-v3/MINIAODSIM"
#config.Data.inputDataset = "/QCD_pThat-15_bJet_TuneCP5_5p02TeV-pythia8/RunIISummer20UL17pp5TeVMiniAODv2-106X_mc2017_realistic_forppRef5TeV_v3-v3/MINIAODSIM"
# config.Data.inputDataset = "/QCD_pThat-15_Mujet_TuneCP5_5p02TeV-pythia8/RunIISummer20UL17pp5TeVMiniAODv2-106X_mc2017_realistic_forppRef5TeV_v3-v2/MINIAODSIM"

## random qcd miniaod sample
# config.Data.inputDataset = "/QCD_Pt_1400to1800_TuneCP5_13TeV_pythia8/RunIISummer19UL17MiniAOD-106X_mc2017_realistic_v6-v2/MINIAODSIM"

## private herwig pp ref
# config.Data.inputDataset = "/Herwig_CH3_bjet_5TeV/mnguyen-Herwig_CH3_bjet_5TeV_MINI_v7-0574b4ed1d118382056a950d3f8bdf1b/USER"
# config.Data.inputDataset = "/Herwig_CH3_qcd_5TeV/mnguyen-Herwig_CH3_qcd_5TeV_MINI_v7-0574b4ed1d118382056a950d3f8bdf1b/USER"

## official herwig pp ref 
# config.Data.inputDataset = "/QCD_PthatGT15_TuneCH3_5p02TeV_herwig7/RunIISummer20UL17pp5TeVMiniAODv2-seedChange_106X_mc2017_realistic_forppRef5TeV_v3-v2/MINIAODSIM"
# config.Data.inputDataset = "/QCD_PthatGT15_bJet_TuneCH3_5p02TeV_herwig7/RunIISummer20UL17pp5TeVMiniAODv2-106X_mc2017_realistic_forppRef5TeV_v3-v2/MINIAODSIM"

# config.Data.inputDBS = "global"
# config.Data.inputDBS = "phys03"
# config.Data.splitting = "FileBased"
config.Data.splitting = "LumiBased"
# config.Data.splitting = "Automatic"
config.Data.unitsPerJob = 20
config.Data.totalUnits = -1
config.Data.publication = False
config.Data.outLFNDirBase = '/store/user/mnguyen/bJet2025'
config.Data.outputDatasetTag = config.General.requestName
config.Data.ignoreLocality = True
config.Data.allowNonValidInputDataset = True

## [DEBUG]: Run the analysis on a single file
# both ways work for finding the files
# inputDataFiles = [
#                   "/store/himc/RunIIpp5Spring18DR/QCD_pThat-15_bJet_TuneCP5_5p02TeV_pythia8/AODSIM/94X_mc2017_realistic_forppRef5TeV_v1-v1/230000/92D50574-667C-E911-B0CA-1866DAEECF18.root",
#                   "root://polgrid4.in2p3.fr//store/himc/RunIIpp5Spring18DR/QCD_pThat-15_bJet_TuneCP5_5p02TeV_pythia8/AODSIM/94X_mc2017_realistic_forppRef5TeV_v1-v1/230000/2894771A-9D88-E911-8F7B-1866DAEB1FCC.root"
#                  ]
# config.Data.userInputFiles = inputDataFiles
# config.Data.outputPrimaryDataset = "SingleFileCrabTest"

#### Site ####
config.section_('Site')
config.Site.whitelist = ['T2_FR_*', "T2_CH_*"]
config.Site.storageSite = 'T2_FR_GRIF_LLR'
