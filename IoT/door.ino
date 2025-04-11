#include <WiFi.h>
#include <Adafruit_Fingerprint.h>
#include <Keypad.h>
#include <ESP32Servo.h>
#include <EEPROM.h>
#include <HTTPClient.h>

// ================== Configuración ==================
const char* ssid = "CARRYENGINER2";
const char* password = "12345678";
const char* apiUrl = "http://172.16.1.95:3000/gg/doors";

// Inicialización de sensores
HardwareSerial mySerial(2); // UART2
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// ========== Pines del teclado ==========
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {16, 17, 18, 19};
byte colPins[COLS] = {21, 22, 23, 25};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Servos
Servo servo1;
Servo servo2;
Servo servo3;

// Variables globales
int userId = -1;
String claveApi = "";
bool registrarHuella = false;
bool registrarClave = false;
String nombreUsuario = "";

void setup() {
  Serial.begin(115200);
  Serial.println("🛠️ Iniciando el sistema...");

  WiFi.disconnect(true); // Limpia redes previas
  delay(1000);
  WiFi.begin(ssid, password);

  Serial.print("Conectando a WiFi: ");
  Serial.println(ssid);
  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 30) {
    delay(500);
    Serial.print("...");
    intentos++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Conectado a WiFi");
    Serial.print("Dirección IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ Error: no se pudo conectar a WiFi");
  }

  // Iniciar lector de huellas
  mySerial.begin(57600, SERIAL_8N1, 4, 5); // RX=4, TX=5
  finger.begin(57600);
  if (finger.verifyPassword()) {
    Serial.println("✅ Módulo de huellas dactilares encontrado!");
  } else {
    Serial.println("❌ No se encontró el módulo de huellas");
    while (1) delay(1);
  }

  // Servos
  servo1.attach(32);
  servo2.attach(33);
  servo3.attach(26);
  servo1.write(0);
  servo2.write(180);
  servo3.write(0);
  Serial.println("🛠️ Servos inicializados en posición 0");

  EEPROM.begin(512);
  Serial.println("📚 EEPROM iniciada");
  cargarClave();

  mostrarOpciones();
}

void loop() {
  if (Serial.available() > 0) {
    char opcion = Serial.read();
    procesarOpcion(opcion);
  }

  char key = keypad.getKey();
  if (key) {
    Serial.print("⌨️ Tecla presionada: ");
    Serial.println(key);
    procesarOpcion(key);
  }

  delay(100);
}

void mostrarOpciones() {
  Serial.println("\n=== Opciones disponibles ===");
  Serial.println("1: Registrar nueva huella");
  Serial.println("2: Abrir puerta manualmente");
  Serial.println("3: Verificar huella");
  Serial.println("============================");
}

void procesarOpcion(char opcion) {
  switch (opcion) {
    case '1':
      Serial.println("📝 Modo de registro de huellas activado");
      registrarHuella = true;
      capturarNombreUsuario();
      break;
    case '2':
      Serial.println("🔓 Apertura manual solicitada...");
      activarServos();
      break;
    case '3':
      Serial.println("🔍 Verificando huella...");
      leerHuella();
      break;
    default:
      Serial.println("❌ Opción no válida");
      mostrarOpciones();
      break;
  }
}
void capturarNombreUsuario() {
  Serial.println("📝 Escribe el nombre del usuario en la consola y presiona Enter:");

  // Esperar hasta que haya datos disponibles en el monitor serie
  while (Serial.available() == 0) {
    delay(100); // Pequeña pausa para evitar un bucle ocupado
  }

  // Leer el nombre completo ingresado por el usuario
  nombreUsuario = Serial.readStringUntil('\n'); // Leer hasta que se presione Enter
  nombreUsuario.trim(); // Eliminar espacios en blanco al inicio y al final

  // Validar que el nombre no esté vacío
  if (nombreUsuario.isEmpty()) {
    Serial.println("❌ Error: El nombre del usuario no puede estar vacío.");
    registrarHuella = false; // Cancelar el registro
    mostrarOpciones();
    return;
  }

  Serial.println("✅ Nombre capturado: " + nombreUsuario);
  registrarNuevaHuella();
}

void leerHuella() {
  Serial.println("🔍 Buscando huella...");
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) {
    Serial.println("❌ Error al capturar huella");
    return;
  }

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) {
    Serial.println("❌ Error al convertir imagen a plantilla");
    return;
  }

  p = finger.fingerSearch();
  if (p != FINGERPRINT_OK) {
    Serial.println("❌ No se encontró la huella");
    return;
  }

  userId = finger.fingerID;
  Serial.print("✅ Huella encontrada con ID: ");
  Serial.println(userId);

  if (verificarPermisoApi(userId)) {
    activarServos();
  } else {
    Serial.println("🚫 Acceso denegado por la API");
  }
}

int obtenerProximoID() {
  Serial.println("🔍 Buscando el próximo ID disponible...");
  for (int id = 1; id < 128; id++) { // El módulo soporta hasta 127 huellas
    if (finger.loadModel(id) != FINGERPRINT_OK) {
      Serial.println("✅ ID disponible encontrado: " + String(id));
      return id;
    }
  }
  Serial.println("❌ No hay IDs disponibles. La memoria del módulo podría estar llena.");
  return -1; // Indica que no hay espacio disponible
}

