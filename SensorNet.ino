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
  String name;
};

bool writePrefString(String key, String value, bool mount = true);

bool fsErr = false;

Scheduler userScheduler; // to control your personal task
Prefs prefs;
painlessMesh  mesh;

// User stub
void sendMessage() ; // Prototype so PlatformIO doesn't complain

Task taskSendMessage( TASK_MILLISECOND * 500 , TASK_FOREVER, &sendMessage );

long overseerId = -1;

/*
 * Commands:
 * O:... - Set overseer id
 * N:... - Set name
 * 
 * Reports:
 * D:... - Data
 * B:... - Button
 * E:... - Error
 */

void sendReport(String msg) {
  if (overseerId == -1) {
    mesh.sendBroadcast(msg);
  } else {
    mesh.sendSingle((uint32_t)overseerId, msg);
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

// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
  switch (msg[0]) {
    case 'O': // Overseer id
      overseerId = msg.substring(2).toInt();
      break;
    case 'N': // Set name
      prefs.name = msg.substring(2);
      writePrefString("name", prefs.name);
      break;
  }
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

String readPrefString(String key, String def, bool mount = true) {
  if (mount) {
    if (!SPIFFS.begin()) {
      Serial.println("An error has occurred while mounting SPIFFS");
      fsErr = true;
      return ""; // ...I guess empty strings just aren't allowed for values :/
    } else {
      Serial.println("Successfully mounted SPIFFS");
    }
  }

  String path = "/prefs/"+key;

  if (!SPIFFS.exists(path)) {
    if (writePrefString(key, def, mount)) {
      return def;
    } else {
      return "";
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
    return "";
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
  //readPref("", "UNNAMED", false);

  SPIFFS.end();
  return true;
}

void setup() {
  Serial.begin(115200);

  loadPrefs(); // Should handle failure?
  
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
  
  // it will run the user scheduler as well
  mesh.update();
}
