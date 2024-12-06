const express = require('express');
const router = express.Router();
const admin = require('firebase-admin');
const mqttClient = require('../mqttclient'); // Import the MQTT client

// Route to add a new user to Firebase
// Route to add or update a gate device in Firebase
router.post('/devConfig', async (req, res) => {
    const { devID, location, gateType, building, deviceType } = req.body;

    if (!devID || !location || !gateType || !deviceType) {
        return res.status(400).json({ success: false, message: 'All fields are required.' });
    }

    try {
        // Reference the 'devices' path in the database
        const devicesRef = admin.database().ref(`devices/${deviceType}`);
        const snapshot = await devicesRef.once('value');

        if (snapshot.exists()) {
            const devices = snapshot.val();

            // Check if the deviceID exists in the devices table
            const deviceIndex = devices.findIndex(device => device.deviceID === devID);

            if (deviceIndex !== -1) {
                // Update the existing device
                devices[deviceIndex].location = location;
                devices[deviceIndex].gateType = gateType;
                devices[deviceIndex].building = building;

                // Save the updated devices array back to the database
                await devicesRef.set(devices);

                console.log('Device config updated successfully');
                // Publish the new user to the MQTT topic
                const userPayload = `${devID}:${location}`;
                mqttClient.publish('ELEC520/deviceConfig', userPayload, (err) => {
                    if (err) {
                        console.error('Failed to publish MQTT message:', err);
                        return res.status(500).json({ message: 'User added but failed to notify via MQTT' });
                    }
                    console.log('Published new user to MQTT:', userPayload);
                    res.json({ success: true, message: 'User added successfully' });
                });

                res.json({ success: true, message: 'Device config updated successfully' });
            } else {
                // If no matching device is found, return a specific error
                console.error('Device ID not found');
                res.status(400).json({ 
                    success: false, 
                    message: 'Enter a valid device ID', 
                });
            }
        } else {
            // If no devices exist in the table
            console.error('No devices found in the database');
            res.status(400).json({ 
                success: false, 
                message: 'Enter a valid device ID', 
            });
        }
    } catch (error) {
        console.error('Error updating device config in Firebase:', error);
        res.status(500).json({ success: false, message: 'Failed to update device config' });
    }
});


  

module.exports = router;
