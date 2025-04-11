# Proyecto: Sistema de Control de Puerta con Arduino y ESP32

Este proyecto implementa un sistema de control de acceso para puertas utilizando un ESP32, un lector de huellas dactilares, un teclado matricial y servomotores. Además, se conecta a una API para verificar permisos y registrar eventos.

## Características principales

- **Autenticación por huella dactilar**: Permite registrar y verificar huellas dactilares.
- **Control de acceso remoto**: Se conecta a una API para verificar permisos y registrar logs.
- **Control manual**: Permite abrir la puerta manualmente mediante un teclado matricial.
- **Registro de usuarios**: Los usuarios pueden ser registrados con su nombre y huella dactilar.
- **Control de servos**: Los servomotores controlan la apertura y cierre de la puerta.

---

## Requisitos de hardware

1. **ESP32**: Microcontrolador principal.
2. **Lector de huellas dactilares**: Para la autenticación biométrica.
3. **Teclado matricial 4x4**: Para ingresar comandos manuales.
4. **Servomotores**: Para controlar la apertura y cierre de la puerta.
5. **EEPROM**: Para almacenar configuraciones como la clave de la API.
6. **Conexión WiFi**: Para interactuar con la API.

---

## Configuración inicial

1. **Conexión WiFi**:
   - Modifica las siguientes líneas en el código para configurar tu red WiFi:
     ```cpp
     const char* ssid = "TU_SSID";
     const char* password = "TU_PASSWORD";
     ```

2. **API**:
   - Configura la URL de tu API en la siguiente línea:
     ```cpp
     const char* apiUrl = "http://TU_API_URL/gg/doors";
     ```

3. **Pines**:
   - Asegúrate de conectar correctamente los pines del lector de huellas, teclado y servos según las configuraciones del código.

---

## Uso del sistema

### 1. Encender el sistema
- Conecta el ESP32 y abre el monitor serie a una velocidad de 115200 baudios.
- El sistema intentará conectarse a la red WiFi configurada.

### 2. Opciones disponibles
El sistema mostrará las siguientes opciones en el monitor serie:


### 3. Registrar una nueva huella
- Presiona `1` en el monitor serie.
- Ingresa el nombre del usuario y sigue las instrucciones para colocar el dedo en el lector de huellas.
- La huella será registrada en el módulo y enviada a la API.

### 4. Abrir la puerta manualmente
- Presiona `2` en el monitor serie o en el teclado matricial.
- Los servos se activarán para abrir la puerta.

### 5. Verificar una huella
- Presiona `3` en el monitor serie o coloca un dedo en el lector.
- El sistema buscará la huella en el módulo y verificará los permisos con la API.
- Si la huella es válida, los servos abrirán la puerta.

---

## Funciones principales del código

### `setup()`
- Configura la conexión WiFi, inicializa el lector de huellas, los servos y la EEPROM.

### `loop()`
- Escucha comandos desde el monitor serie o el teclado matricial y ejecuta las acciones correspondientes.

### `registrarNuevaHuella()`
- Registra una nueva huella en el módulo y la envía a la API.

### `leerHuella()`
- Verifica una huella dactilar y consulta los permisos en la API.

### `activarServos()`
- Controla los servos para abrir y cerrar la puerta.

### `enviarHuellaApi()`
- Envía los datos de una nueva huella a la API.

### `registrarLogApi()`
- Registra un log en la API cuando se abre la puerta.

---

## API utilizada

El sistema interactúa con una API REST para registrar y verificar huellas, así como para registrar logs. Las rutas principales son:

1. **Registrar huella**: `POST /registerFingerprint`
   - Payload:
     ```json
     {
       "door_id": "100",
       "fingerprint_id": "ID_DE_HUELLA",
       "name": "NOMBRE_DEL_USUARIO"
     }
     ```

2. **Verificar huella**: `GET /verificar/?id=ID_DE_HUELLA`
   - Respuesta esperada:
     ```json
     {
       "access": true
     }
     ```

3. **Registrar log**: `POST /registerLog`
   - Payload:
     ```json
     {
       "door_id": "100",
       "fingerprint_id": "ID_DE_HUELLA",
       "action": "Door Opened"
     }
     ```

---

## Notas adicionales

- Asegúrate de que el lector de huellas esté correctamente conectado y configurado.
- Si el sistema no se conecta a WiFi, verifica las credenciales y la señal de tu red.
- El módulo de huellas soporta hasta 127 huellas registradas.
