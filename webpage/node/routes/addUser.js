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
        // Check if the user already exists in the database
        const userRef = admin.database().ref(`users/${username}`);
        const snapshot = await userRef.once('value');

        if (snapshot.exists()) {
            return res.status(400).json({ success: false, message: 'User already exists' });
        }

        // Generate a unique 6-digit gateCode
        let gateCode;
        const usersRef = admin.database().ref('users');
        let isUnique = false;


        while (!isUnique) {
            gateCode = Math.floor(Math.random*1000000).toString().padStart(6,'0'); // Generate 6-digit code
            const usersSnapshot = await usersRef.once('value');
            isUnique = !Object.values(usersSnapshot.val() || {}).some(user => user.gateCode === gateCode);
        }

        // Save the user data in Firebase Realtime Database
        await userRef.set({
            password,
            location,
            permissions,
            username,
            gateCode
        });

        // Publish the new user to the MQTT topic
        const userPayload = `${username}:${gateCode}`;
        mqttClient.publish('ELEC520/users/add', userPayload, (err) => {
            if (err) {
                console.error('Failed to publish MQTT message:', err);
                return res.status(500).json({ message: 'User added but failed to notify via MQTT' });
            }
            console.log('Published new user to MQTT:', userPayload);
            res.json({ success: true, message: 'User added successfully' });
        });
    } catch (error) {
        console.error('Error adding user to Firebase:', error);
        res.status(500).json({ success: false, message: 'Failed to add user' });
    }
});

  

module.exports = router;
