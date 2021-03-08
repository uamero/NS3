#!/usr/bin/env python
## @package ns3_run
# <h1>Pre-requisites:</h1>
# 
# <ol>
# <li> This was tested on Mac OSX (Sierra to Mojave) & Ubuntu 16 to 18 with Python 2 & Python3 installed.</li>
#
# <li> You need to defined $NS3_ROOT_DIR as an environment variable so that it points to the ns3 root directory from whhich you run ./waf </li>
# commands. For example: <br>
#    <center><strong>~/Documents/NS3Download/ns-3.29</strong> </center><br>
# ns-3.29 has the directories: src, build, etc..
#    
# <li> Your NS3 program(s) should be in a directory under $NS3_ROOT_DIR/scratch. The name of the directory is the name of your program.</li>
#
# Make sure that your program have at least one file that has the main function.
#   Clarification:
#        ns-3.29 > 
#            scratch >
#                > NS3Program1
#                    > Program.cc                (this can have any name, but must have a main function)
#                    > SomeUserDefinedFile.cc    (Optional: if you want some separation of code when you design some C++ classes)
#                    > SomeUserDefinedFile.h     (All these will be compiled when you run ./waf )
#                > NS3Program2
#                    > main.cc 
#
# <li> All scripts use sort_dir.py to find the last modified project under scratch directory</li>
#
# <li> Make sure the python file permission allow "execution". </li> You can do that by running
#   chmod 700 run_ns3.py
# otherwise, you will have to run is by typing: "python run_ns3.py" instead of ./run_ns3.py
# </ol>
# <h2> Usage: </h2>
# <ul>
# <li> To run the last program you edited & saved under scratch directory simply run:
#    path_to_script/run_ns3.py </li>
#    
#    - You need to be in the NS3 root directory for this to run.
#    - You may place these python scripts in any directory. For example, under $NS3_ROOT_DIR/scripts.BaseException
#        + To run the last modified program, we type:
#            ./scripts/run_ns3.py
#        This execuates the following:
#            $NS3_ROOT_DIR/ns-3.29/waf --run LastModifiedProgram
#
# <li> You can run it with arguments like: </li>
#    ./scripts/run_ns3.py --nCsma=10 --nWifi=10
# which runs
#    $NS3_ROOT_DIR/ns-3.29/waf --run LastModifiedProgram "--nCsma=10 --nWifi=10"
# </li>
                
import sys
import os
import sort_dir as sort_dir_obj
import random
import matplotlib.pyplot as plt 
import pandas as pd
import numpy as np


from MyColor import MyColor
from subprocess import Popen, PIPE, STDOUT
from sys import platform

#Adds Color to the Terminal. 
try:
    from termcolor import colored
    no_color = False
except ImportError:
    no_color = True
    pass # module doesn't exist, deal with it.

print_details = 1

def do_run_command (cmd_str):
    os.system (cmd_str)

def make_compatible_str(line_input):
    '''
    For compatibility with both Python2 and Python3
    '''
    if type(line_input) is str:
        return line_input       #for Python2
    elif type(line_input) is bytes:
        return line_input.decode() #Python3

"""
Argumentos de la simulacion 
  cmd.AddValue ("t", "Tiempo de simulacion", simTime);
  cmd.AddValue ("nit", "Numero de iteraciones", n_iteracion);
  cmd.AddValue ("i", "Duracion del intervalo de broadcast", interval);
  cmd.AddValue ("nA", "Numero de nodos generadores", n_SecundariosA);  -->por default es 1
  cmd.AddValue ("nB", "Numero de nodos no generadores", n_SecundariosB); --->>>por default son 15
  cmd.AddValue ("nPTS", "Numero de paquetes a generar", n_Packets_A_Enviar);  --->>> por default es 1
  cmd.AddValue ("nP", "Numero de nodos Primarios", n_Primarios);   -->>>> por default es 1
  cmd.AddValue ("CSVFile", "Nombre del archivo CSV donde se almacenaran los resultados de la simulacion", CSVFile);
  cmd.AddValue ("StartSim", "Comienza un escenario de simulacion nuevo", StartSimulation);
"""
def CalculaSemilla(default_program,pwd):
    TA = 1
    TB=1
    nA=1
    nPTS=1
    nB=15
    nP=1
    n_iteracion = 1
    Start="true"
    CSVName="CalculoSemilla.csv"
    n_iteracion
    for x in range(1000):
        seed=x+1
        #seed=1
        #print ("IT Command "+str(n_iteracion)+" |semilla: "+str(seed)+" |TB: "+str(TB[x])+" : " + pwd + "/waf --run \"" + default_program + "\"")        
        argumentos= " --iA="+str(TA)+" --iB="+str(TB) +" --nPTS="+str(nPTS)+" --nA="+str(nA)+" --nB="+str(nB)+" --nP="+str(nP)\
        +" --StartSim="+Start+" --nit="+str(n_iteracion)+" --CSVFile="+CSVName +" --Seed="+str(seed)
        os.system(pwd + "/waf --run \"" + default_program+argumentos+"\""+"\n")
        Start="false"
