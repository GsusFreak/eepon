
import os
import random
import math


numWeightSets = 10
numWeights = 32
nameFile = 'random_100.txt'
randomRange = 100

listOfStrOut = []

for iaa in range(1, numWeightSets+1):
    strOut = ''
    for ibb in range(1, numWeights+1):
        if ibb == 1:
            strOut += "\t'" 

        # Random
        num = math.floor(random.random()*randomRange+1)
        if num == randomRange+1:
            print('ERROR\n')

        ## One-Sided Low
        #if ibb > 1:
        #    num = round((iaa-1)*16/31+1, 2)
        #else:
        #    num = 1

        strOut += str(num) 
        if ibb < numWeights:
            strOut += ' '
        if ibb == numWeights:
            strOut += "', "
    listOfStrOut.append(strOut)
result = '\n'.join(listOfStrOut)
result += '\n'
print(result)

with open(os.path.join(os.getcwd(), nameFile), "w") as f:
    f.write(result)

