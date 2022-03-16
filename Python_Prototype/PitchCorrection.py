import numpy as np
from scipy.io import wavfile
import math
from scipy.io.wavfile import write

from scipy.signal import butter, lfilter, freqz
from scipy.signal import decimate
from scipy.interpolate import interp1d
from scipy.signal import resample
import sounddevice as sd

class PitchCorrector():

    def __init__(self, wav, sensitivity=.02, min_freq=50.0, max_freq=2500.0 ):

        print("data type: ", wav.dtype)

        detection_mode = True

        # Determina la relacion entre el pitch buscado y el real para hacer el resample
        resample_rate2 = 1

        # Se utiliza para hacer la correccion el metodo propuesto en la patente. En ese metodo se tiene un input addr que es un indice entero de la entrada inicializado a 10
        # Tambien se tiene un output addr que es un indice float tambien de la entrada
        input_addr = 10
        output_addr = 0
        
        eps = sensitivity
        decay = 0
        
        cycle_period = 0
        desired_cycle_period = 0
        
        all_samples = np.array(len(wav) + 10)
        
        new_decimated_value = True
        
        # Audio modificado
        self.output = np.zeros_like(wav)
        # Entrada pasada por un filtro tomando 1 de cada 8 muestras
        decimated_input =decimate(wav, q=8)

        # Saving frequencies for representation in notebook
        self.real_frequencies = np.zeros_like(decimated_input)
        self.desired_frequencies = np.zeros_like(decimated_input)
        
        # Pitch correcto
        desired_Lmin = 0
        # Pitch detectado
        Lmin = 0
        
        # Valores para cada posible valor de lag para la funcion de energia acumulada
        E_Table = np.zeros(111)
        # Valores para cada posible valor de lag para la correlacion
        H_Table = np.zeros(111)
        # Valores detectados para cada posible valor de lag de la funcion E - 2*H
        sub_Table = np.zeros(111)
        
        # Minimo valor posible de lag es el sample rate partido por la frecuencia maxima detectable
        # Maximo valor posible de lag es el sample rate partido por la frecuencia minima detectable
        # Como los calculos se hacen sobre el decimated input solo tenemos 1/8 de las muestras. Por eso hay que dividir por 8 tambien
        # Si lo hiciesmos sobre la entrada completa con frecuencia maxima 2500 seria 16. Al ser sobre el decimated input es 2
        # Lo mismo para la maxima. Si fuese la entrada completa con frecuencia minima 20 seria 880. Al ser sobre el decimated input es 110.

        min_L = int(44100.0/(float(max_freq)*8))
        max_L = int(44100.0/(float(min_freq)*8))
          
        # Con esta variable voy controlando por que indice del decimated input voy para depurar
        decimated_index = 0

        input_addr = 9
        
        # Pasamos por cada sample de la entrada y vamos construyendo la salida dependiendo del resample rate que se vaya calculando
        for index in range (0, len(wav)):
            
            input_addr = input_addr + 1
            
            if index % 8 == 0:
                # Si el indice es multiplo de 8 tenemos un nuevo valor que leer de decimated_input
                new_decimated_value = True
                
            if (new_decimated_value):

                new_decimated_value = False

                # Esperamos hasta el indice del decimated input 220
                # Como el lag maximo es 110 y los calculos se hacen utilziando las 2*lag muestras anteriores, no se tienen datos completos hasta el indice 220
                # Inicializamos E y H con las funciones 2 y 3 de la patente.
                # Probe a utilizar directamente las ecuaciones 4 y 5 pero vete tu a saber por que no funcionan. Llorando por dentro
                if (decimated_index == 220):
                    print('initialize tables')
                    for L in range(min_L, max_L):
                        # Ecuacion 2: E(lag) = sumatorio de xj al cuadrado para valores de j de indice - 2*L a indice
                        E_Table[L] = sum(np.square(decimated_input[221-2*L:221]))
                        # Ecuacion 3: H(lag) = sumatorio de xj * x(j-L) para valores de j de indice - L a indice
                        H_Table[L] = sum(np.multiply(decimated_input[221-2*L:221-L], decimated_input[221-L:221]))
                    
                elif decimated_index > 220:

                    # A partir de 220 ya utilizo las ecuaciones 4 y 5 
                    for L in range (min_L, max_L + 1):

                        # Como los calculos se hacen utilizando las 2*lag muestras anteriores, podemos actualizar los valores de E y H
                        # sumando al valor calculado anteriormente el que corresponde al sample actual
                        # y restando el valor caculado para el sample 2*lag anterior
                        # Es decir, si yo he utilizado los valores de las muestras 0 1 2 3 4 y 5. En el paso 6 simplemente quito el valor calculado para la muestra 0 
                        # y añado el valor calculado para la muestra 6 y asi tengo el valor que habria obtenido con las ecuaciones 2 y 3 para las muestras 1 2 3 4 5 y 6.
                        # Ecuacion 4: E(lag) = E(lag) del calculo anterior + samples[indice] al cuadrado - samples[indice - 2*lag] al cuadrado
                        # Ecuacion 5: H(lag) = H(lag) del calculo anterior + samples[indice]*samples[indice-lag] - samples[indice-L]*samples[indice-2*lag]
                        E_Table[L] = E_Table[L] + decimated_input[decimated_index]**2 - decimated_input[decimated_index-2*L]**2
                        H_Table[L] = H_Table[L] + decimated_input[decimated_index]*decimated_input[decimated_index-L] - decimated_input[decimated_index-L]*decimated_input[decimated_index-2*L]

                        # sub = E - 2*H
                        sub_Table[L] = E_Table[L] - 2 * H_Table[L]

                        # Ecuacion 6 de la patente: epsilon * E >= E - 2*H 
                        # Si se cumple esta condicion hemos encontrado un valor para el lag que se aproxima lo suficiente como para ser considerado el periodo
                        if sub_Table[L] < eps * E_Table[L] and L > min_L and L < max_L:
                            
                            #Lmin = 8*L
                            # Esta funcion obtiene cual seria un lag mas aproximado al real utilizando la entrada completa en vez del decimated input. Se explica el proceso en la funcion
                            Lmin,freq = self.get_real_freq(wav, decimated_index, L)
                            # Una vez obtenida la frecuencia real aproximada buscamos cual era la correcta
                            # Se compara la frecuencia encontrada con las frecuencias de todas las notas de la escala para determinar cual es la nota mas cercana a esa frecuencia
                            desired_freq = self.find_desired_freq(freq)
                            # Con esa frecuencia buscada, calculamos cual era el pitch buscado igual que hicimos antes para buscar el lag minimo y maximo:
                            # sample rate / frecuencia
                            desired_Lmin = 44100/desired_freq

                            self.real_frequencies[decimated_index] = freq
                            self.desired_frequencies[decimated_index] = desired_freq

                            # Esto es solo para comparar los resultados en pruebas
                            if not decimated_index % 10000:
                                print('i:', index, 'real_freq:', freq, 'desired_freq:', desired_freq)
                            break
                    

            # Si se ha encontrado un lag calculamos el resmample rate como lag encontrado / lag buscado
            # Aqui faltaria añadir una forma de suavizar los cambios en el pitch propuesta en la patente
            # De momento no lo he puesto para poder comparar los resultados con los del modelo de referencia   
            if Lmin != 0:
                resample_raw_rate = Lmin / desired_Lmin
                resample_rate2 = resample_raw_rate
            
            # Sample corregido a escribir en la salida
            output_sample = 0
            
            # Aqui esta el metodo propuesto en la patente para hacer el resampling
            # El output addr se incrementa con el resample rate
            output_addr = output_addr + resample_rate2
            
            # Si el resample rate es mayor que 1 corremos el riesgo de que en algun momento el indice de salida output addr sobre pase al indice de entrada input addr
            # Si esto pasa podriamos estar cogiendo valores para la salida utilizando datos de una seccion de las muestras con otro pitch
            # Para evitar esto, si sucede que el output addr es mayor que el input addr le restamos al output addr el lag encontrado acutalmente de manera que
            # estemos seguros de que seguimos tomando datos dentro del pitch actual
            if resample_rate2 > 1:
                if output_addr > input_addr:
                   output_addr = output_addr - Lmin
            # De manera analoga, si el resample raet es menor que 1 puede quedarse muy rezagado respecto al input addr
            # Si esto pasa podemos estar tomando datos de un pitch anterior al actual.
            # Para evitar esta situacion, si vemos que la distancia entre el input addr y el output addr es mayor al periodo, 
            # adelantamos el output addr un ciclo entero
            else:
                if output_addr + Lmin < input_addr:
                    output_addr = output_addr + Lmin
                
            # Se calcula la muestra de salida utilizando output_sample - 5
            # Para ello se toman los dos valores mas cercanos y se interpola el valor que habria en el output addr - 5
            if Lmin == 0:
                output_sample = 0
                
            # Si resample rate es 1 simplemente tomamos la salida y ya porque no hay cambios
            elif resample_rate2 == 1 and output_addr - 5 >= 0:
                output_sample = wav[index]
                
            # Ejemplo: si el output addr es 9,7 tomamos los valores de la entrada en el indice 9 y 10 y vemos cual seria el valor que hay en 9,7
            elif output_addr - 6 >= 0 and output_addr - 4 < len(wav):
                x = output_addr - 5
                x1 = math.floor(x)
                x2 = math.ceil(x)
                y1 = wav[x1]
                y2 = wav[x2]
                output_sample = y1 + (((y2 - y1) / (x2 - x1)) * (x - x1))
                
            
            # Se escribe el valor calculado en la salida
            self.output[index] = output_sample
            
            if not index % 8:
                decimated_index = decimated_index + 1
                resample_rate2 = 1
                    
        print('Finished')


    def play(self):
        sd.play(self.output)
        
    def find_desired_freq(self, freq):
        
        notes = np.array([55.00,58.2,61.74,
            65.41,69.30,73.42,77.78,82.41,87.31,92.50,98.00,103.83,110.00,116.54,123.47,130.81,
            138.59,146.83,155.56,164.81,174.61,185.00,196.00,207.65,220.00,233.08,246.94,261.63,
            277.18,293.66,311.13,329.63,349.23,369.99,392.00,415.30,440.00,466.16,493.88,523.25,
            554.37,587.33,622.25,659.25,698.46,739.99,783.99,830.61,880.00,932.33,987.77,1046.50])

        desired = 0
        dist = abs(notes-freq)
        desired = notes[np.argmin(dist)]
        
        return desired
    
    def get_real_freq(self, wav, down_indx, down_f_est):

        # Convertimos el indice y el lag al valor real (multiplando por 8 vaya)
        real_indx = 8*down_indx
        real_f_est = 8*down_f_est

        # Si por ejemplo el lag calculado anteriormente es 40. El lag real seria 320. El valor mas aproximado a la realidad
        # debera encontrarse entre 39*8 y 41*8 ya que todos estos valores intermedios no se tienen en cuenta en el main

        # real line son los indices que no hemos tenido en cuenta
        real_line = np.linspace(real_f_est-9,real_f_est+9,19, dtype=int)
        # real mins se utilizara para almecenar los valores calculados para buscar la estimacion mas aproximada
        real_mins = np.zeros(len(real_line), dtype=float)

        for L in real_line:
            # Esencialmente, para cada lag en real line se calculan E y H y se almacena en real_mins el valor E - 2*H / E
            a = np.array(wav[real_indx-2*L+1:real_indx-L+1], dtype=np.float64)
            b = np.array(wav[real_indx-L+1:real_indx+1], dtype=np.float64)
            E = np.sum(np.square(a)) + np.sum(np.square(b))
            H = sum(a*b)

            val = (E-2*H)/E
            real_mins[L-real_line[0]] = val

        # Para hacer la aproximacion mas precisa, utilizamos interpolacion para calcular los valores de la autocorrelacion para un rango de 100 valores de lag posibles
        interpolation_line = np.linspace(real_line[0],real_line[-1],100, endpoint=True)
        int_obj = interp1d(real_line, real_mins, kind='cubic')
        interpolated = int_obj(interpolation_line)
        # El lag mas aproxiamdo sera el que obtenga el valor minimo
        final_lag = interpolation_line[np.argmin(interpolated)]
        value = np.min(interpolated)

        # devolvemos el lag aproximado y la frecuencia correspondiente
        return(final_lag, (1/(final_lag/44100)))