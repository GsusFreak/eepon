import os
import configparser
from shutil import copyfile

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

# Print the executable (code), profiles, and results directories
#print(pathCode)
#print(pathProfiles)
#print(pathResults)

# Attempt to compile the code
os.system('cd '+pathCode+'; make eponsim; cd ..')

# For each profile in the profiles directory
for files in os.listdir(pathProfiles):
    # If the file is a text file (which the profiles should be)
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

write_config(config)



