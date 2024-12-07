const express = require('express');
const router = express.Router();
const admin = require('firebase-admin'); // Firebase Admin SDK should be initialized
const events = require('events');
const eventEmitter = new events.EventEmitter();


// Function to get all users from Firebase
const getAllUsers = async () => {
  try {
    const snapshot = await admin.database().ref('users').once('value');
    const users = [];

    snapshot.forEach((childSnapshot) => {
      const user = childSnapshot.val();
      users.push({
        username: childSnapshot.key, // Assuming the username is the key
        gateCode: user.gateCode,    // Adjust according to your database structure
        location: user.location
      });
    });
    //console.log('Retrieved users:', users); // Debug log
    return users;
  } catch (error) {
    console.error('Error fetching users from Firebase:', error);
    throw error; // Propagate the error for the caller to handle
  }
};

const getDeviceID = async (deviceType) => {
  try {
      // Fetch devices of the specified type from Firebase
      const snapshot = await admin.database().ref(`devices/${deviceType}`).once('value');
      const devices = snapshot.val() || [];

      // Create a unique new device ID
      const deviceCount = devices.length;
      const newDeviceID = `${deviceType}${deviceCount + 1}`;

      // Generate a random 1-byte address not already in use
      let randomAddress;
      const existingAddresses = new Set(
          Object.values(devices).map(device => device.address)
      );

      do {
        randomAddress = Buffer.from([Math.floor(Math.random() * 256)]); // Generate a single-byte buffer
      } while (existingAddresses.has(randomAddress[0]) || [0x32, 0x42].includes(randomAddress[0]));
      randomAddress = randomAddress.toString('hex');
      // Create the new device entry
      const newDevice = {
        deviceID: newDeviceID,
        device_type: deviceType,
        address: randomAddress, // Save as a hexadecimal string
      };   

      // Add the new device to the database
      await admin.database().ref(`devices/${deviceType}/${deviceCount}`).set(newDevice);

      console.log(`Added new device to database: ${JSON.stringify(newDevice)}`);
      return { newDeviceID, randomAddress };
  } catch (error) {
      console.error('Error fetching devices from Firebase:', error);
      throw error;
  }
};


// Endpoint to get table data
router.get('/get-table-data', async (req, res) => {
  try {
    const snapshot = await admin.database().ref('users').once('value');
    const data = [];

    snapshot.forEach((childSnapshot) => {
      const entry = childSnapshot.val();
      entry.username = childSnapshot.key; // Add username/key as part of the data if needed 
      data.push(entry);
    });

    // Send the data array as JSON
    res.json(data);
  } catch (error) {
    console.error('Error fetching table data from Firebase:', error);
    res.status(500).json({ message: 'Failed to fetch table data' });
  }
});

// SSE endpoint to send updates to the client
router.get('/userAccess-updates', (req, res) => {
  // Set headers for SSE
  res.setHeader('Content-Type', 'text/event-stream');
  res.setHeader('Cache-Control', 'no-cache');
  res.setHeader('Connection', 'keep-alive');

  console.log('Client connected for userAccess updates');

  // Function to send data to the client
  const sendUpdate = (data) => {
    res.write(`data: ${JSON.stringify(data)}\n\n`);
    console.log(`data: ${JSON.stringify(data)}\n\n`);
  };

  // Listen to Firebase database changes
  const userAccessRef = admin.database().ref('userAccess');
  const handleDatabaseChange = (snapshot) => {
    const data = [];
    snapshot.forEach((childSnapshot) => {
      const entry = childSnapshot.val();
      data.push(entry);
    });
    sendUpdate(data); // Send updated data to the client
  };

  // Attach the listener
  userAccessRef.on('value', handleDatabaseChange);

  // Handle client disconnect
  req.on('close', () => {
    console.log('Client disconnected from updates');
    userAccessRef.off('value', handleDatabaseChange); // Remove the database listener
    res.end();
  });
});

router.get('/get-userAccess-data', async (req, res) => {
  try {
    const snapshot = await admin.database().ref('userAccess').once('value');
    const data = [];

    snapshot.forEach((childSnapshot) => {
      const entry = childSnapshot.val();
      data.push(entry);
    });

    res.json(data); // Return the data as JSON
  } catch (error) {
    console.error('Error fetching userAccess data:', error);
    res.status(500).json({ message: 'Failed to fetch userAccess data' });
  }
});

