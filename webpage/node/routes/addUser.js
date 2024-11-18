const express = require('express');
const router = express.Router();
const admin = require('firebase-admin');
const mqttClient = require('../mqttclient'); // Import the MQTT client

// Route to add a new user to Firebase
router.post('/add-user', async (req, res) => {
    const { username, password, permissions } = req.body;
  
    if (!username || !password || !permissions) {
      return res.status(400).json({ success: false, message: 'All fields are required.' });
    }
  
    try {
        // Save the user data in Firebase Realtime Database
        await admin.database().ref(`users/${username}`).set({
            password,
            permissions,
            username,
        })
        .then(() => {
            // Publish the new user to the MQTT topic
            // Format the payload as 'user:password'
            const userPayload = `${username}:${password}`;
    
            mqttClient.publish('ELEC520/users/add', userPayload, (err) => {
                if (err) {
                    console.error('Failed to publish MQTT message:', err);
                    return res.status(500).json({ message: 'User added but failed to notify via MQTT' });
                }
                console.log('Published new user to MQTT:', userPayload);
                res.json({ success: true, message: 'User added successfully' });
            });
        })
    } catch (error) {
      console.error('Error adding user to Firebase:', error);
      res.status(500).json({ success: false, message: 'Failed to add user' });
    }
});
  

module.exports = router;
