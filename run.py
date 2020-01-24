import os
import configparser
import subprocess
import time
import shutil
import getopt
import sys
import csv
from glob import glob

#upperBound = 350
#lowerBound = 250
#increment = 50
#paramList1 = [*range(lowerBound, upperBound+1, increment)]
paramPrecision = 3
#paramList1 = [x * 0.001 for x in range(1,11)]
#paramList1 = [x * 200 for x in range(1,11)]
#paramList1 = [x for x in range(1,21)]
#paramList1 = [x for x in range(1,3)]
paramList1 = [10, 50, 100, 250, 1000] 
paramList1b = ['1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 10 10 10 10 10 10 10 10 10 10 10 10 10 10 10 10', 
        '1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 50 50 50 50 50 50 50 50 50 50 50 50 50 50 50 50',
        '1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 100 100 100 100 100 100 100 100 100 100 100 100 100 100 100 100',
        '1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 250 250 250 250 250 250 250 250 250 250 250 250 250 250 250 250',
        '1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1000 1000 1000 1000 1000 1000 1000 1000 1000 1000 1000 1000 1000 1000 1000 1000',
        ]
livingProcesses = []
namesOfLivingProcesses = []
cntLivingProcesses = 0
projectDirectories = []

def read_config():
    config = configparser.ConfigParser()
    config.read('config.ini')
    return config

def write_config(config):
    # Turn the string into an integer, increment it, and then turn it back into
    # a string with three spaces worth of left-hand zero padding (eg. 007)
    #runNum = str(int(config['SETTINGS']['run number']) + 1).zfill(3)
    # Update the config file with the new run number
    config.set('SETTINGS', 'run number', str(int(config['SETTINGS']['run number']) + 1).zfill(3))
    with open('config.ini', 'w') as configfile:
        config.write(configfile)

def update_rand_seed(config, randSeed):
    # Turn the string into an integer, increment it, and then turn it back into
    # a string with three spaces worth of left-hand zero padding (eg. 007)
    #runNum = str(int(config['SETTINGS']['run number']) + 1).zfill(3)
    # Update the config file with the new run number
    config.set('SETTINGS', 'rand seed', str(randSeed))
    with open('config.ini', 'w') as configfile:
        config.write(configfile)

def start_sim(files, ver, param1):
    global cntLivingProcesses
    global livingProcesses
    global resultDirectories
    global namesOfLivingProcesses
    global randSeed
    
    # Trap the process in a while loop if there are already enough 
    # processes running
    while cntLivingProcesses >= int(config['SETTINGS']['simultaneous processes']):
        for process in livingProcesses:
            if process.poll() != None:
                indexProcess = livingProcesses.index(process) 
                livingProcesses.remove(process)
                processIDStr = namesOfLivingProcesses.pop(indexProcess)
                print('Process Completed ({})'.format(processIDStr))
                cntLivingProcesses -= 1
                break
        time.sleep(0.01)
    
    # Take off the .auto file extension
    fileName, fileExt = os.path.splitext(files)
    
    # Create a new directory for each simulation process
    formatString = '{:.'+str(paramPrecision)+'f}'
    if ver == 1:
        newPath = os.path.join(cwd,pathResults,runNum,fileName)
    if ver == 2:
        newPath = os.path.join(cwd,pathResults,runNum,fileName+'_'+formatString.format(param1))
    # Make the new directory
    os.makedirs(newPath)

    # Add the current project path to the list of project paths
    projectDirectories.append(newPath)

    # Copy (and Format, if ver == 2) the sim_cfg file to the new dir
    with open(os.path.join(cwd,pathProfiles,files)) as f: 
        profileString = f.read()
    if ver == 1:
        profileString = profileString.format(randSeed)
    if ver == 2:
        paramListIndex = paramList1.index(param1)         
        param1b = paramList1b[paramListIndex] 
        #profileString = profileString.format(randSeed, param1)
        profileString = profileString.format(randSeed, param1b)
    with open(os.path.join(newPath,"sim_cfg"),'w') as f:
        f.write(profileString)

    # Increment the random number generator seed for the next simulation
    randSeed += 1

    # Create a pid file which the c program will edit later
    pid_file = open(os.path.join(newPath,'pid'), 'w')
    pid_file.close()
    # Print the system command for debug purposes
    #print('sh -c "cd '+newPath+'; '+os.path.join(cwd,pathCode,'eponsim')+' > sim_log.txt"')
     
    # Run the system command.
    livingProcesses.append(subprocess.Popen([os.path.join(cwd,pathCode,'eponsim')+' > sim_log.txt'], cwd=newPath, shell=True))
    cntLivingProcesses += 1
    if ver == 1:
        namesOfLivingProcesses.append(fileName)
        print("Process Launched ({})".format(fileName))
    if ver == 2:
        namesOfLivingProcesses.append(fileName+'_'+formatString.format(param1))
        print("Process Launched ({})".format(fileName+'_'+formatString.format(param1)))