def Calcula_Punto_DeRuptura(default_program,pwd):
    TA=1
    nPTS=1
    Semillas = np.random.randint(1,2**32-1,(500,));#Arreglo de 10 semillas
    nA=1
    nP=random.randint(1,10)
    homo="false"
    CSVName = "Sim_"
    rwp="true"
    nB=[100,500,1000]
    i=0
    tp=random.randint(1,20)
    for NB in nB:
        Start="true"
        for x in Semillas:
            print ("IT Command "+str(i)+": "+"nB="+str(NB)+" |semilla: "+str(x) + pwd + "/waf --run \"" + default_program + "\"")
            argumentos= " --iA="+str(TA) +" --nPTS="+str(nPTS)+" --nA="+str(nA)+" --nB="+str(NB)+" --nP="+str(1)\
            +" --StartSim="+Start+" --CSVFile="+CSVName+"nA_"+str(1)+"_nB_"+str(NB)+".csv" +" --Seed="+str(x)\
            +" --rwp="+rwp+" --hg="+homo+ " --tp="+str(tp)     
            os.system(pwd + "/waf --run \"" + default_program+argumentos+"\""+"\n")
            Start="false"
            i+=1        
                
def Tiempo_Generacion_VS_Tiempo_RX(default_program,pwd):
    #datos=pd.read_csv("CalculoSemilla.csv",sep=",",header=0)
    #Semillas=datos["Semilla"][0:100]# Este archivo contiene 270 semillas de las cuales solo tomamos 100

    #Generar las graficas incrementando los nodos A y B con una misma denciada de nodos P 
    Semillas = np.random.randint(1,2**32-1,(100,));#Arreglo de 10 semillas
    TA = 1 #Se generan paquetes cada 1s
    nA=1
    nPTS=1
    nB=[2500]
    nP=1
    n_iteracion = 1
    CSVName = "Sim_"
    rwp= "true"
    homo="false"
    tp=random.randint(1,20)
    nch=random.randint(2,10)
    #seed=random.randint(0,9)

    i=0  
    for y in nB:
        Start="true"
        for seed  in Semillas:
            print ("IT Command: "+"i:"+str(i)+" y:"+str(y)+" |semilla: "+str(seed) + pwd + "/waf --run \"" + default_program + "\"")        
            argumentos= " --iA="+str(TA) +" --nPTS="+str(nPTS)+" --nA="+str(nA)+" --nB="+str(y)+" --nP="+str(nP)\
            +" --StartSim="+Start+" --CSVFile="+CSVName+"nA_"+str(nA)+"_nB_"+str(y)+".csv" +" --Seed="+str(seed)\
            +"--rwp="+rwp+" --hg="+homo+ " --tp="+str(tp) + " --nch="+str(nch)     
            os.system(pwd + "/waf --run \"" + default_program+argumentos+"\""+"\n")
            Start="false"
            i+=1
           

