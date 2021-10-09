/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp-mesh-esp32-esp8266-painlessmesh/
  
  This is a simple example that uses the painlessMesh library: https://github.com/gmag11/painlessMesh/blob/master/examples/basic/basic.ino
*/

#include "painlessMesh.h"
#include <FS.h>

#define   MESH_PREFIX     "whateverYouLike"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

bool fsErr = false;

Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;

// User stub
void sendMessage() ; // Prototype so PlatformIO doesn't complain

Task taskSendMessage( TASK_MILLISECOND * 100 , TASK_FOREVER, &sendMessage );

void sendMessage() {
  if (fsErr) {
    String msg = "FILE ERR";
    mesh.sendBroadcast( msg );
  }
  
  String msg = "Hi from node ";
  msg += mesh.getNodeId();
  mesh.sendBroadcast( msg );
  //taskSendMessage.setInterval( random( TASK_SECOND * 1, TASK_SECOND * 5 ));
}

// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
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

void setup() {
  Serial.begin(115200);

  if (!SPIFFS.begin()) {
    Serial.println("An error has occurred while mounting SPIFFS");
    fsErr = true;
  } else {
    Serial.println("Successfully mounted SPIFFS");
  }

  if (SPIFFS.exists("/name")) {
    File f = SPIFFS.open("/name","r");
    if (f) {
      Serial.print("Opened /name: ");
      while(f.available()) {
        //indexHTML = f.readString(); //<= DOES NOT WORK!!!
        //HTTP.write(f.read()); //<= DOES NOT WORK!!!
        Serial.write(f.read()); //<= DISPLAYS UNLIMITED "?" until it crashes
        //indexHTML += f.read(); //<= DOES NOT WORK!!!
      }
      f.close();
    } else {
      Serial.println("Failed to open /name");
    }
  } else {
    File f = SPIFFS.open("/name","w");
    if (f) {
      f.print("Hyperdrive watts");
      f.close();
      Serial.println("Created /name");
    } else {
      Serial.println("Failed to create /name");
    }
  }
  SPIFFS.end();
  
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

void loop() {
  // it will run the user scheduler as well
  mesh.update();
}
