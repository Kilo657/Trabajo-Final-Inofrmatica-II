#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define PIN_SENSOR_Z1_A  A0
#define PIN_SENSOR_Z1_B  A1
#define PIN_SENSOR_Z2    A2
#define PIN_SENSOR_POWER 13 

#define PIN_RELE_VALVULA_1  4
#define PIN_RELE_VALVULA_2  5
#define PIN_RELE_BOMBA      6

int valorSeco   = 0;    
int valorHumedo = 863;  

int humedadMinima = 10; 

unsigned long intervaloMed = 3600000UL; 
unsigned long intervaloRieg     = 600000UL;  
unsigned long intervaloDisplay  = 500UL;

LiquidCrystal_I2C lcd(0x27, 16, 2);

class InputDevice {
public:
	virtual int leerPromedioRaw() = 0;
	virtual int leerPorcentaje() = 0;
};

class sensorHumedad : public InputDevice {
private:
	uint8_t pin;
public:
	sensorHumedad(uint8_t p) {
		pin = p;
		pinMode(pin, INPUT);
	}
	
	int leerPromedioRaw() override {
		long suma = 0;
		for(int i=0; i<5; i++){
			suma += analogRead(pin);
		}
		return (int)(suma / 5);
	}
	
	int leerPorcentaje() override {
		int raw = leerPromedioRaw();
		long pct = map(raw, valorSeco, valorHumedo, 0, 100);
		return constrain((int)pct, 0, 100);
	}
};

class Zone {
private:
	sensorHumedad* s1;
	sensorHumedad* s2;
	uint8_t pinValvula;
	bool estadoRiego; 
	int humedadActual;
	unsigned long inicioRiego;
	
public:
	Zone(int pinS1, int pinV) {
		s1 = new sensorHumedad(pinS1);
		s2 = nullptr;
		init(pinV);
	}
	
	Zone(int pinS1, int pinS2, int pinV) {
		s1 = new sensorHumedad(pinS1);
		s2 = new sensorHumedad(pinS2);
		init(pinV);
	}
	
	void init(int pinV) {
		pinValvula = pinV;
		pinMode(pinValvula, OUTPUT);
		digitalWrite(pinValvula, LOW);
		estadoRiego = false;
		humedadActual = 0;
		inicioRiego = 0;
	}
	
	void actualizar(unsigned long now) {
		if (estadoRiego) {
			if (now - inicioRiego >= intervloRieg) {
				estadoRiego = false;
				digitalWrite(pinValvula, LOW);
			}
		}
	}
	
	void verificarHumedad() {
		int h1 = s1->leerPorcentaje();
		int lecturaFinal = h1;
		
		if (s2 != nullptr) {
			int h2 = s2->leerPorcentaje();
			lecturaFinal = (h1 + h2) / 2;
		}
		humedadActual = lecturaFinal;
		
		if (!estadoRiego && humedadActual <= humedadMinima) {
			estadoRiego = true;
			inicioRiego = millis();
			digitalWrite(pinValvula, HIGH);
		}
	}
	
	int getHumedad() { return humedadActual; }
	bool estaRegando() { return estadoRiego; }
};

Zone zona1(PIN_SENSOR_Z1_A, PIN_SENSOR_Z1_B, PIN_RELE_VALVULA_1); 
Zone zona2(PIN_SENSOR_Z2, PIN_RELE_VALVULA_2);                    

unsigned long lastMeasure = 0;
unsigned long lastDisplay = 0;
bool firstRun = true;

void setup() {
	pinMode(PIN_SENSOR_POWER, OUTPUT);
	digitalWrite(PIN_SENSOR_POWER, LOW); 
	
	pinMode(PIN_RELE_BOMBA, OUTPUT);
	digitalWrite(PIN_RELE_BOMBA, LOW);
	
	lcd.init();
	lcd.backlight();
	lcd.clear();
}

void loop() {
	unsigned long now = millis();
	
	zona1.actualizar(now);
	zona2.actualizar(now);
	
	if (now - lastMeasure >= intervaloMed || firstRun) {
		
		digitalWrite(PIN_SENSOR_POWER, HIGH);
		delay(100); 
		
		zona1.verificarHumedad();
		zona2.verificarHumedad();
		
		digitalWrite(PIN_SENSOR_POWER, LOW); 
		
		lastMeasure = now;
		firstRun = false;
	}
	
	if (zona1.estaRegando() || zona2.estaRegando()) {
		digitalWrite(PIN_RELE_BOMBA, HIGH);
	} else {
		digitalWrite(PIN_RELE_BOMBA, LOW);
	}
	
	if (now - lastDisplay >= intervaloDisplay) {
		lcd.setCursor(0, 0);
		lcd.print("Zona 1:"); lcd.print(zona1.getHumedad()); lcd.print("% ");
		lcd.print(zona1.estaRegando() ? "ON " : "   ");
		
		lcd.setCursor(0, 1);
		lcd.print("Zona 2:"); lcd.print(zona2.getHumedad()); lcd.print("% ");
		lcd.print(zona2.estaRegando() ? "ON " : "   ");
		
		lastDisplay = now;
	}
}