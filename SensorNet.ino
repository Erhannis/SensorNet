/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp-mesh-esp32-esp8266-painlessmesh/
  
  This is a simple example that uses the painlessMesh library: https://github.com/gmag11/painlessMesh/blob/master/examples/basic/basic.ino
*/

#include "painlessMesh.h"
#include <FS.h>

#define   MESH_PREFIX     "s-net"
#define   MESH_PASSWORD   "8eae48cacee812d9"
#define   MESH_PORT       5555

class Prefs {
  public:
  long overseerId;
  String name;
  int sensor; // -1 disabled, -2 analog, -3 I2C, else GPIO pin
  int interval; // milliseconds
};

bool writePrefString(String key, String value, bool mount = true);
bool writePrefInt(String key, int value, bool mount = true);

bool fsErr = false;
long nextTime = 0;

Scheduler userScheduler; // to control your personal task
Prefs prefs;
painlessMesh  mesh;

// User stub
void sendMessage() ; // Prototype so PlatformIO doesn't complain

Task taskSendMessage( TASK_MILLISECOND * 500 , TASK_FOREVER, &sendMessage );

void sendReport(String msg) {
  if (prefs.overseerId == -1) {
    mesh.sendBroadcast(msg);
  } else {
    mesh.sendSingle((uint32_t)prefs.overseerId, msg);
  }
}

void sendMessage() {
  if (fsErr) {
    String msg = "E:FILE ERR";
    mesh.sendBroadcast( msg );
  }
  
  String msg = "MY NAME, IS, " + prefs.name;
  mesh.sendBroadcast( msg );
  //taskSendMessage.setInterval( random( TASK_SECOND * 1, TASK_SECOND * 5 ));
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}

bool writePrefString(String key, String value, bool mount) {
  if (mount) {
    if (!SPIFFS.begin()) {
      Serial.println("An error has occurred while mounting SPIFFS");
      fsErr = true;
      return false;
    } else {
      Serial.println("Successfully mounted SPIFFS");
    }
  }

  String path = "/prefs/"+key;
  
  File f = SPIFFS.open(path,"w");
  if (f) {
    f.print(value);
    f.close();
    Serial.println("Created "+path);
  } else {
    Serial.println("Failed to create "+path);
    return false;
  }

  if (mount) {
    SPIFFS.end();
  }
  return true;
}

bool writePrefInt(String key, int value, bool mount) {
  if (mount) {
    if (!SPIFFS.begin()) {
      Serial.println("An error has occurred while mounting SPIFFS");
      fsErr = true;
      return false;
    } else {
      Serial.println("Successfully mounted SPIFFS");
    }
  }

  String path = "/prefs/"+key;
  
  File f = SPIFFS.open(path,"w");
  if (f) {
    f.print(String(value));
    f.close();
    Serial.println("Created "+path);
  } else {
    Serial.println("Failed to create "+path);
    return false;
  }

  if (mount) {
    SPIFFS.end();
  }
  return true;
}

String readPrefString(String key, String def, bool mount = true) {
  if (mount) {
    if (!SPIFFS.begin()) {
      Serial.println("An error has occurred while mounting SPIFFS");
      fsErr = true;
      return ""; //TODO Return def?  // ...I guess empty strings just aren't allowed for values :/
    } else {
      Serial.println("Successfully mounted SPIFFS");
    }
  }

  String path = "/prefs/"+key;

  if (!SPIFFS.exists(path)) {
    if (writePrefString(key, def, mount)) {
      return def;
    } else {
      return ""; //TODO Return def?
    }
  }

  String result = "";
  
  File f = SPIFFS.open(path,"r");
  if (f) {
    Serial.print("Opened "+path+": ");
    result = f.readString();
    Serial.print(result);
    f.close();
  } else {
    Serial.println("Failed to open " + path);
    return ""; //TODO Return def?
  }
 
  if (mount) {
    SPIFFS.end();
  }
  return result;
}

int readPrefInt(String key, int def, bool mount = true) {
  if (mount) {
    if (!SPIFFS.begin()) {
      Serial.println("An error has occurred while mounting SPIFFS");
      fsErr = true;
      return -1; //TODO Return default?
    } else {
      Serial.println("Successfully mounted SPIFFS");
    }
  }

  String path = "/prefs/"+key;

  if (!SPIFFS.exists(path)) {
    if (writePrefInt(key, def, mount)) { //TODO Write as binary?
      return def;
    } else {
      return -1; //TODO Return def?
    }
  }

  int result = -1;
  
  File f = SPIFFS.open(path,"r");
  if (f) {
    Serial.print("Opened "+path+": ");
    result = f.readString().toInt();
    Serial.print(result);
    f.close();
  } else {
    Serial.println("Failed to open " + path);
    return -1; //TODO Return def?
  }
 
  if (mount) {
    SPIFFS.end();
  }
  return result;
}

