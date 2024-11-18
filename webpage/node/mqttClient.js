const mqtt = require('mqtt');
const { getAllUsers, getDeviceID } = require('./routes/getTableData'); // Assuming a database module exists

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
  mqttClient.subscribe('ELEC520/users/viewall', (err) => {
    if (err) {
      console.error('Failed to subscribe to ELEC520/users/viewall:', err);
    } else {
      console.log('Subscribed to ELEC520/users/viewall');
    }
  });
  mqttClient.subscribe('ELEC520/devices/view', (err) => {
    if (err) {
      console.error('Failed to subscribe to ELEC520/devices/view:', err);
    } else {
      console.log('Subscribed to ELEC520/devices/view');
    }
  });
});

// Handle incoming messages
mqttClient.on('message', async (topic, message) => {
    console.log('Received mqtt messaged');
    if (topic === 'ELEC520/users/viewall') {
      console.log('Received request to view all users');
  
      try {
        // Retrieve all users from the database
        const users = await getAllUsers();
  
        // Publish each user's credentials independently
        users.forEach((user) => {
          const payload = `${user.username}:${user.password}`;
          mqttClient.publish('ELEC520/users/updateall', payload, (err) => {
            if (err) {
              console.error('Failed to publish user credentials:', err);
            } else {
              console.log('Published user credentials:', payload);
            }
          });
          delay(100);
        });
      } catch (error) {
        console.error('Error retrieving users from database:', error);
      }
    }

    if (topic === 'ELEC520/devices/view') {
        const deviceType = message.toString().trim(); // Extract the device type from the payload
        console.log(`Received device type: ${deviceType}`);
    
        try {
          const newDeviceID = await getDeviceID(deviceType);
          // Publish the new device ID to the 'ELEC520/devices/update' topic
          mqttClient.publish('ELEC520/devices/update', newDeviceID, (err) => {
            if (err) {
              console.error('Failed to publish new device ID:', err);
            } else {
              console.log(`Published new device ID: ${newDeviceID}`);
            }
          });
        } catch (error) {
          console.error('Error processing ELEC520/devices/view message:', error);
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
