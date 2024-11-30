const mqtt = require('mqtt');
const { getAllUsers, getDeviceID, getLocation, getGateType, getBuilding,getDeviceIDsByLocation } = require('./routes/getTableData'); // Assuming a database module exists
const admin = require('firebase-admin'); // Firebase Admin SDK should be initialized
const eventEmitter = require('./eventEmitter');

function notifyUserAccessTableUpdate() {
  console.log('Notify userAccess table update');
  eventEmitter.emit('userAccess_table_update', { message: 'Table data updated' });
}

function notifyDeviceStatusUpdate() {
  console.log('Notify deviceStatus table update');
  eventEmitter.emit('deviceStatus_table_update', { message: 'Table data updated' });
}

function delay(time) {
    return new Promise(resolve => setTimeout(resolve, time));
}

// Configure the MQTT broker address and options
const MQTT_BROKER_URL = 'mqtts://40c06ef97ec5427eb54aa49e5c03c12c.s1.eu.hivemq.cloud:8883'; // Replace with your broker URL
const MQTT_OPTIONS = {
    username: 'database',
    password: 'Databasepass1'
}; // Add additional options if needed

// Create and connect the MQTT client
const mqttClient = mqtt.connect(MQTT_BROKER_URL, MQTT_OPTIONS);

// Handle connection events
mqttClient.on('connect', () => {
  console.log('Connected to MQTT broker');
  mqttClient.subscribe('ELEC520/users/view', (err) => {
    if (err) {
      console.error('Failed to subscribe to ELEC520/users/view:', err);
    } else {
      console.log('Subscribed to ELEC520/users/view');
    }
  });
  mqttClient.subscribe('ELEC520/devices/view', (err) => {
    if (err) {
      console.error('Failed to subscribe to ELEC520/devices/view:', err);
    } else {
      console.log('Subscribed to ELEC520/devices/view');
    }
  });
  mqttClient.subscribe('ELEC520/userAccess', (err) => {
    if (err) {
      console.error('Failed to subscribe to ELEC520/userAccess:', err);
    } else {
      console.log('Subscribed to ELEC520/userAccess');
    }
  });
  mqttClient.subscribe('ELEC520/devicePing', (err) => {
    if (err) {
      console.error('Failed to subscribe to ELEC520/devicePing:', err);
    } else {
      console.log('Subscribed to ELEC520/devicePing');
    }
  });
});