bool loadPrefs() {
  if (!SPIFFS.begin()) {
    Serial.println("An error has occurred while mounting SPIFFS");
    fsErr = true;
    return false;
  } else {
    Serial.println("Successfully mounted SPIFFS");
  }

  prefs.name = readPrefString("name", "UNNAMED", false);
  prefs.overseerId = readPrefInt("overseer", -1, false);
  prefs.sensor = readPrefInt("sensor", 5, false);
  prefs.interval = readPrefInt("interval", 1000, false);
  //readPref("", "UNNAMED", false);

  SPIFFS.end();
  return true;
}

/*
 * Commands:
 * O:... - Set overseer id
 * N:... - Set name
 X L:(X|I|A|#) - Enable sensor ('listen'):
 *   X - disable
 X   I - I2C
 *   A - Analog input
 *   # - GPIO pin
 * I:... - Set interval
 * Q:(O|N|L|I) - Request setting
 X W - Wipe flash
 * 
 * Reports:
 * D:... - Data
 * B:... - Button
 * E:... - Error
 * R:... - Request response
 */


// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
  switch (msg[0]) {
    case 'O': // Overseer id
      prefs.overseerId = msg.substring(2).toInt();
      writePrefInt("overseer", prefs.overseerId);
      break;
    case 'N': // Set name
      prefs.name = msg.substring(2);
      writePrefString("name", prefs.name);
      break;
    case 'L': // Set sensor
      switch (msg[2]) {
        case 'X': // Disable
          prefs.sensor = -1;
          break;
        case 'I': // I2C
          prefs.sensor = -3;
          break;
        case 'A': // Analog
          prefs.sensor = -2;
          break;
        default: // GPIO pin
          prefs.sensor = msg.substring(2).toInt();
          pinMode(prefs.sensor, INPUT); // Pull low?
          break;
      }
      writePrefInt("sensor", prefs.sensor);
      break;
    case 'I': { // Set interval
      long t = nextTime - prefs.interval;
      prefs.interval = msg.substring(2).toInt();
      nextTime = t + prefs.interval;
      writePrefInt("interval", prefs.interval);
      break;}
    case 'Q': { // Request setting
      String s = "R:";
      switch (msg[2]) {
        case 'O':
          sendReport(s + prefs.overseerId);
          break;
        case 'N':
          sendReport(s + prefs.name);
          break;
        case 'L':
          switch (prefs.sensor) {
            case -1: // Disabled
              sendReport("R:X");
              break;
            case -2: // Analog
              sendReport("R:A");
              break;
            case -3: // I2C
              sendReport("R:I");
              break;
            default: // GPIO
              sendReport(s + prefs.sensor);
              break;
          }
          break;
        case 'I':
          sendReport(s + prefs.interval);
          break;
      }
      break;}
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(0, INPUT); // Pull low?
  loadPrefs(); // Should handle failure?
  if (prefs.sensor >= 0) {
    pinMode(prefs.sensor, INPUT); // Pull low?
  }
  
  mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
//mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask( taskSendMessage );
  taskSendMessage.enable();
}

int lastBtn = 1;
//int lastValue = -1;
void loop() {
  int curBtn = digitalRead(0);
  if (lastBtn != curBtn) {
    if (curBtn) {
      sendReport("B:0");
    } else {
      sendReport("B:1");
    }
  }
  lastBtn = curBtn;

  long t = millis();
  if (t >= nextTime) {
    String s;
    int val;
    switch (prefs.sensor) {
      case -1: // Disabled
        break;
      case -2: // Analog
        s = "D:";
        val = analogRead(A0);
        sendReport(s + val);
        break;
      case -3: // I2C
        //TODO Do
        sendReport("E:I2C not yet implemented");
        break;
      default: // GPIO
        val =  digitalRead(prefs.sensor);
        //TODO If change?
        if (val) {
          sendReport("D:1");
        } else {
          sendReport("D:0");
        }
        break;
    }
    nextTime = t + prefs.interval;
  }
  
  // it will run the user scheduler as well
  mesh.update();
}
