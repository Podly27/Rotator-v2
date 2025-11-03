/*
 * Arduino Rotátor s víceotáčkovým potenciometrem
 * Převodový poměr: 96:16 (6:1)
 * TFT displej: ST7789V 240x320
 * 
 * Autor: AI Assistant
 * Verze: 1.0
 */

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

// Definice pinů
#define TFT_CS    10
#define TFT_RST   9
#define TFT_DC    8
#define POT_PIN   A0
#define RELAY_LEFT_PIN  2
#define RELAY_RIGHT_PIN 3

// Konstanty pro potenciometr
#define POT_MAX_VALUE    1023    // Maximální hodnota ADC (10-bit)
#define POT_MAX_TURNS    10      // Počet otáček potenciometru
#define POT_DEGREES_PER_TURN 360 // Stupně na otáčku

// Konstanty pro převod
#define GEAR_RATIO       6       // Převodový poměr (96:16)
#define ROTATOR_MAX_DEG  600     // Maximální rozsah rotátoru (360° ± 120°)
#define ROTATOR_CENTER   360     // Střední pozice rotátoru

// Bezpečnostní limity
#define SAFETY_LIMIT_HIGH  450   // Horní bezpečnostní limit (360° + 90°)
#define SAFETY_LIMIT_LOW   270   // Dolní bezpečnostní limit (360° - 90°)

// Inicializace TFT displeje
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// Globální proměnné
float currentRotatorAngle = 0;
float currentPotRatio = 0;
bool safetyLimitReached = false;
int lastPotValue = 0;

// Proměnné pro optimalizaci vykreslování (zapamatování předchozí pozice)
float lastDrawnAngle = -1;
int lastX1 = 0, lastY1 = 0, lastX2 = 0, lastY2 = 0, lastX3 = 0, lastY3 = 0;
String lastAngleStr = "";

void setup() {
  Serial.begin(9600);
  Serial.println("Rotátor - inicializace...");
  
  // Inicializace pinů
  pinMode(RELAY_LEFT_PIN, OUTPUT);
  pinMode(RELAY_RIGHT_PIN, OUTPUT);
  digitalWrite(RELAY_LEFT_PIN, HIGH);   // Relé vypnuto (HIGH = vypnuto pro aktivní LOW relé)
  digitalWrite(RELAY_RIGHT_PIN, HIGH);
  
  // Inicializace TFT displeje
  tft.init(240, 320);
  tft.setRotation(3); // Otočeno o 180°
  tft.fillScreen(ST77XX_BLACK);
  
  // Načtení počáteční pozice
  loadInitialPosition();
  
  // Úvodní obrazovka
  drawCompass();
  
  // Vynutit překreslení při prvním zobrazení
  lastAngleStr = "XXX";
  
  updateDisplay();
  
  Serial.println("Rotátor připraven!");
}

void loop() {
  // Čtení hodnoty potenciometru
  int potValue = analogRead(POT_PIN);
  
  // Kontrola změny pozice
  if (abs(potValue - lastPotValue) > 2) { // Hystereze pro stabilitu
    updatePosition(potValue);
    checkSafetyLimits();
    updateDisplay();
    lastPotValue = potValue;
    
    // Debug výstup
    Serial.print("Pot: ");
    Serial.print(potValue);
    Serial.print(" | Rotátor: ");
    Serial.print(currentRotatorAngle, 1);
    Serial.print("° | Poměr: ");
    Serial.print(currentPotRatio, 2);
    Serial.println();
  }
  
  delay(50); // Krátká pauza pro stabilitu
}

void loadInitialPosition() {
  int potValue = analogRead(POT_PIN);
  updatePosition(potValue);
  lastPotValue = potValue;
  
  Serial.print("Počáteční pozice načtena: ");
  Serial.print(currentRotatorAngle, 1);
  Serial.println("°");
}

void updatePosition(int potValue) {
  // Výpočet poměru natočení potenciometru (0.0 - 1.0)
  currentPotRatio = (float)potValue / POT_MAX_VALUE;
  
  // Výpočet úhlu potenciometru (0° - 3600°)
  float potAngle = currentPotRatio * POT_MAX_TURNS * POT_DEGREES_PER_TURN;
  
  // Střed potenciometru (50% = 5 otáček = 1800°) odpovídá 180° azimutu (jih)
  float potAngleFromCenter = potAngle - 1800.0; // -1800° až +1800°
  
  // Výpočet azimutu: každá otáčka potenciometru = 60° azimutu
  // potAngleFromCenter / 6 = azimut od středu (180°)
  currentRotatorAngle = (potAngleFromCenter / GEAR_RATIO) + 180.0;
  
  // Normalizace azimutu na rozsah 0° - 360°
  while (currentRotatorAngle < 0) currentRotatorAngle += 360;
  while (currentRotatorAngle >= 360) currentRotatorAngle -= 360;
}

void checkSafetyLimits() {
  bool wasLimitReached = safetyLimitReached;
  safetyLimitReached = false;
  
  // Kontrola podle procent potenciometru (0-10% a 90-100%)
  float potPercent = currentPotRatio * 100;
  
  if (potPercent >= 90) {
    safetyLimitReached = true;
    digitalWrite(RELAY_RIGHT_PIN, LOW); // Vypnutí motoru doprava
    Serial.println("BEZPEČNOSTNÍ LIMIT - Horní limit (90-100%)!");
  } else if (potPercent <= 10) {
    safetyLimitReached = true;
    digitalWrite(RELAY_LEFT_PIN, LOW); // Vypnutí motoru doleva
    Serial.println("BEZPEČNOSTNÍ LIMIT - Dolní limit (0-10%)!");
  } else {
    // Vypnutí všech relé když jsme v bezpečném rozsahu
    digitalWrite(RELAY_LEFT_PIN, HIGH);
    digitalWrite(RELAY_RIGHT_PIN, HIGH);
  }
  
  if (safetyLimitReached && !wasLimitReached) {
    // První dosažení limitu - zvukový signál
    tone(7, 1000, 500); // Piezo na pinu 7
  }
}

