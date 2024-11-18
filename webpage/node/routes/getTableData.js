const express = require('express');
const router = express.Router();
const admin = require('firebase-admin'); // Firebase Admin SDK should be initialized

// Function to get all users from Firebase
const getAllUsers = async () => {
  try {
    const snapshot = await admin.database().ref('users').once('value');
    const users = [];

    snapshot.forEach((childSnapshot) => {
      const user = childSnapshot.val();
      users.push({
        username: childSnapshot.key, // Assuming the username is the key
        password: user.password,    // Adjust according to your database structure
      });
    });

    return users;
  } catch (error) {
    console.error('Error fetching users from Firebase:', error);
    throw error; // Propagate the error for the caller to handle
  }
};

const getDeviceID = async (deviceType) => {
    try{
        // Fetch devices of the specified type from Firebase
        const snapshot = await admin.database().ref(`devices/${deviceType}`).once('value');
        
        // Check if the device type exists
        const devices = snapshot.val() || [];
        const deviceCount = devices.length;

        // Create the new device ID
        const newDeviceID = `${deviceType}${deviceCount + 1}`;
        
        // Create the new device entry
        const newDevice = {
            deviceID: newDeviceID,
            device_type: deviceType,
        };

        // Add the new device to the database using numerical index
        await admin.database().ref(`devices/${deviceType}/${deviceCount}`).set(newDevice);

        console.log(`Added new device to database: ${JSON.stringify(newDevice)}`);
        return newDeviceID;
    } catch (error) {
        console.error('Error fetching devices from Firebase:', error);
        throw error; // Propagate the error for the caller to handle
    }
}

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

// Export both the router and the getAllUsers function
module.exports = { router, getAllUsers, getDeviceID };
