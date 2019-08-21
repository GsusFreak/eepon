import os
import configparser
from shutil import copyfile

upperBound = 350
lowerBound = 250
increment = 50
paramList1 = [*range(lowerBound, upperBound+1, increment)]

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
            # It runs the eponsim file locaded in the code directory but moves the
            # working directory to the folder which corresponds to its profile and
            # run number. The "sh" command is used to detach the whole process from
            # the main run.py thread and allows multiple instances of the simulator
            # to run simultaneously. Dr. Haddad used "ssh" instead of "sh" which is
            # more difficult to use.
            os.system('sh -c "cd '+newPath+'; '+os.path.join(cwd,pathCode,'eponsim')+' > sim_log.txt" &')
            print("run "+runNum+"_"+fileName+" started")

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
                os.system('sh -c "cd '+newPath+'; '+os.path.join(cwd,pathCode,'eponsim')+' > sim_log.txt" &')
    print("run "+runNum+" started {}:{}:{}".format(lowerBound,increment,upperBound))

# Rewrite the config file with the updated settings
write_config(config)



