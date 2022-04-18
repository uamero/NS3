import matplotlib.pyplot as plt 
import pandas as pd
import numpy as np
import math

def Calcula_Media_desvSTDR(datos):
    Media=0
    DesvSTDR=0
    for x in datos:
        Media=Media + x
    Media=Media/len(datos)
    #print(Media)    
    for x in datos:
        DesvSTDR =DesvSTDR+ pow(x-Media,2)
    #print("->"+str(DesvSTDR))    
    DesvSTDR=math.sqrt(DesvSTDR/len(datos))         
    return Media,DesvSTDR


CSVFiles="Tiempo_de_NB"
#TB= [x for x in range (1,11)]
TB= [1]
Tiempos_a_graficar = []
#for x in TB:
file=CSVFiles+str(1)+".csv"
datos=pd.read_csv("Tiempo_de_NB1.csv",sep=",",header=0)
for NumberOfPacket in range (1,21):
    datos_MASK=datos['No. Paquete']==NumberOfPacket # mascara para identificar en donde se localiza la informacion de un paquete
    datos_filtrados=datos[datos_MASK]#Datos de un solo paquete
    Tiempos_de_paquete=datos_filtrados["Tiempo de entrega"]#Filtro por columna
    Tiempos_a_graficar.append(Tiempos_de_paquete)
#Eje_X = [x for x in range (1,21)]
#medias=[]
#Y_Error_max=[]
#Y_Error_min=[]
#desv_ALL=[]
"""for x in range(len(Tiempos_a_graficar)):
    #media,desv=Calcula_Media_desvSTDR(Tiempos_a_graficar[x])
    
    m=np.mean(Tiempos_a_graficar[x])
    ds=np.std(Tiempos_a_graficar[x])
    medias.append(m)
    #desv_ALL.append(desv)
    Y_Error_max.append(m+ds)
    Y_Error_min.append(m-ds)
    #print(np.mean(Tiempos_a_graficar[x]))
    #print(np.std(Tiempos_a_graficar[x]))
    Y_error=[Y_Error_min,Y_Error_max]
#plt.plot(Eje_X,medias,'o')
#plt.errorbar(Eje_X,medias,yerr=Y_error,fmt='o')
"""    

plt.boxplot(Tiempos_a_graficar)
plt.ylabel('Tiempo de entrega (s)')
plt.xlabel('Paquete')
plt.xlim([0,22])
plt.ylim([0,8])
plt.title("No. de paquete Vs Tiempo de entrega")
plt.savefig("graficaS2.emf")
plt.show(block=False)
#print(medias)
#print(desv_ALL)