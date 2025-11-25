#include <Wire.h>
#include <LiquidCrystal_I2C.h> //Librería para el display.

#define PIN_SENSOR_1_Z1 A0 //Definimos entradas de los sensores
#define PIN_SENSOR_2_Z1 A1 //y reles para una fácil configuración.
#define PIN_SENSOR_Z2 A2
#define PIN_SENSOR_POWER 13

#define PIN_RELE_VALVULA_1 4
#define PIN_RELE_VALVULA_2 5
#define PIN_RELE_BOMBA 6

int valorSeco = 0; //Definimos los criterios para las
int valorHumedo = 863; //mediciones.

int humedadMinima = 30; //

unsigned long intervaloMedicion = 10000UL; //10segundos //Usamos unsigned long porque son valores referidos a los milisegundos
unsigned long intervaloRiego = 5000UL; //5segundos //de tiempo que van pasando desde que encendió Arduino, por lo que
unsigned long intervaloDisplay = 500UL; //0.5segundos //se toman valores muy altos.

LiquidCrystal_I2C lcd(0x27, 16, 2); //Seteamos el display.

class InputDevice { //Clase base abstracta para dispositivos genéricos.
public:
    virtual int leerPromedio() = 0;
    virtual int leerPorcentaje() = 0;
};

class sensorHumedad : public InputDevice {
private:
    int pin;
public:
    sensorHumedad(int p): pin(p) { //Constructor por parametro.
        pinMode(pin, INPUT);
    }
    
    int leerPromedio() { //Sensa 5 veces y retorna el promedio.
        int suma = 0;
        for (int i = 0; i < 5; i++) {
            suma += analogRead(pin);
        }
        return (suma / 5);
    }
    
    int leerPorcentaje() {
        int porcentaje = map(leerPromedio(), 0, 1023, 0, 100);
        return porcentaje;
    }
};

class Zona {
private:
    sensorHumedad* s1;
    sensorHumedad* s2;
    int pinValvula;
    bool estadoRiego;
    int humedadActual; //En porcentaje
    unsigned long inicioRiego;
    
public:
    Zona(int pinS1, int pinV) { //Constructor por parametro.
        s1 = new sensorHumedad(pinS1);
        s2 = nullptr;
        inicializar(pinV); //metodo de la zona.
    }
    
    Zona(int pinS1, int pinS2, int pinV) { //Constructor por parametro.
        s1 = new sensorHumedad(pinS1);
        s2 = new sensorHumedad(pinS2);
        inicializar(pinV);
    }
    
    ~Zona() { //Destructor
        delete s1;
        if (s2 != nullptr) {
            delete s2;
        }
    }
    void inicializar(int pinV) {
        pinValvula = pinV;
        pinMode(pinValvula, OUTPUT);
        digitalWrite(pinValvula, LOW);
        estadoRiego = 0;
        humedadActual = 0;
        inicioRiego = 0;
    }
    
    void actualizar(unsigned long Tiempo) {
        if (estadoRiego) {
            if (Tiempo - inicioRiego >= intervaloRiego) {
                estadoRiego = 0;
                digitalWrite(pinValvula, LOW);
            }
        }
    }
    
    void verificarHumedad() {
        int h1 = s1->leerPorcentaje();
        int lecturaFinal = h1;
        
        if (s2 != nullptr) { //Porque una de las zonas solo tiene un sensor (s1).
            int h2 = s2->leerPorcentaje();
            lecturaFinal = (h1 + h2) / 2;
        }
        humedadActual = lecturaFinal;
        
        if (!estadoRiego && humedadActual <= humedadMinima) {
            estadoRiego = 1;
            inicioRiego = millis();
            digitalWrite(pinValvula, HIGH);
        }
    }
    
    int getHumedad() {
        return humedadActual;
    }
    
    bool getEstado() {
        return estadoRiego;
    }
};

Zona zona1(PIN_SENSOR_1_Z1, PIN_SENSOR_2_Z1, PIN_RELE_VALVULA_1);
Zona zona2(PIN_SENSOR_Z2, PIN_RELE_VALVULA_2);

unsigned long ultimaMedida = 0; //Tiempo desde la última medida.
unsigned long ultimoDisplay = 0; //Tiempo desde
bool firstRun = 1; //Caso particular, primera corrida.

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
    unsigned long Tiempo = millis(); //Almacena el tiempo en milisegundos desde que Arduino
    //encendió (devuelve unsigned long).
    zona1.actualizar(Tiempo); //Chequea que si las valvulas están encendidas, se cierren
    zona2.actualizar(Tiempo); //si se superó el intervalo de riego establecido.
    
    if (Tiempo - ultimaMedida >= intervaloMedicion || firstRun) {
        
        digitalWrite(PIN_SENSOR_POWER, HIGH);
        delay(100);
        
        zona1.verificarHumedad();
        zona2.verificarHumedad();
        
        digitalWrite(PIN_SENSOR_POWER, LOW);
        
        ultimaMedida = Tiempo;
        firstRun = 0;
    }
    
    if (zona1.getEstado() || zona2.getEstado()) { //primero abre valvula luego enciende bomba.
        digitalWrite(PIN_RELE_BOMBA, HIGH);
    } else {
        digitalWrite(PIN_RELE_BOMBA, LOW);
    }
    
    if (Tiempo - ultimoDisplay >= intervaloDisplay) {
        lcd.setCursor(0, 0);
        lcd.print(" "); // 16 espacios
        lcd.setCursor(0, 0);
        lcd.print("Zona 1: "); lcd.print(zona1.getHumedad()); lcd.print("% ");
        lcd.print(zona1.getEstado() ? "ON " : "OFF");
        
        lcd.setCursor(0, 1);
        lcd.print(" "); // 16 espacios
        lcd.setCursor(0, 1);
        lcd.print("Zona 2: "); lcd.print(zona2.getHumedad()); lcd.print("% ");
        lcd.print(zona2.getEstado() ? "ON " : "OFF");
        
        ultimoDisplay = Tiempo;
          }
}
