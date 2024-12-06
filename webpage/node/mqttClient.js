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

let currentLeader = null; // Holds the current leader

// Function to elect a leader
async function electLeader() {
  const deviceStatusRef = admin.database().ref('deviceStatus');
  try {
    const deviceStatusSnapshot = await deviceStatusRef.once('value');
    const deviceStatus = deviceStatusSnapshot.val();

    if (!deviceStatus) {
      console.log('No devices found in database.');
      currentLeader = null;
      return;
    }

    // Filter Alarm devices online
    const alarmDevices = Object.values(deviceStatus).filter(
      (device) => device.deviceID.startsWith('Alarm') && device.status === 'online'
    );

    // If no Alarm devices are online, filter Gate devices online
    let leaderCandidates = alarmDevices;
    if (alarmDevices.length === 0) {
      leaderCandidates = Object.values(deviceStatus).filter(
        (device) => device.deviceID.startsWith('Gate') && device.status === 'online'
      );
    }

    // Randomly select a leader from available candidates
    if (leaderCandidates.length > 0) {
      const randomIndex = Math.floor(Math.random() * leaderCandidates.length);
      currentLeader = leaderCandidates[randomIndex];

      // Extract the deviceID for publishing
      const leaderDeviceID = currentLeader.deviceID;
      
      mqttClient.publish(
        "ELEC520/forwarder",
        leaderDeviceID, // Pass the deviceID as the payload
        {
          qos: 0, // Change to 2 for QoS 2 if needed
          retain: true, // Retain the message
        },
        (err) => {
          if (err) {
            console.error(`Failed to publish to topic "ELEC520/forwarder":`, err);
          } else {
            console.log(`Successfully published to topic "ELEC520/forwarder" with QoS 0 and retained: "${leaderDeviceID}"`);
          }
        }
      );
      
      console.log(`New leader elected: ${currentLeader.deviceID}`);
    } else {
      currentLeader = null;
      console.log('No available devices to elect as leader.');
    }
  } catch (error) {
    console.error('Error electing leader:', error);
  }
}

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
  mqttClient.subscribe('ELEC520/users/admin/view', (err) => {
    if (err) {
      console.error('Failed to subscribe to ELEC520/users/admin/view:', err);
    } else {
      console.log('Subscribed to ELEC520/users/admin/view');
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
  mqttClient.subscribe("ELEC520/users/admin/needUpdate", (err) => {
    if (err) {
      console.error("Failed to subscribe to topic:", err);
    } else {
      console.log("Subscribed to ELEC520/users/admin/needUpdate");
    }
  });
  mqttClient.subscribe("ELEC520/users/needUpdate", (err) => {
    if (err) {
      console.error("Failed to subscribe to topic:", err);
    } else {
      console.log("Subscribed to ELEC520/users/needUpdate");
    }
  });
  //electLeader();
});

// Handle incoming messages
mqttClient.on('message', async (topic, message) => {
    console.log('Received mqtt messaged');
    if (topic === 'ELEC520/users/view') {
      try {
        // Extract the deviceID from the payload
        const deviceID = message.toString().trim();
    
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
    
        // Retrieve the latest usersUpdated timestamp

        // Update users and publish to relevant topics
        const devicesRef = admin.database().ref("devices");
        const devicesSnapshot = await devicesRef.once("value");
        const devicesData = devicesSnapshot.val();
    
        const usersLastUpdatedRef = admin.database().ref("users/lastUpdated");
    
        // Retrieve the users last updated timestamp
        const usersLastUpdatedSnapshot = await usersLastUpdatedRef.once("value");
        const usersLastUpdated = usersLastUpdatedSnapshot.val()?.lastUpdated;
    
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
    
            // Update the `usersUpdated` key for the device
            let deviceUpdated = false;
    
            for (const [type, deviceList] of Object.entries(devicesData || {})) {
              if (Array.isArray(deviceList)) {
                for (let device of deviceList) {
                  if (device.deviceID === id) {
                    device.usersUpdated = usersLastUpdated;
                    deviceUpdated = true;
                    break;
                  }
                }
              }
              if (deviceUpdated) break;
            }
    
            if (deviceUpdated) {
              console.log(`Updated usersUpdated for device ${id} to ${usersLastUpdated}`);
            } else {
              console.log(`Device ${id} not found in devices table.`);
            }
    
            // Add a delay to prevent flooding
            // await delay(100);
          }
        }
    
        // Commit the updates to the database
        await devicesRef.set(devicesData);
        console.log('Updated devices in the database with new usersUpdated timestamps.');
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
        mqttClient.publish('ELEC520/devices/update',newDeviceID, (err) => {
          if (err) {
            console.error(`Failed to publish to topic ${topic}:`, err);
          } else {
            console.log(`Successfully published to topic ${topic} with QoS 1: "${newDeviceID}"`);
          }
        });
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

          // If currentLeader is null and a device comes online, elect a leader
      if (!currentLeader && currentStatus === 'online') {
        console.log(`Device ${deviceID} is online and no leader exists. Electing a leader...`);
        await electLeader();
      }

      // If the current leader goes offline, elect a new leader
      if (currentLeader && currentLeader.deviceID === deviceID && currentStatus === 'offline') {
        console.log(`Leader ${deviceID} went offline. Electing a new leader...`);
        await electLeader();
      }
  
      //console.log(`Received access attempt: device ID=${deviceID}, Status=${currentStatus}`);
  
      try {
        // Get the current date and time
        const now = new Date();
        const timeaccess = now.toISOString(); // Format as ISO string
    
        // Reference the deviceStatus path in the database
        const deviceStatusRef = admin.database().ref('deviceStatus');
        const snapshot = await deviceStatusRef.once('value');
        const deviceStatusData = snapshot.val() || {}; // Default to an empty object if no data exists
    
        // Reference the devices table to check if the deviceID exists
        const devicesRef = admin.database().ref('devices');
        const devicesSnapshot = await devicesRef.once('value');
        const devices = devicesSnapshot.val() || {};
    
        // Check if the deviceID exists in the devices table
        const deviceExists = Object.keys(devices).some(deviceType => {
          const deviceList = devices[deviceType]; // Get the array of devices for this type
          return Array.isArray(deviceList) && deviceList.some(device => device.deviceID === deviceID);
        });

    
        if (!deviceExists) {
            console.warn(`Device ID ${deviceID} not found in devices table. Ignoring ping.`);
            return; // Exit the function
        }
    
        // Update or create the deviceStatus entry
        if (deviceStatusData[deviceID]) {
            // Update existing entry
            deviceStatusData[deviceID].deviceID = deviceID;
            deviceStatusData[deviceID].lastPing = timeaccess;
            deviceStatusData[deviceID].status = currentStatus;
        } else {
            // Create a new entry
            deviceStatusData[deviceID] = {
                deviceID: deviceID,
                lastPing: timeaccess,
                status: currentStatus,
            };
        }
    
        // Write the updated object back to the database
        await deviceStatusRef.set(deviceStatusData);
    
        // Notify about the update
        notifyDeviceStatusUpdate();
      } catch (error) {
        console.error('Error processing ELEC520/deviceStatus:', error);
      }
    }  
    
    
    if (topic === "ELEC520/users/admin/needUpdate" || topic === "ELEC520/users/needUpdate") {
      const deviceID = message.toString().trim();
      console.log(`Received deviceID: ${deviceID}`);
    
      try {
        // Determine the target topic based on the input topic
        const targetTopic =
          topic === "ELEC520/users/admin/needUpdate"
            ? "ELEC520/users/admin/updateNeeded"
            : "ELEC520/users/updateNeeded";
    
        // Get the database references
        const devicesRef = admin.database().ref("devices");
        const usersLastUpdatedRef = admin.database().ref("users/lastUpdated");
    
        // Retrieve the users last updated timestamp
        const usersLastUpdatedSnapshot = await usersLastUpdatedRef.once("value");
        const usersLastUpdated = usersLastUpdatedSnapshot.val()?.lastUpdated;
    
        // Retrieve the device's usersUpdated timestamp
        const devicesSnapshot = await devicesRef.once("value");
        const devicesData = devicesSnapshot.val();
    
        let deviceUsersUpdated = null;
        // Search for the deviceID in the nested devices table
        for (const [type, deviceList] of Object.entries(devicesData || {})) {
          if (Array.isArray(deviceList)) {
            const device = deviceList.find((d) => d.deviceID === deviceID);
            if (device) {
              deviceUsersUpdated = device.usersUpdated;
              break;
            }
          }
        }
    
        if (deviceUsersUpdated) {
          console.log(`Device usersUpdated: ${deviceUsersUpdated}, Users lastUpdated: ${usersLastUpdated}`);
    
          const payload = deviceUsersUpdated === usersLastUpdated ? "0" : "1";
          mqttClient.publish(targetTopic, payload, (err) => {
            if (err) {
              console.error(`Failed to publish updateNeeded message to ${targetTopic}:`, err);
            } else {
              console.log(`Published updateNeeded message with payload: ${payload} to ${targetTopic}`);
            }
          });
        } else {
          console.error("Device not found or usersUpdated key missing");
        }
      } catch (error) {
        console.error("Error handling message:", error);
      }
    }
    
    if (topic === "ELEC520/users/admin/view") {
      const deviceID = message.toString().trim();
      console.log("Received request to view admin users");
    
      try {
        // Reference the 'users' section in the Firebase Realtime Database
        const usersRef = admin.database().ref("users");
        const snapshot = await usersRef.once("value");
    
        // Retrieve the data from the snapshot
        const usersData = snapshot.val();
        if (!usersData) {
          console.error("No users found in the database.");
          return;
        }
    
        // Get the lastUpdated timestamp
        const lastUpdated = usersData.lastUpdated?.lastUpdated;
        if (!lastUpdated) {
          console.error("No lastUpdated key found in the database.");
          return;
        }
    
        // Filter users with the 'admin' permission
        const adminUsers = Object.entries(usersData)
          .filter(([_, user]) => user.permissions === "admin")
          .map(([key, user]) => ({
            username: user.username,
            gateCode: user.gateCode,
          }));
    
        if (adminUsers.length === 0) {
          console.log("No admin users found.");
          return;
        }
    
        // Publish each admin user's username and gate code
        for (const adminUser of adminUsers) {
          const payload = `${adminUser.username}:${adminUser.gateCode}`;
          mqttClient.publish(
            `ELEC520/users/admin/update/${deviceID}`,
            payload,
            { qos: 0 },
            (err) => {
              if (err) {
                console.error(`Failed to publish admin user: ${payload}`, err);
              } else {
                console.log(`Published admin user: ${payload}`);
              }
            }
          );
        }
    
        // Update the `usersUpdated` value in the devices table for the given deviceID
        const devicesRef = admin.database().ref("devices");
        const devicesSnapshot = await devicesRef.once("value");
        const devicesData = devicesSnapshot.val();
    
        let deviceUpdated = false;
    
        // Search for the deviceID and update its `usersUpdated` field
        for (const [type, deviceList] of Object.entries(devicesData || {})) {
          if (Array.isArray(deviceList)) {
            for (let device of deviceList) {
              if (device.deviceID === deviceID) {
                device.usersUpdated = lastUpdated;
                deviceUpdated = true;
                break;
              }
            }
          }
          if (deviceUpdated) break;
        }
    
        if (deviceUpdated) {
          await devicesRef.set(devicesData);
          console.log(`Updated usersUpdated for device ${deviceID} to ${lastUpdated}`);
        } else {
          console.log(`Device ${deviceID} not found in devices table.`);
        }
      } catch (error) {
        console.error("Error processing admin view request:", error);
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
