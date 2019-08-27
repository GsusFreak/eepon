import os
import configparser
import subprocess
from shutil import copyfile
import time

#upperBound = 350
#lowerBound = 250
#increment = 50
#paramList1 = [*range(lowerBound, upperBound+1, increment)]
paramList1 = [x * 0.001 for x in range(1,11)]
livingProcesses = []
cntLivingProcesses = 0
projectDirectories = []

def read_config():
    config = configparser.ConfigParser()
    config.read('config.ini')
    return config

def write_config(config):
    # Turn the string into an integer, increment it, and then turn it back into
    # a string with three spaces worth of left-hand zero padding (eg. 007)
    runNum = str(int(config['SETTINGS']['run number']) + 1).zfill(3)
    # Update the config file with the new run number
    config.set('SETTINGS', 'run number', runNum)
    with open('config.ini', 'w') as configfile:
        config.write(configfile)

def start_sim(files, ver, param1):
    global cntLivingProcesses
    global livingProcesses
    
    # Trap the process in a while loop if there are already enough 
    # processes running
    while cntLivingProcesses >= int(config['SETTINGS']['simultaneous processes']):
        for process in livingProcesses:
            if process.poll() != None:
                livingProcesses.remove(process)
                cntLivingProcesses -= 1
                break
        time.sleep(0.01)
    
    # Take off the .auto file extension
    fileName, fileExt = os.path.splitext(files)
    
    # Create a new directory for each simulation process
    if ver == 1:
        newPath = os.path.join(cwd,pathResults,runNum,fileName)
    if ver == 2:
        newPath = os.path.join(cwd,pathResults,runNum,fileName+'_'+'{:.6f}'.format(param1))
    os.makedirs(newPath)

    # Add the current project path to the list of project paths
    projectDirectories.append(newPath)

    # Copy (and Format, if ver == 2) the sim_cfg file to the new dir
    with open(os.path.join(cwd,pathProfiles,files)) as f: 
        profileString = f.read()
    if ver == 2:
        profileString = profileString.format(param1)
    with open(os.path.join(newPath,"sim_cfg"),'w') as f:
        f.write(profileString)

    # Create a pid file which the c program will edit later
    pid_file = open(os.path.join(newPath,'pid'), 'w')
    pid_file.close()
    # Print the system command for debug purposes
    #print('sh -c "cd '+newPath+'; '+os.path.join(cwd,pathCode,'eponsim')+' > sim_log.txt" &')
     
    # Run the system command.
    livingProcesses.append(subprocess.Popen([os.path.join(cwd,pathCode,'eponsim')+' > sim_log.txt'], cwd=newPath, shell=True))
    cntLivingProcesses += 1
    if ver == 1:
        print("Process Launched ({})".format(fileName))
    if ver == 2:
        print("Process Launched ({})".format(fileName+'_'+'{:.6f}'.format(param1)))

########################################################################
# Start main code

print('Launcher Started ({})'.format(runNum))

# Read the config file
config = read_config()

# Find the current working directory
cwd = os.getcwd()
#print(cwd)

# Rename the settings to have shorter names
runNum = config['SETTINGS']['run number']
pathCode = config['SETTINGS']['program path']
pathProfiles = config['SETTINGS']['profile path']
pathResults = config['SETTINGS']['results path']

# Print the executable (code), profiles, and results directories
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
            livingProcesses.remove(process)
            cntLivingProcesses -= 1
            break
    time.sleep(0.01)

print('All Processes have Finished ({})'.format(runNum))


# Rewrite the config file with the updated settings
write_config(config)