// Handle incoming messages
mqttClient.on('message', async (topic, message) => {
    console.log('Received mqtt messaged');
    if (topic === 'ELEC520/users/view') {
      try {
        // Extract the deviceID from the payload
        const deviceID = message.toString().trim();
        //console.log(`Raw message received: "${message}"`);
        //console.log(`Extracted deviceID: "${deviceID}"`);
    
        if (!deviceID) {
          console.error("No deviceID provided in the MQTT message payload.");
          return;
        }
    
        console.log('Received request to view all users');
    
        // Retrieve all users from the database
        const users = await getAllUsers();
        console.log('Retrieved users:', users); // Debug log
        if (!users || users.length === 0) {
          console.error("No users found in the database.");
          return;
        }
    
        // Retrieve the location for the device
        const location = await getLocation(deviceID);
        if (!location) {
          console.error(`No location found for deviceID: ${deviceID}`);
          return;
        }
    
        console.log(`Found location for deviceID ${deviceID}: ${location}`);
    
        // Get all devices associated with the location
        let deviceIDs = await getDeviceIDsByLocation(location);
    
        if (!deviceIDs) {
          console.error(`No devices found for location: ${location}`);
          return;
        }
    
        // Ensure deviceIDs is an array
        if (!Array.isArray(deviceIDs)) {
          deviceIDs = [deviceIDs];
        }
    
        if (deviceIDs.length === 0) {
          console.error(`No devices to publish to for location: ${location}`);
          return;
        }
    
        console.log(`Found devices for location ${location}: ${deviceIDs}`);
    
        // Process each user and publish to relevant topics
        for (const user of users) {
          const payload = `${user.username}:${user.gateCode}`;
    
          for (const id of deviceIDs) {
            const topic = `ELEC520/users/update/${id}`;
            console.log(`Publishing to topic: ${topic} with payload: "${payload}"`);
    
            mqttClient.publish(
              topic,
              payload,
              { qos: 0 }, // Change to 2 for QoS 2
              (err) => {
                if (err) {
                  console.error(`Failed to publish to topic ${topic}:`, err);
                } else {
                  console.log(`Successfully published to topic ${topic} with QoS 1: "${payload}"`);
                }
              }
            );
    
            // Add a delay to prevent flooding
            await delay(100);
          }
        }
      } catch (error) {
        console.error('Error processing request:', error);
      }
    }
    

    if (topic === 'ELEC520/devices/view') {
      const deviceType = message.toString().trim(); // Extract the device type from the payload
      console.log(`Received device type: ${deviceType}`);

      try {
        const newDeviceID = await getDeviceID(deviceType);
        // Publish the new device ID to the 'ELEC520/devices/update' topic
        mqttClient.publish(
          'ELEC520/devices/update',
          newDeviceID,
          { qos: 0 }, // Change to 2 for QoS 2
          (err) => {
            if (err) {
              console.error(`Failed to publish to topic ${topic}:`, err);
            } else {
              console.log(`Successfully published to topic ${topic} with QoS 1: "${payload}"`);
            }
          }
        );
      } catch (error) {
        console.error('Error processing ELEC520/devices/view message:', error);
      }
    }
    if (topic === 'ELEC520/userAccess') {
      const payload = message.toString().trim();
  
      const [username, gateID] = payload.split(' ');
  
      if (!username || !gateID) {
          console.error('Invalid message format. Expected "username gateID".');
          return;
      }
  
      try {
          const locationAccessed = await getLocation(gateID);
          const gateType = await getGateType(gateID);
          const building = await getBuilding(gateID);
  
          if (!locationAccessed || !gateType || !building) {
              console.error(`Invalid gateID or missing data for: ${gateID}`);
              return;
          }
  
          const now = new Date();
          const timeaccess = now.toISOString();
  
          const userAccessRef = admin.database().ref('userAccess');
          const snapshot = await userAccessRef.once('value');
          const userAccessData = snapshot.val() || {};
  
          let userFound = false;
  
          // Iterate through the keys of the object
          for (const [key, value] of Object.entries(userAccessData)) {
              if (value.user === username) {
                  userFound = true;
                  
                  if (gateType === 'room' && value.location === locationAccessed) {
                      console.log(`User ${username} accessed room ${locationAccessed} again. Removing location.`);
                      value.location = building;
                      break;
                  }
  
                  if (gateType === 'entrance' && ((value.location === locationAccessed)||(value.building === building))) {
                      console.log(`User ${username} exited the building via ${locationAccessed}. Removing user.`);
                      delete userAccessData[key]; // Remove user
                      break;
                  }
  
                  value.location = locationAccessed;
                  value.timeAccessed = timeaccess;
                  value.building = building;
                  break;
              }
          }
  
          // If no existing entry was found, create a new one
          if (!userFound) {
              const newKey = userAccessRef.push().key; // Generate a new unique key
              userAccessData[newKey] = {
                  user: username,
                  location: locationAccessed,
                  timeAccessed: timeaccess,
                  building: building,
              };
          }
  
          await userAccessRef.set(userAccessData);
  
          console.log(`Access logged: User=${username}, Location=${locationAccessed}, Time=${timeaccess}, Building=${building}`);
          notifyUserAccessTableUpdate();
      } catch (error) {
          console.error('Error processing ELEC520/userAccess:', error);
      }
  }
  
        
    if (topic === 'ELEC520/devicePing') {
      const payload = message.toString().trim();
  
      // Split the message into username and gateID
      const [deviceID, currentStatus] = payload.split(' ');
  
      if (!deviceID || !currentStatus) {
          console.error('Invalid message format. Expected "deviceID status".');
          return;
      }
  
      //console.log(`Received access attempt: device ID=${deviceID}, Status=${currentStatus}`);
  
      try {
          // Get the current date and time
          const now = new Date();
          const timeaccess = now.toISOString(); // Format as ISO string
  
          // Update the database with the access attempt details
          const deviceStatusRef = admin.database().ref('deviceStatus');
          const snapshot = await deviceStatusRef.once('value');
          const deviceStatusData = snapshot.val() || []; // Default to an empty array if no data exists

          let deviceFound = false;

          // Iterate through userAccess array to find an existing entry for the user
          for (let i = 0; i < deviceStatusData.length; i++) {
              if (deviceStatusData[i].deviceID === deviceID) {
                  // Update the existing entry
                  deviceStatusData[i].lastPing = timeaccess;
                  deviceStatusData[i].status = currentStatus;
                  deviceFound = true;
                  break;
              }
          }

          // If no existing entry was found, create a new one
          if (!deviceFound) {
            deviceStatusData.push({
              deviceID: deviceID,
              lastPing: timeaccess,
              status: currentStatus,
            });
          }

          // Write the updated array back to the database
          await deviceStatusRef.set(deviceStatusData);

        //  console.log(`Access logged: device=${deviceID}, Status=${currentStatus}, Time=${timeaccess}`);
          notifyDeviceStatusUpdate();
      } catch (error) {
          console.error('Error processing ELEC520/deviceStatus:', error);
      }
  } 

});

mqttClient.on('error', (err) => {
  console.error('MQTT connection error:', err);
});

mqttClient.on('close', () => {
  console.log('Disconnected from MQTT broker');
});

// Export the client to be used in other files
module.exports = mqttClient;
