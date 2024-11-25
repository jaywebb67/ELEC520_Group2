const express = require('express');
const router = express.Router();
const admin = require('firebase-admin');
const mqttClient = require('../mqttclient'); // Import the MQTT client

// Route to add a new user to Firebase
// Route to add or update a gate device in Firebase
router.post('/devConfig', async (req, res) => {
    const { devID, location, gateType, building } = req.body;

    if (!devID || !location || !gateType) {
        return res.status(400).json({ success: false, message: 'All fields are required.' });
    }

    try {
        // Reference the 'devices/Gate' path in the database
        const deviceRef = admin.database().ref('devices/Gate');
        const snapshot = await deviceRef.once('value');

        if (snapshot.exists()) {
            const gates = snapshot.val();

            // Find the index of the gate with the matching deviceID
            const gateIndex = gates.findIndex(g => g.deviceID === devID);

            if (gateIndex !== -1) {
                // Update the existing gate
                gates[gateIndex].location = location;
                gates[gateIndex].gateType = gateType;
                gates[gateIndex].building = building;
                // Save the updated gates array back to the database
                await deviceRef.set(gates);

                console.log('Device config updated successfully');
                res.json({ success: true, message: 'Device config updated successfully' });
            } else {
                // If no matching gate is found, create a new one
                gates.push({
                    deviceID: devID,
                    device_type: 'Gate', // Assuming a fixed device type for this case
                    location,
                    gateType,
                    building,
                });

                // Save the updated gates array back to the database
                await deviceRef.set(gates);

                console.log('Device config added successfully');
                res.json({ success: true, message: 'Device config added successfully' });
            }
        } else {
            // If no gates exist yet, create the first one
            const newGate = [{
                deviceID: devID,
                device_type: 'Gate',
                location,
                gateType,
                building,
            }];

            await deviceRef.set(newGate);

            console.log('Device config added successfully');
            res.json({ success: true, message: 'Device config added successfully' });
        }
    } catch (error) {
        console.error('Error adding/updating device config to Firebase:', error);
        res.status(500).json({ success: false, message: 'Failed to add/update device config' });
    }
});

  

module.exports = router;