// SSE endpoint to send updates to the client
router.get('/deviceStatus-updates', (req, res) => {
  // Set headers for SSE
  res.setHeader('Content-Type', 'text/event-stream');
  res.setHeader('Cache-Control', 'no-cache');
  res.setHeader('Connection', 'keep-alive');

  console.log('Client connected for deviceStatus updates');

  // Function to send data to the client
  const sendUpdate = (data) => {
    res.write(`data: ${JSON.stringify(data)}\n\n`);
    console.log(`data: ${JSON.stringify(data)}\n\n`);
  };

  // Listen to Firebase database changes
  const deviceStatusRef = admin.database().ref('deviceStatus');
  const handleDatabaseChange = (snapshot) => {
    const data = [];
    snapshot.forEach((childSnapshot) => {
      const entry = childSnapshot.val();
      data.push(entry);
    });
    sendUpdate(data); // Send updated data to the client
  };

  // Attach the listener
  deviceStatusRef.on('value', handleDatabaseChange);

  // Handle client disconnect
  req.on('close', () => {
    console.log('Client disconnected from updates');
    deviceStatusRef.off('value', handleDatabaseChange); // Remove the database listener
    res.end();
  });
});

router.get('/get-deviceStatus-data', async (req, res) => {
  try {
    const snapshot = await admin.database().ref('deviceStatus').once('value');
    const data = [];

    snapshot.forEach((childSnapshot) => {
      const entry = childSnapshot.val();
      data.push(entry);
    });

    res.json(data); // Return the data as JSON
  } catch (error) {
    console.error('Error fetching deviceStatus data:', error);
    res.status(500).json({ message: 'Failed to fetch deviceStatus data' });
  }
});

router.get('/get-user/:username', async (req, res) => {
  const { username } = req.params;

  try {
    const snapshot = await admin.database().ref(`users/${username}`).once('value');
    if (!snapshot.exists()) {
      return res.status(404).json({ message: 'User not found' });
    }

    const userData = snapshot.val();
    res.json({ username, ...userData });
  } catch (error) {
    console.error('Error fetching user data:', error);
    res.status(500).json({ message: 'Failed to fetch user data' });
  }
});

async function getLocation(gateID) {
  try {
      // Reference the 'devices/Gate' path in the database
      const gatesRef = admin.database().ref('devices/Gate');
      const snapshot = await gatesRef.once('value');

      if (snapshot.exists()) {
          const gates = snapshot.val();

          // Find the gate with the matching gateID
          const gate = gates.find(g => g.deviceID === gateID);

          if (gate && gate.location) {
              return gate.location; // Return the location if found
          }
      }

      console.warn(`Device with gateID=${gateID} not found.`);
      return null;
  } catch (error) {
      console.error('Error fetching location:', error);
      throw error;
  }
}

async function getGateType(gateID) {
  try {
      // Reference the 'devices/Gate' path in the database
      const gatesRef = admin.database().ref('devices/Gate');
      const snapshot = await gatesRef.once('value');

      if (snapshot.exists()) {
          const gates = snapshot.val();

          // Find the gate with the matching gateID
          const gate = gates.find(g => g.deviceID === gateID);

          if (gate && gate.gateType) {
              return gate.gateType; // Return the location if found
          }
      }

      console.warn(`Device with gateID=${gateID} not found.`);
      return null;
  } catch (error) {
      console.error('Error fetching gate type:', error);
      throw error;
  }
}

async function getBuilding(gateID) {
  try {
      // Reference the 'devices/Gate' path in the database
      const gatesRef = admin.database().ref('devices/Gate');
      const snapshot = await gatesRef.once('value');

      if (snapshot.exists()) {
          const gates = snapshot.val();

          // Find the gate with the matching gateID
          const gate = gates.find(g => g.deviceID === gateID);

          if (gate && gate.building) {
            console.log(`Building = ${gate.building}`);
            return gate.building; // Return the location if found
          }
      }

      console.warn(`Device with gateID=${gateID} not found.`);
      return null;
  } catch (error) {
      console.error('Error fetching building:', error);
      throw error;
  }
}


async function getDeviceIDsByLocation(location) {
  try {
    if (!location) {
      console.error('Invalid location:', location);
      return null;
    }
  
    const ref = admin.database().ref('devices');
    const snapshot = await ref.once('value');
    const devices = snapshot.val();
    
    const deviceIDs = [];

    for (const category of Object.keys(devices)) {
      const deviceArray = devices[category];
      for (const device of deviceArray) {
        if (device.location === location) {
          console.log(`Found deviceID for location ${device.deviceID}: ${location}`);
          deviceIDs.push(device.deviceID);
        }
      }
    }
    return deviceIDs;
  } catch (error) {
      console.error('Error fetching deviceIDs from location:', error);
      throw error;
  }
}


// Export both the router and the getAllUsers function
module.exports = { router, getAllUsers, getDeviceID, getLocation, getGateType,getBuilding,getDeviceIDsByLocation };
