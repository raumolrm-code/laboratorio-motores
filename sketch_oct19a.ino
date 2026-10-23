#include <Servo.h>
#include <Stepper.h>

// Pines donde conectamos los jumpers
const int palanquitaX  = A0;  // Para mover el servomotor de lado a lado
const int palanquitaY  = A1;  // Para mover el motor de pasos hacia adelante y atras
const int botonPalanca = 2;   // El boton de la palanquita para quitar el bloqueo
const int sensorGolpe  = 3;   // El sensor
const int focoBloqueo  = 13;  // El foco que avisa si el sistema se detuvo

// Cantidad de pasos que da el motor redondo para dar una vuelta entera
const int pasosVuelta = 2048;

// Nombres para controlar los motores
Servo miServo;
// Orden de los cables que van a la tarjeta verde del motor
Stepper motorPasos(pasosVuelta, 4, 6, 5, 7);

// Variables para recordar como estaban las cosas
bool estaBloqueado = false;
int anguloAnterior = 90;
int estadoReposoSensor = HIGH; // Se ajusta solo al encender la placa

// Valores para que la palanquita no tiemble al estar en el centro
const int centroPalanca = 512;
const int margenCentro  = 50;  // Espacio donde el motor no hara nada para evitar que se mueva solo
const int velMaxima     = 15;  // Lo mas rapido que puede girar el motor stepper

void setup() {
  Serial.begin(9600);

  // Definimos quines reciben senales y cuales mandan corriente
  pinMode(botonPalanca, INPUT_PULLUP);
  pinMode(sensorGolpe, INPUT_PULLUP);
  pinMode(focoBloqueo, OUTPUT);
  digitalWrite(focoBloqueo, LOW);

  // Preparamos el servomotor
  miServo.attach(9);
  miServo.write(anguloAnterior);

  // Guardamos como esta el sensor cuando nadie mueve la mesa
  delay(100);
  estadoReposoSensor = digitalRead(sensorGolpe);

  Serial.println("Sistema iniciado. Operacion normal.");
}

void loop() {
  // 1. Revisar si alguien golpeo la mesa
  int lecturaGolpe = digitalRead(sensorGolpe);
  if (!estaBloqueado && (lecturaGolpe != estadoReposoSensor)) {
    estaBloqueado = true;
    digitalWrite(focoBloqueo, HIGH); // Prendemos el foco de aviso
    
    // Le quitamos la corriente al motor de pasos para que no se mueva nada
    digitalWrite(4, LOW);
    digitalWrite(5, LOW);
    digitalWrite(6, LOW);
    digitalWrite(7, LOW);

    Serial.println("¡ALERTA! Movimiento brusco detectado. Sistema bloqueado.");
  }

  // 2. Que hacer si el sistema esta bloqueado por un golpe
  if (estaBloqueado) {
    // Si alguien presiona el boton de la palanquita, quitamos el bloqueo
    if (digitalRead(botonPalanca) == LOW) {
      delay(50); // Esperamos un momento para que el boton no se lea dos veces por error
      if (digitalRead(botonPalanca) == LOW) {
        estaBloqueado = false;
        digitalWrite(focoBloqueo, LOW);
        estadoReposoSensor = digitalRead(sensorGolpe); // Volvemos a leer como quedo el sensor para no bloquearnos por error
        Serial.println("Sistema reactivado por el usuario.");
        while (digitalRead(botonPalanca) == LOW); // Esperar a que la persona suelte el boton
      }
    }
    return;
  }

  // 3. Mover el servomotor con la palanquita
  int lecturaX = analogRead(palanquitaX);
  // Convertimos la lectura de la palanca a grados para el motor
  int nuevoAngulo = map(lecturaX, 0, 1023, 0, 180);
  if (abs(nuevoAngulo - anguloAnterior) >= 1) {
    miServo.write(nuevoAngulo);
    anguloAnterior = nuevoAngulo;
  }

  // 4. Mover el motor de pasos con la palanquita
  int lecturaY = analogRead(palanquitaY);

  // Si la palanquita esta en el centro, apagamos el motor
  if (lecturaY >= (centroPalanca - margenCentro) && lecturaY <= (centroPalanca + margenCentro)) {
  
    digitalWrite(4, LOW);
    digitalWrite(5, LOW);
    digitalWrite(6, LOW);
    digitalWrite(7, LOW);
  }
  // Si empujan la palanquita hacia arriba, gira hacia un lado
  else if (lecturaY > (centroPalanca + margenCentro)) {
    int velocidadMotor = map(lecturaY, centroPalanca + margenCentro, 1023, 2, velMaxima);
    motorPasos.setSpeed(velocidadMotor);
    motorPasos.step(10); // Da unos cuantos pasos hacia adelante
  }
  // Si empujan la palanquita hacia abajo, gira al reves
  else if (lecturaY < (centroPalanca - margenCentro)) {
    int velocidadMotor = map(lecturaY, centroPalanca - margenCentro, 0, 2, velMaxima);
    motorPasos.setSpeed(velocidadMotor);
    motorPasos.step(-10);
  }
}