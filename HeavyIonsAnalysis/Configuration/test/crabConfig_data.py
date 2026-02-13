from WMCore.Configuration import Configuration
config = Configuration()


### General ###
config.section_('General')
config.General.requestName = 'bJet2017G_HighEGJet_aggrTMVA_withMCJPCalibration'
config.General.workArea = 'crab_projects'
config.General.transferOutputs = True
config.General.transferLogs = True

#### JobType ####
config.section_('JobType')
config.JobType.pluginName = "Analysis"
config.JobType.psetName = "forest_miniAOD_106X_data.py"
#config.JobType.maxMemoryMB = 16000
#config.JobType.maxMemoryMB = 3000
#config.JobType.maxMemoryMB = 4000
config.JobType.allowUndistributedCMSSW = True
#config.JobType.numCores = 8
config.JobType.sendExternalFolder = True # To load libxgboost.so

#### Data ####
config.section_("Data")
# config.Data.inputDataset = "/LowEGJet/Run2017G-UL2017_MiniAODv2-v2/MINIAOD"
config.Data.inputDataset = "/HighEGJet/Run2017G-UL2017_MiniAODv2-v2/MINIAOD"
# config.Data.inputDataset = "/SingleMuon/Run2017G-UL2017_MiniAODv2-v1/MINIAOD"
# config.Data.inputDataset = "/SingleMuonTnP/Run2017G-09Aug2019_UL2017-v1/MINIAOD"
config.Data.lumiMask = 'Cert_306546-306826_5TeV_EOY2017ReReco_Collisions17_JSON.txt'
# config.Data.lumiMask = 'crab_projects/crab_bJet2017G_HighEGJet_muTrigger_v2/results/notFinishedLumis.json'
config.Data.inputDBS = "global"
# config.Data.splitting = "LumiBased"
config.Data.splitting = "FileBased"
config.Data.unitsPerJob = 1
#config.Data.totalUnits = 500
config.Data.publication = False
config.Data.outLFNDirBase = '/store/user/lkalipol/bJet2017G/' 
config.Data.outputDatasetTag = config.General.requestName
config.Data.ignoreLocality = True
config.Data.allowNonValidInputDataset = True

#### Site ####
config.section_('Site')
config.Site.whitelist = ['T2_FR_*', "T2_US_*","T2_CH_CERN", "T2_IN_TIFR"]
config.Site.storageSite = 'T2_FR_GRIF_LLR'