void registrarNuevaHuella() {
  Serial.println("📝 Coloca tu dedo para registrar...");
  int p = -1;

  // Buscar el próximo ID disponible
  userId = obtenerProximoID();
  if (userId == -1) {
    Serial.println("❌ No se puede registrar la huella. La memoria del módulo está llena.");
    return;
  }

  // Capturar la primera imagen de la huella
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) continue;
    if (p != FINGERPRINT_OK) {
      Serial.println("❌ Error capturando imagen. Código de error: " + String(p));
      return;
    }
  }
  Serial.println("✅ Primera imagen capturada correctamente.");

  // Convertir la primera imagen en plantilla
  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    Serial.println("❌ Error al convertir la primera imagen. Código de error: " + String(p));
    return;
  }
  Serial.println("✅ Primera imagen convertida a plantilla.");

  Serial.println("👉 Retira el dedo...");
  delay(2000);
  while (finger.getImage() != FINGERPRINT_NOFINGER);

  Serial.println("👉 Vuelve a colocar el mismo dedo...");
  while ((p = finger.getImage()) != FINGERPRINT_OK) {
    if (p == FINGERPRINT_NOFINGER) continue;
    Serial.println("❌ Error capturando segunda imagen. Código de error: " + String(p));
    return;
  }
  Serial.println("✅ Segunda imagen capturada correctamente.");

  // Convertir la segunda imagen en plantilla
  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) {
    Serial.println("❌ Error al convertir la segunda imagen. Código de error: " + String(p));
    return;
  }
  Serial.println("✅ Segunda imagen convertida a plantilla.");

  // Crear el modelo de la huella
  p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    Serial.println("❌ Error al crear el modelo de la huella. Código de error: " + String(p));
    return;
  }
  Serial.println("✅ Modelo de huella creado correctamente.");

  // Guardar la huella en el módulo
  p = finger.storeModel(userId);
  if (p == FINGERPRINT_OK) {
    Serial.println("✅ Huella registrada correctamente en el módulo con ID: " + String(userId));

    // Enviar huella a la API
    enviarHuellaApi(userId, nombreUsuario);
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("❌ Error de comunicación con el módulo al guardar la huella.");
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("❌ Las huellas no coinciden. Intenta nuevamente.");
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("❌ ID de ubicación inválido.");
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("❌ Error al escribir en la memoria del módulo.");
  } else {
    Serial.println("❌ Error desconocido al guardar la huella. Código de error: " + String(p));
  }

  // Reiniciar variables
  registrarHuella = false;
  nombreUsuario = "";
}

void enviarHuellaApi(int fingerprint_id, String name) {
  Serial.println("📤 Preparando para enviar datos a la API...");

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("🚫 No hay conexión WiFi para registrar huella.");
    return;
  }

  if (fingerprint_id <= 0 || name.isEmpty()) {
    Serial.println("❌ Error: Datos incompletos para registrar huella.");
    Serial.println("fingerprint_id: " + String(fingerprint_id) + ", name: " + name);
    return;
  }

  HTTPClient http;
  String url = String(apiUrl) + "/registerFingerprint";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  // Crear el payload con los datos de la huella
  String payload = "{\"door_id\":\"100\",\"fingerprint_id\":\"" + String(fingerprint_id) + "\",\"name\":\"" + name + "\"}";
  Serial.println("📤 Enviando datos a la API: " + payload);

  int httpCode = http.POST(payload);

  if (httpCode > 0) {
    String response = http.getString();
    Serial.print("📦 Respuesta de API al registrar huella: ");
    Serial.println(response);

    if (httpCode == 200) {
      Serial.println("✅ Huella registrada correctamente en la base de datos.");
    } else {
      Serial.println("❌ Error al registrar huella en la base de datos. Código HTTP: " + String(httpCode));
    }
  } else {
    Serial.print("❌ Error al conectar con la API. Código HTTP: ");
    Serial.println(httpCode);
  }

  http.end();
}
void activarServos() {
  Serial.println("🛠️ Activando servos...");
  servo3.write(90);
  delay(1000);
  servo1.write(90);
  servo2.write(90);
  
  delay(2000);
  servo1.write(0);
  servo2.write(180);
  delay(1000);
  servo3.write(0);
  Serial.println("✅ Servos regresados a posición 0");

  // Registrar log en la API
  registrarLogApi();
}

void registrarLogApi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("🚫 No hay conexión WiFi para registrar log");
    return;
  }

  HTTPClient http;
  String url = String(apiUrl) + "/registerLog";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  // Enviar el ID de usuario (huella) al servidor
  String payload = "{\"door_id\":\"100\",\"fingerprint_id\":\"" + String(userId) + "\",\"action\":\"Door Opened\"}";
  int httpCode = http.POST(payload);

  if (httpCode > 0) {
    Serial.print("📦 Respuesta de API al registrar log: ");
    Serial.println(http.getString());
  } else {
    Serial.print("❌ Error al registrar log: ");
    Serial.println(httpCode);
  }

  http.end();
}

void guardarClave(String clave) {
  EEPROM.write(0, clave[0]);
  EEPROM.commit();
  Serial.println("🔐 Clave guardada en EEPROM");
}

void cargarClave() {
  claveApi = String(char(EEPROM.read(0)));
  Serial.print("Clave cargada: ");
  Serial.println(claveApi);
}

bool verificarPermisoApi(int id) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("🚫 No hay conexión WiFi");
    return false;
  }

  HTTPClient http;
  String url = String(apiUrl) + "/verificar/?id=" + String(id);
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();
    Serial.print("📦 Respuesta de API: ");
    Serial.println(payload);
    http.end();
    return payload.indexOf("true") >= 0;
  } else {
    Serial.print("❌ Error en la solicitud: ");
    Serial.println(httpCode);
    http.end();
    return false;
  }
}