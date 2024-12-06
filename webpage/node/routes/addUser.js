const express = require('express');
const router = express.Router();
const admin = require('firebase-admin');
const mqttClient = require('../mqttclient'); // Import the MQTT client

// Route to add a new user to Firebase
router.post('/add-user', async (req, res) => {
    const { username, password, location, permissions } = req.body;

    if (!username || !password || !permissions || !location) {
        return res.status(400).json({ success: false, message: 'All fields are required.' });
    }

    try {
        const userRef = admin.database().ref(`users/${username}`);
        const snapshot = await userRef.once('value');

        if (snapshot.exists()) {
            return res.status(400).json({ success: false, message: 'User already exists' });
        }

        let gateCode;
        const usersRef = admin.database().ref('users');
        let isUnique = false;

        while (!isUnique) {
            gateCode = Math.floor(Math.random() * 1000000).toString().padStart(6, '0');
            const usersSnapshot = await usersRef.once('value');
            isUnique = !Object.values(usersSnapshot.val() || {}).some(user => user.gateCode === gateCode);
        }

        await userRef.set({
            password,
            location,
            permissions,
            username,
            gateCode
        });

        const currentTime = new Date().toISOString();
        await admin.database().ref('users/lastUpdated').set({ lastUpdated: currentTime });

        // Retrieve gate devices with location matching the user's location
        const gateDevicesRef = admin.database().ref('devices/Gate');
        const gateDevicesSnapshot = await gateDevicesRef.once('value');
        const gateDevices = Object.values(gateDevicesSnapshot.val() || []); // Ensure it's an array
        const matchingGateDevices = gateDevices.filter(device => device.location === location);

        const gateDeviceUpdates = matchingGateDevices.map(async device => {
            const devicePayload = `${username}:${gateCode}`;
            const topic = `ELEC520/users/add/${device.deviceID}`;
            mqttClient.publish(topic, devicePayload, { qos: 0 }, err => {
                if (err) console.error(`Failed to publish to topic ${topic}:`, err);
            });
            await admin.database().ref(`devices/Gate/${matchingGateDevices.indexOf(device)}`).update({ usersUpdated: currentTime });
        });

        const alarmDevicesRef = admin.database().ref('devices/Alarm');
        const alarmDevicesSnapshot = await alarmDevicesRef.once('value');
        const alarmDevices = alarmDevicesSnapshot.val() || [];

        const alarmDeviceUpdates = alarmDevices.map(async alarmDevice => {
            await admin.database().ref(`devices/Alarm/${alarmDevices.indexOf(alarmDevice)}`).update({ usersUpdated: currentTime });
        });

        await Promise.all([...gateDeviceUpdates, ...alarmDeviceUpdates]);

        if (permissions === 'admin') {
            const adminPayload = `${username}:${gateCode}`;
            mqttClient.publish('ELEC520/users/admin/add', adminPayload, { qos: 0 }, err => {
                if (err) console.error('Failed to publish admin user MQTT message:', err);
            });
        }

        return res.json({ success: true, message: 'User added successfully' });
    } catch (error) {
        console.error('Error adding user to Firebase:', error);
        return res.status(500).json({ success: false, message: 'Failed to add user' });
    }
});



  

module.exports = router;
