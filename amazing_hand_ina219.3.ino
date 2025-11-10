#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_INA219.h>

// === INITIALISATION DES MODULES ===
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);
Adafruit_INA219 ina219(0x41); // Adresse 0x41, A0 = HIGH

// === PARAMÈTRES GÉNÉRAUX ===
#define SERVOMIN 150
#define SERVOMAX 600
int MaxSpeed = 10;
int CloseSpeed = 10;

// === POSITION ACTUELLE DES SERVOS ===
int MiddlePos[8] = {90, 90, 90, 90, 90, 90, 90, 90};

// === POSITION D'OUVERTURE CIBLE POUR CHAQUE SERVO ===
int OPEN_TARGET[8] = {140, 40, 140, 40, 140, 40, 140, 40};

// === SEUILS DE COURANT ===
float CURRENT_THRESHOLD_MA = 1800.0; // seuil de "force détectée" à ajuster
float SAFE_CURRENT_MA = 700.0;       // courant normal où on considère que la pression est redevenue sûre

// === FONCTION UTILE ===
int angleToPulse(int ang) {
return map(ang, 0, 180, SERVOMIN, SERVOMAX);
}

void setup() {
Wire.begin();
Serial.begin(115200);
pwm.begin();
pwm.setPWMFreq(50);
delay(500);

if (!ina219.begin()) {
Serial.println(" INA219 non détecté !");
while (1);
}

Serial.println(" Système prêt — commande : open / close");
OpenHand();
}

void loop() {
// Lecture commandes du moniteur série
if (Serial.available()) {
String cmd = Serial.readStringUntil('\n');
cmd.trim();


if (cmd.equalsIgnoreCase("open")) {
  OpenHand();
} else if (cmd.equalsIgnoreCase("close")) {
  CloseHand();
} else {
  Serial.println(" Commande inconnue — options : open / close");
}


}

// Affichage du courant en continu
printIna219Data();
delay(500);
}

// === FONCTION : RELÂCHEMENT PROGRESSIF ===
void gradualReleaseUntilSafe() {
float current_mA = ina219.getCurrent_mA();
Serial.println(" Relâchement progressif en cours...");

while (current_mA > SAFE_CURRENT_MA) {
bool anyMoved = false;


for (int i = 0; i < 8; i++) {
  int target = OPEN_TARGET[i];
  int pos = MiddlePos[i];

  if (pos < target) {
    pos = min(pos + 2, target);
    anyMoved = true;
  } else if (pos > target) {
    pos = max(pos - 2, target);
    anyMoved = true;
  }

  MiddlePos[i] = pos;
  pwm.setPWM(i, 0, angleToPulse(pos));
}

if (!anyMoved) {
  Serial.println(" Position ouverte atteinte, arrêt du relâchement.");
  break;
}

delay(100);
current_mA = ina219.getCurrent_mA();
Serial.print(" Courant : ");
Serial.print(current_mA);
Serial.println(" mA");


}

Serial.println(" Pression redevenue normale (ou ouverture complète atteinte).");
}

// === FONCTION : SURVEILLANCE ET RELÂCHEMENT AUTOMATIQUE ===
void checkThumbForceAndRelease() {
float current_mA = ina219.getCurrent_mA();

if (current_mA > CURRENT_THRESHOLD_MA) {
Serial.print(" Force détectée : ");
Serial.print(current_mA);
Serial.println(" mA → relâchement en cours...");
gradualReleaseUntilSafe();
}
}

// === MOUVEMENTS ===
void OpenHand() {
int target[8] = {130, 40, 130, 40, 140, 40, 140, 40};
moveAllServosTogether(target, MaxSpeed);
Serial.println("Main ouverte");
}

void CloseHand() {
int target[8] = {20, 160, 10, 170, 10, 170, 10, 110};
moveAllServosTogether(target, CloseSpeed);
Serial.println(" Main fermée");

// Vérification de la force après fermeture
checkThumbForceAndRelease();
}

// === MOUVEMENT SYNCHRONISÉ ===
void moveAllServosTogether(int targetAngles[8], int delayTime) {
int maxDiff = 0;
for (int i = 0; i < 8; i++) {
int diff = abs(targetAngles[i] - MiddlePos[i]);
if (diff > maxDiff) maxDiff = diff;
}

for (int step = 0; step <= maxDiff; step++) {
for (int i = 0; i < 8; i++) {
int start = MiddlePos[i];
int target = targetAngles[i];
int newAngle = start;


  if (target > start)
    newAngle = start + min(step, target - start);
  else if (target < start)
    newAngle = start - min(step, start - target);

  pwm.setPWM(i, 0, angleToPulse(newAngle));
}
delay(delayTime);


}

for (int i = 0; i < 8; i++) {
MiddlePos[i] = targetAngles[i];
}
}

// === AFFICHAGE INA219 ===
void printIna219Data() {
float busVoltage_V = ina219.getBusVoltage_V();
float shuntVoltage_mV = ina219.getShuntVoltage_mV();
float current_mA = ina219.getCurrent_mA();
float power_mW = ina219.getPower_mW();

Serial.print(" Bus: ");
Serial.print(busVoltage_V);
Serial.print(" V | Shunt: ");
Serial.print(shuntVoltage_mV);
Serial.print(" mV | Courant: ");
Serial.print(current_mA);
Serial.print(" mA | Puissance: ");
Serial.print(power_mW);
Serial.println(" mW");
}