void drawCompass() {
  tft.fillScreen(ST77XX_BLACK);
  
  // Nakreslení růžice
  int centerX = tft.width() / 2;
  int centerY = tft.height() / 2;
  int radius = min(centerX, centerY) - 10; // Větší kružnice (šipka bude uvnitř)
  
  // Jedna vnější kružnice
  tft.drawCircle(centerX, centerY, radius, ST77XX_WHITE);
}

void updateDisplay() {
  int centerX = tft.width() / 2;
  int centerY = tft.height() / 2;
  int radius = min(centerX, centerY) - 10; // Stejný radius jako růžice
  
  // Vymazání staré šipky (černým trojúhelníkem)
  if (lastDrawnAngle >= 0) {
    tft.fillTriangle(lastX1, lastY1, lastX2, lastY2, lastX3, lastY3, ST77XX_BLACK);
    // Překreslení kruhu na místě kde byla šipka
    tft.drawCircle(centerX, centerY, radius, ST77XX_WHITE);
  }
  
  // Nová šipka - trojúhelník UVNITŘ kružnice
  float arrowAngle = radians(currentRotatorAngle - 90); // -90° pro správnou orientaci (0° = nahoru)
  int arrowLength = radius - 25; // Délka šipky od středu
  
  // Špička trojúhelníku (blízko vnějšího kruhu)
  int x1 = centerX + cos(arrowAngle) * (radius - 10);
  int y1 = centerY + sin(arrowAngle) * (radius - 10);
  
  // Levý roh základny (blíž ke středu)
  float leftAngle = arrowAngle - 0.15;
  int x2 = centerX + cos(leftAngle) * arrowLength;
  int y2 = centerY + sin(leftAngle) * arrowLength;
  
  // Pravý roh základny (blíž ke středu)
  float rightAngle = arrowAngle + 0.15;
  int x3 = centerX + cos(rightAngle) * arrowLength;
  int y3 = centerY + sin(rightAngle) * arrowLength;
  
  // Nakreslení červeného trojúhelníku
  tft.fillTriangle(x1, y1, x2, y2, x3, y3, ST77XX_RED);
  
  // Zapamatování pozice pro příští vymazání
  lastX1 = x1; lastY1 = y1;
  lastX2 = x2; lastY2 = y2;
  lastX3 = x3; lastY3 = y3;
  lastDrawnAngle = currentRotatorAngle;
  
  // Zobrazení úhlu rotátoru - velký standardní font
  int angle = (int)currentRotatorAngle;
  String angleStr = String(angle);
  
  // Pokud se číslo změnilo
  if (lastAngleStr != angleStr) {
    // Přepsat staré číslo černou (pokud existuje a není "XXX")
    if (lastAngleStr != "" && lastAngleStr != "XXX") {
      tft.setTextSize(6);
      tft.setTextColor(ST77XX_BLACK);
      
      // Vypočítat pozici starého textu
      int oldTextWidth = lastAngleStr.length() * 36; // přibližná šířka pro font velikost 6
      tft.setCursor(centerX - oldTextWidth/2, centerY - 24);
      tft.print(lastAngleStr);
    }
    
    // Nastavit velký font (velikost 6 - maximum)
    tft.setTextSize(6);
    tft.setTextColor(ST77XX_YELLOW);
    
    // Vypočítat šířku textu pro vystředění
    int textWidth = angleStr.length() * 36; // přibližná šířka pro font velikost 6
    
    // Nakreslit žlutý text
    tft.setCursor(centerX - textWidth/2, centerY - 24);
    tft.print(angleStr);
    
    // Zapamatovat poslední zobrazený text
    lastAngleStr = angleStr;
  }
  
  // Zobrazení poměru potenciometru - levý dolní roh (jen procenta)
  tft.fillRect(0, tft.height() - 25, 80, 25, ST77XX_BLACK); // Vymazání levého dolního rohu
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(10, tft.height() - 20);
  tft.print((int)(currentPotRatio * 100));
  tft.print("%");
  
  // Zobrazení bezpečnostního stavu - pravý dolní roh
  if (safetyLimitReached) {
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_RED);
    tft.setCursor(tft.width() - 80, tft.height() - 20);
    tft.print("LIMIT!");
  } else {
    tft.fillRect(tft.width() - 80, tft.height() - 25, 80, 25, ST77XX_BLACK);
  }
}

// Funkce pro manuální ovládání (volitelné)
void emergencyStop() {
  digitalWrite(RELAY_LEFT_PIN, HIGH);
  digitalWrite(RELAY_RIGHT_PIN, HIGH);
  safetyLimitReached = true;
  Serial.println("EMERGENCY STOP!");
}

void resetSafetyLimits() {
  safetyLimitReached = false;
  digitalWrite(RELAY_LEFT_PIN, HIGH);
  digitalWrite(RELAY_RIGHT_PIN, HIGH);
  Serial.println("Bezpečnostní limity resetovány");
}
