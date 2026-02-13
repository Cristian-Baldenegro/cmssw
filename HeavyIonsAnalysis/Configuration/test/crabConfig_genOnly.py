from WMCore.Configuration import Configuration
config = Configuration()


### General ###s
config.section_('General')
config.General.requestName = 'pythia8_bjet_Bothdown'
config.General.workArea = 'crab_projects'
config.General.transferOutputs = True
config.General.transferLogs = True

#### JobType ####
config.section_('JobType')
config.JobType.pluginName = "Analysis"
config.JobType.psetName = "forest_miniAOD_106X_genOnly.py"
config.JobType.maxMemoryMB = 4000
config.JobType.allowUndistributedCMSSW = True
# config.JobType.numCores = 8
config.JobType.sendExternalFolder = True # To load libxgboost.so

#### Data ####
config.section_("Data")

config.Data.inputDataset = "/Pythia8_bJets_Bothdown/mnguyen-Pythia8_bJets_Bothdown-66fb0ebbe374e5047043fbf991faafaf/USER"
# config.Data.inputDataset = "/Pythia8_bJets_Bothup/mnguyen-Pythia8_bJets_Bothup-f5f3b0746ac0751ed33f4d883bb22f04/USER"
# config.Data.inputDataset = "/Pythia8_bJets_FSRdown/mnguyen-Pythia8_bJets_FSRdown-1898fa3a02b22eccdbc70bf22bef8336/USER"
# config.Data.inputDataset = "/Pythia8_bJets_FSRup/mnguyen-Pythia8_bJets_FSRup-760eeeb0e6865267ec10aff2ec3a2394/USER"
# config.Data.inputDataset = "/Pythia8_bJets_ISRdown/mnguyen-Pythia8_bJets_ISRdown-3d33e3d095daab1a5bcde6ce701852a9/USER"
# config.Data.inputDataset = "/Pythia8_bJets_ISRup/mnguyen-Pythia8_bJets_ISRup-233f85ca15aa5d493263752803a7da4d/USER"
# config.Data.inputDataset = "/Pythia8_inclusiveJets_Bothdown/mnguyen-Pythia8_inclusiveJets_Bothdown-7048840eca1b4e67e7820d3a26b8ff13/USER"
# config.Data.inputDataset = "/Pythia8_inclusiveJets_Bothup/mnguyen-Pythia8_inclusiveJets_Bothup-75063a6b0614dc2fd887935a9383a537/USER"
# config.Data.inputDataset = "/Pythia8_inclusiveJets_FSRdown/mnguyen-Pythia8_inclusiveJets_FSRdown-09f667cc1afa26a104d33526560db735/USER"
# config.Data.inputDataset = "/Pythia8_inclusiveJets_FSRup/mnguyen-Pythia8_inclusiveJets_FSRup-76f464b267ebce0f4991d239353e1c26/USER"
# config.Data.inputDataset = "/Pythia8_inclusiveJets_ISRdown/mnguyen-Pythia8_inclusiveJets_ISRdown-483a371f106ade2341e311f68a558864/USER"
# config.Data.inputDataset = "/Pythia8_inclusiveJets_ISRup/mnguyen-Pythia8_inclusiveJets_ISRup-5b4717c921afe2029def22a7a999e144/USER" # 500 files

# config.Data.inputDBS = "global"
config.Data.inputDBS = "phys03"
config.Data.splitting = "FileBased"
# config.Data.splitting = "LumiBased"
# config.Data.splitting = "Automatic"
config.Data.unitsPerJob = 1
config.Data.totalUnits = -1
config.Data.publication = False
config.Data.outLFNDirBase = '/store/user/lkalipol/bJet2023'
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