def gather_results(runNum):
    listRunNumbers = []

    # Find the full results folder ie. /home/eepon/results
    pathResultsAbs = os.path.join(cwd, pathResults)

    # Find all of the objects inside that folder
    for aDir in os.listdir(pathResultsAbs):
        # If that object is a directory, add it to the listRunNumbers list
        if os.path.isdir(os.path.join(pathResultsAbs, aDir)):
            listRunNumbers.append(aDir)
    
    # Check to make sure that the directory exists
    if runNum not in listRunNumbers:
        print("Invalid Run Number")
        sys.exit()

    # Edit the results path to include the given run number ie. /home/eepon/results/001
    pathResultsAbs = os.path.join(pathResultsAbs, runNum)

    # Make a list of the directories of each of the simulation instances inside of that
    # results folder
    # simInstances contains the absolute paths of each of the simulations run
    simInstances = []
    for aDir in os.listdir(pathResultsAbs):
        if os.path.isdir(os.path.join(pathResultsAbs, aDir)):
            simInstances.append(os.path.join(pathResultsAbs, aDir))
    #print(simInstances)

    # Check to make sure simInstances is not empty
    if not simInstances:
        print('No simulation instances were found.')
        sys.exit()

    # Check if the desired results directory already exists (if results were
    # already generated for it), and if so, exit the script. Otherwise,
    # create the directory and proceed.
    resultsDirFinalAbs = os.path.join(pathResultsAbs, RESULTS_FOLDER_NAME)
    if OVERWRITE_RESULTS_DIRECTORY == True:
        print('Results were Overwritten (-d option used)')
        shutil.rmtree(resultsDirFinalAbs)
        os.makedirs(resultsDirFinalAbs) 
        simInstances.remove(resultsDirFinalAbs)
    else:
        if resultsDirFinalAbs in simInstances:
            print('ERROR: {} directory already exists'.format(RESULTS_FOLDER_NAME))
            sys.exit()
        else:
            os.makedirs(resultsDirFinalAbs) 
    
    # For each simulation instance
    for simInstance in simInstances:
        # For each file to be copied 
        for AFile in filesToBeCopied:
            # Check to make sure the source file exists
            if AFile not in os.listdir(simInstance):
                print('{} is not in the simulation instance {}'.format(AFile, simInstance))
                sys.exit()

            # Take off the file extension
            fileName, fileExt = os.path.splitext(AFile)

            # Break down the source directory into its parts
            dirParts = os.path.split(simInstance)

            # Name the new file name using the previous file name
            # and the profile name
            nameFileNew = '_'.join([fileName, dirParts[-1]])
            nameFileNew = nameFileNew+fileExt
            #print(nameFileNew)

            # Copy (and rename) the file
            shutil.copyfile(os.path.join(simInstance, AFile), os.path.join(resultsDirFinalAbs, nameFileNew))

    for compileFile, column in filesToBeCompiled:
        fileOutput = []
        ibb = 0
        for simInstance in simInstances:
            fileOutput.append([])
            with open(os.path.join(simInstance, compileFile),'r') as f:
                csvread = csv.reader(f, delimiter=' ')
                for row in csvread:
                    fileOutput[ibb].append(row[column])
            ibb += 1
        # Take off the file extension
        fileName, fileExt = os.path.splitext(compileFile)
        nameFileNew = '_'.join(['COMPILED', fileName, 'col'+str(column)])
        nameFileNew = nameFileNew+fileExt
        with open(os.path.join(resultsDirFinalAbs, nameFileNew),'w') as f:
            csvwrite = csv.writer(f, delimiter=' ')
            fileOutputInverted = [[fileOutput[j][i] for j in range(len(fileOutput))] for i in range(len(fileOutput[0]))]
            csvwrite.writerows(fileOutputInverted)
        #print(fileOutputInverted)


