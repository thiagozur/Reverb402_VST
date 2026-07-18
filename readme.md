# Reverb402
Este es un efecto de audio desarrollado  con el framework JUCE para la simulación de la reverb del aula 402 de la sede Caseros II de la UNTREF. El programa convoluciona un archivo de audio *dry* (sin procesar) con una respuesta al impulso medida in situ para lograr el efecto.

## Prestaciones

* **Visualización de la forma de onda de la respuesta al impulso**.
* **Visualización del espectrograma de la respuesta al impulso**.
* **Respuestas al impulso incluidas**: el software viene con 8 IR nativas. Se incluyen las IR de las cuatro combinaciones de fuente-receptor utilizadas en la medición, junto a dos archivos IR estéreo generados con el mismo criterio de recorte que utiliza el efecto en base a los pares izquierda-derecha de las IR mono. Además, se incluyeron dos versiones estéreo *prealigned*, que fueron creadas manualmente con otro criterio de alineación y ajuste de nivel (por lo que tienen una sonoridad distinta).
* **Sistema integrado de presets**.

## Parámetros modificables

* **Decay (factor de escalado temporal)**: se implementó un parámetro para modificar la respuesta al impulso original de la sala (0.1x - 5x). Al setearse en 1x se utiliza la IR original. Por debajo de ese valor, la IR se recorta con una envolvente exponencial. Por encima, se aplica un escalado temporal mediante *time-stretching* logrado con interpolación lineal.
* **Pre-delay (factor de escalado temporal)**: control del tiempo de retardo inicial (0 ms - 150ms).
* **Controles de filtrado**: se implementaron filtros High-Pass (20 Hz - 500 Hz) y Low-Pass (1 kHz - 20 kHz) que actúan sobre la señal procesada. Ambos son filtros Butterworth de orden 2 (-12 dB/oct). Si se setean al mínimo (en el caso del High-Pass) o al máximo (en el caso del Low-Pass) de frecuencia quedan desactivados por completo.
* **Mix**: este parámetro controla la proporción entre señal procesada y sin procesar a la salida. Un mix al 0% implica una totalidad de señal *dry* a la salida, mientras que al 100% implica una señal completamente *wet*. La señal procesada pasa previamente por un proceso de normalización por proporción (en base a los valores eficaces) para igualar su sonoridad con la de la señal original antes de mezclarlas.

## Formatos disponibles
El efecto está disponible como VST3 y en formato Standalone.

Por: Joaquín García, Candelaria Grisafi, Juan Cruz Villarreal, Santiago Zumárraga, Thiago Zurschmitten.