def FirstScenario(default_program,pwd):
    #nPTS=[x for x in range (1,11)]
    nPTS=[1,5,10]
    CSVNames=["FirstSA.csv","FirstSB.csv","FirstSC.csv"]
    TSimulation= [100,150,200]
    nA=1#una lista de 1 a 10 nodos generadores
    nB=[15,30,60]
    nP=1
    
    #for y in nB:
    for x in range(3):#numero de paquetes a enviar
        Start="true" 
        for n_iteracion in range(10):
            aleatA=random.randint(1,10)
            aleatB=aleatA*5       
            print ("IT Command "+str(n_iteracion)+" |nPTS: "+str(x)+" : " + pwd + "/waf --run \"" + default_program +" --iA=" + str(aleatA) + "\"")
            argumentos= " --iA="+str(aleatA)+" --iB="+str(aleatB) +" --nPTS="+str(nPTS[x])+" --nA="+str(nA)+" --nB="+str(nB[0])+" --nP="+str(nP)\
            +" --StartSim="+Start+" --nit="+str(n_iteracion)+" --CSVFile="+CSVNames[x]+" --t="+str(TSimulation[0])      
            os.system(pwd + "/waf --run \"" + default_program+argumentos+"\""+"\n")
            Start="false"
        
    #os.system("libreoffice /home/manuel/Escritorio/Datos_Sim/datos.csv &")
def main(argv):
    """Main function"""
    #Get the present working directory
    pwd = os.getcwd()
    #check if NS3_ROOT_DIR is set, otherwise choose ~/workarea as a default directory (will probably be wrong)
    mypath = os.getenv('NS3_ROOT_DIR', '~/eclipse-workspace/NS3Work')
    ''' 
    #If you're not at the NS3 root directory, this program stops. 
    if pwd != mypath:
        print (MyColor.RED + MyColor.BOLD + "Error : " +MyColor.END + MyColor.RED + "Make sure you are in the NS3 root directory" + MyColor.END)
        exit(1)
        
        if no_color:
            print "Error : Make sure you are in the NS3 root directory"
        else:
            err_str = 'Error : You need to be in the NS3 Root directory' 
            sys.exit(colored(err_str, 'red'))
    '''

    #Get the most recent modified program from $NS3_ROOT_DIR/scratch
    default_program = sort_dir_obj.get_most_recent()

    
    '''
    if no_color:
        print  "Script is running NS3 program : " + default_program   
    else:
        print colored("Script is running NS3 program : " + default_program, 'green') 
    '''
    
    print (MyColor.GREEN + "Scripts is running ns3 program : " + default_program + MyColor.END)
    
    #If running without arguments...
    if len(argv) == 0:
        #Printing the command string so that users can see if they're not running the intended program.
        """aleat=random.randint(0,100)/100.0
        i=0
        if print_details:
             #print "Program is not supplied with parameters"
             #print ("Command : " + pwd + "/waf --run " + default_program)
            print ("IT Command "+str(i)+" : " + pwd + "/waf --run \"" + default_program +" --i=" + str(aleat) + "\"")
        #Run the program!
        #os.system(pwd + "/waf --run " + default_program)
        os.system(pwd + "/waf --run \"" + default_program+" --i="+str(aleat)+"\""+"\n")
        #Set NS3_PROGRAM to the last executed program (not needed. This should be removed later.)
        #os.environ['NS3_PROGRAM'] = default_program"""
        #FirstScenario(default_program,pwd)
        Tiempo_Generacion_VS_Tiempo_RX(default_program,pwd)
        #Calcula_Punto_DeRuptura(default_program,pwd)
        #CalculaSemilla(default_program,pwd)
    else:
        #Program have more than zero arguments, so we go through them in a loop, and use build a command string.
        #The default command string with a quotation mark added..
        command_string = pwd + "/waf --run \"" + default_program

        #Looping through Command line arguments
        arguments = ""

        for i in range(0, len(argv)):
            #add commandline arguments one by one.
            arguments = arguments + argv[i] + "\n"
            command_string = command_string + " " + argv[i]

        #close the string literal
        command_string = command_string + "\""

        
        #Showing command string before executaion.
        print ("Command: " + command_string)
        #Run it with parameters!
        
        do_run_command (command_string)
        
        #TODO : Delete this
        #Set NS3_PROGRAM to the last executed program (not needed. This should be removed later.)
        #os.environ['NS3_PROGRAM'] = default_program

if __name__ == "__main__":
    main(sys.argv[1:])


