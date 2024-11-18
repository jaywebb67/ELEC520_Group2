const express = require('express');
const router = express.Router();
const admin = require('firebase-admin'); // Firebase Admin SDK should already be initialized in app.js
const eventEmitter = require('../eventEmitter.js');
const mqttClient = require('../mqttclient'); // Import the MQTT client


function notifyTableUpdate() {
  eventEmitter.emit('table_update', { message: 'Table data updated' });
}

// Define updateUserInFirebase function
function updateUserInFirebase(username, password, permissions) {
  // Code to update the user in Firebase
  return new Promise((resolve, reject) => {
    // Assume firebaseAdmin is initialized
    admin.database().ref(`users/${username}`)
      .update({ password, permissions })
      .then(() => resolve('User updated successfully'))
      .catch(error => reject(error));
  });
}


// Route to update user information
router.post('/update-user', async (req, res) => {
  const { username, password, permissions } = req.body;
  console.log('Received data to update user:', { username, password, permissions }); // Corrected log

  if (!username || !password || !permissions) {
    return res.status(400).json({ success: false, message: 'All fields are required.' });
  }

  try {
    await admin.database().ref(`users/${username}`).update({
      password,
      permissions,
    })
    .then(() => {
      // Publish the new user to the MQTT topic
      // Format the payload as 'user:password'
      const userPayload = `${username}:${password}`;

      mqttClient.publish('ELEC520/users/update', userPayload, (err) => {
          if (err) {
              console.error('Failed to publish MQTT message:', err);
              return res.status(500).json({ message: 'User added but failed to notify via MQTT' });
          }
          console.log('Published new user to MQTT:', userPayload);
          res.json({ success: true, message: 'User updated successfully' });
          notifyTableUpdate(); // Emit table update event
      });
  })
  } catch (error) {
    console.error('Error updating user in Firebase:', error);
    res.status(500).json({ success: false, message: 'Failed to update user' });
  }
});


module.exports = router;