########################################################################
# Start main code


# Read the config file
config = read_config()

# Find the current working directory
cwd = os.getcwd()
#print(cwd)

# Set the default behavior when the results directory already exists
OVERWRITE_RESULTS_DIRECTORY = False
RESULTS_FOLDER_NAME = 'results'

# Rename the settings to have shorter names
runNum = config['SETTINGS']['run number']
pathCode = config['SETTINGS']['program path']
pathProfiles = config['SETTINGS']['profile path']
pathResults = config['SETTINGS']['results path']
randSeed = int(config['SETTINGS']['rand seed'])

# List the files to be copied
#filesToBeCopied = ['ppc.txt']
filesToBeCopied = ['ppc.txt', 'pc.txt', 'tpc.txt', 'od.txt']

# For compiling files, the file name and column to be compiled must be given
filesToBeCompiled = [('tpc.txt', 1), ('tpc.txt', 2), ('tpc.txt', 3), ('tpc.txt', 4)]

# Process the command line arguments
#print(sys.argv)
try:
    opts, args = getopt.getopt(sys.argv[1:],"r:d", ["results="])
except:
    print("Options were passed incorrectly")
    sys.exit()
for opt, arg in opts:
    if opt in ('-d'):
        OVERWRITE_RESULTS_DIRECTORY = True
    if opt in ("-r", "--results"):
        # If the generate results flag is given, only generate results
        gather_results(arg)
        sys.exit()

# Rewrite the config file with the updated settings
write_config(config)

# Print the executable (code), profiles, and results directories
print('Launcher Started ({})'.format(runNum))

#print(pathCode)
#print(pathProfiles)
#print(pathResults)

# Attempt to compile the code
os.system('cd '+pathCode+'; make eponsim; cd ..')

# For each profile in the profiles directory
for files in os.listdir(pathProfiles):
    # If the file is a .txt file (which the profile should be)
    if files.endswith(".txt"):
        start_sim(files, 1, 0)

# Edit the base profile to have a range of options automatically
# For each profile in the profiles directory
for files in os.listdir(pathProfiles):
    # If the file is a .auto file (which the auto gen parameters profiles should be)
    if files.endswith(".auto"):
        for param1 in paramList1:
            start_sim(files, 2, param1)

print('All Processes have Launched ({})'.format(runNum))

# Wait for the remaining processes to finish
while cntLivingProcesses > 0:
    for process in livingProcesses:
        if process.poll() != None:
            indexProcess = livingProcesses.index(process) 
            livingProcesses.remove(process)
            processIDStr = namesOfLivingProcesses.pop(indexProcess)
            print('Process Completed ({})'.format(processIDStr))
            cntLivingProcesses -= 1
            break
    time.sleep(0.01)

print('All Processes have Finished ({})'.format(runNum))

update_rand_seed(config, randSeed)

gather_results(runNum)


