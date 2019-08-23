import os
import configparser
import subprocess
from shutil import copyfile

#upperBound = 350
#lowerBound = 250
#increment = 50
#paramList1 = [*range(lowerBound, upperBound+1, increment)]
paramList1 = [x * 0.001 for x in range(1,11)]
livingProcesses = []

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

config = read_config()

# Find the current working directory
cwd = os.getcwd()
#print(cwd)

# Rename the settings to have shorter names
runNum = config['SETTINGS']['run number']
pathCode = config['SETTINGS']['program path']
pathProfiles = config['SETTINGS']['profile path']
pathResults = config['SETTINGS']['results path']
genMultipleProfiles = int(config['SETTINGS']['auto generate profiles'])

# Print the executable (code), profiles, and results directories
#print(pathCode)
#print(pathProfiles)
#print(pathResults)

# Attempt to compile the code
os.system('cd '+pathCode+'; make eponsim; cd ..')

if genMultipleProfiles == 0:
    # For each profile in the profiles directory
    for files in os.listdir(pathProfiles):
        # If the file is an auto file (which the generic profile should be)
        if files.endswith(".txt"):
            # Take off the .txt file extension
            fileName, fileExt = os.path.splitext(files)
            # Make a new file name out of the run number (from the config file) and
            # the profile name
            newPath = os.path.join(cwd,pathResults,runNum+'_'+fileName)
            os.mkdir(newPath)
            # Copy the profile to its respective folder and rename it "sim_cfg"
            copyfile(os.path.join(cwd,pathProfiles,files),os.path.join(newPath,"sim_cfg"))
            # Create a pid file which the c program will edit later
            pid_file = open(os.path.join(newPath,'pid'), 'w')
            pid_file.close()
            # Print the system command for debug purposes
            #print('sh -c "cd '+newPath+'; '+os.path.join(cwd,pathCode,'eponsim')+' > sim_log.txt" &')
             
            # Run the system command.
            #os.system('sh -c "cd '+newPath+'; '+os.path.join(cwd,pathCode,'eponsim')+' > sim_log.txt" &')
            livingProcesses.append(subprocess.Popen([os.path.join(cwd,pathCode,'eponsim')+' > sim_log.txt'], cwd=newPath, shell=True))
            print("run "+runNum+"_"+fileName+" started")
    for process in livingProcesses:
        process.wait()
    print('All of the processes have completed.')

# If genMultipleProfiles is ON (== 1), edit the base profile to have a range 
# of options automatically
if genMultipleProfiles == 1:
    # For each profile in the profiles directory
    for files in os.listdir(pathProfiles):
        # If the file is a .auto file (which the auto gen parameters profiles should be)
        if files.endswith(".auto"):
            # Take off the .auto file extension
            fileName, fileExt = os.path.splitext(files)
            # Edit the profile for each parameter file
            for param1 in paramList1:
                newPath = os.path.join(cwd,pathResults,runNum,fileName+'_'+str(param1))
                os.makedirs(newPath)
                with open(os.path.join(cwd,pathProfiles,files)) as f: 
                    profileString = f.read()
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
    for process in livingProcesses:
        process.wait()
    print('All of the processes have completed.')

# Rewrite the config file with the updated settings
write_config(config)



