const express = require('express');
const bodyParser = require('body-parser');
const path = require('path');
const session = require('express-session');
require('dotenv').config();

const app = express();
const PORT = 3000;



// Initialize Firebase Admin SDK
const admin = require('firebase-admin');
const serviceAccount = require('./serviceAccountKey.json'); // Update the path as needed
admin.initializeApp({
  credential: admin.credential.cert(serviceAccount),
  databaseURL: "https://elec520-58761-default-rtdb.europe-west1.firebasedatabase.app"
});

// Import Firebase SDK
const { initializeApp } = require("firebase/app");
const { getDatabase, ref, get, child } = require("firebase/database");
// Firebase configuration
const firebaseConfig = {
  apiKey: "AIzaSyBaEAAnRaCB5Q6HrW_rg3PWlU_gjdxw5ZY",
  authDomain: "elec520-58761.firebaseapp.com",
  databaseURL: "https://elec520-58761-default-rtdb.europe-west1.firebasedatabase.app",
  projectId: "elec520-58761",
  storageBucket: "elec520-58761.firebasestorage.app",
  messagingSenderId: "31636359275",
  appId: "1:31636359275:web:2627996b0df78262c7892e",
  measurementId: "G-6FNJDP8F5Y"
};
// Initialize Firebase and database
const firebaseApp = initializeApp(firebaseConfig);
const database = getDatabase(firebaseApp);

// Middleware to parse JSON and URL-encoded data
app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: true }));

// Route setup
const updateUserRoute = require('./routes/updateUser');
const { router: getTableDataRoute } = require('./routes/getTableData');
const addUserRoute = require('./routes/addUser'); // Adjust path as needed
const mqttClient = require('./mqttclient'); // Import the MQTT client
const devConfigRoute = require('./routes/devConfig'); // Adjust path as needed

// Middleware
app.use(bodyParser.urlencoded({ extended: true }));
app.use(express.static(path.join(__dirname, 'public')));
app.use('/api', updateUserRoute);
app.use('/api', getTableDataRoute); // Register the route for get-table-data
app.use('/api', addUserRoute);
app.use('/api', devConfigRoute);

// Set EJS as the view engine
app.set('view engine', 'ejs');
app.set('views', path.join(__dirname, 'public'));

app.use(session({
  secret: process.env.SESSION_SECRET, // replace with your secret key
  resave: false,
  saveUninitialized: true
}));

// Serve login page (for both root and /login)
app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, 'public', 'login.html'));
});

app.get('/login', (req, res) => {
  res.sendFile(path.join(__dirname, 'public', 'login.html'));
});


app.get('/admin', (req, res) => {
  return res.redirect('/indexTutorial.html');
});

// Handle login form submission
app.post('/login', async (req, res) => {
  const { username, password } = req.body;
  try {
    const dbRef = ref(database);
    const snapshot = await get(child(dbRef, `users/${username}`));
    if (snapshot.exists() && snapshot.val().password === password) {
      req.session.user = {
        username,
        permissions: snapshot.val().permissions,
      };

      // Send a response that sets sessionStorage and redirects
      res.send(`
        <html>
          <head>
            <script>
              sessionStorage.setItem('isLoggedIn', 'true');
              sessionStorage.setItem('user','${username}');
              window.location.href = '/indexTutorial.html';
            </script>
          </head>
          <body>
            <p>Logging you in...</p>
          </body>
        </html>
      `);
    } else {
      res.redirect('/login?error=Invalid credentials, please try again.');
    }
  } catch (error) {
    console.error('Error accessing Firebase:', error);
    res.status(500).send('Server error');
  }
});

app.post('/api/update-user/:username', async (req, res) => {
  const { username } = req.params;
  const { password } = req.body;

  console.log("Updating user:", username);
  console.log("Request body:", req.body);

  if (!username || !password) {
    return res.status(400).json({ message: 'Invalid data provided.' });
  }
  try {
    await admin.database().ref(`users/${username}`).update({ password });
    res.json({ message: 'Password updated successfully' });
  } catch (error) {
    console.error('Error updating password:', error);
    res.status(500).json({ message: 'Failed to update password' });
  }
});


// Handle logout
app.post('/logout', (req, res) => {
  req.session.destroy(err => {
      if (err) {
          return res.status(500).send('Could not log out.');
      }
      // Redirect to login page after logout
      res.redirect('/login');
  });
});

// Admin login route
app.post('/api/admin-login', async (req, res) => {
  const { username, password } = req.body;

  try {
      const dbRef = ref(database);
      console.log(`Querying path: users/${username}`);
      const snapshot = await get(child(dbRef, `users/${username}`)); // Query the specific user

      if (snapshot.exists()) {
          const user = snapshot.val();
          console.log('User data:', user);

          // Compare raw password directly (for now, without bcrypt)
          if (user.password === password) {
              if (user.permissions === 'admin') {
                  return res.status(200).json({ isAdmin: true });
              } else {
                  return res.status(403).json({ message: 'You do not have admin privileges.' });
              }
          } else {
              return res.status(401).json({ message: 'Invalid credentials.' });
          }
      } else {
          console.error('User not found.');
          return res.status(401).json({ message: 'Invalid credentials.' });
      }
  } catch (error) {
      console.error('Error during admin login:', error);
      res.status(500).json({ message: 'Internal server error.' });
  }
});


// Render main dashboard
app.get('/indexTutorial.html', async (req, res) => {
  // Check that session data is correct
  if (!req.session.user) {
    return res.redirect('/login');
  }
  console.log("Session User:", req.session.user);
  
  try {
    const dbRef = ref(database);

    // Fetch user access and device status data
    const userAccessSnapshot = await get(child(dbRef, `userAccess`));
    const deviceStatusSnapshot = await get(child(dbRef, `deviceStatus`));
    const usersSnapshot = await get(child(dbRef, 'users'));

    const userAccessData = userAccessSnapshot.exists() ? userAccessSnapshot.val() : [];
    const deviceStatusData = deviceStatusSnapshot.exists() ? deviceStatusSnapshot.val() : [];
    const usersData = usersSnapshot.exists() ? usersSnapshot.val() : [];

    console.log("User Access Data:", userAccessData);
    console.log("Device Status Data:", deviceStatusData);
    console.log("User Data: ", usersData);

    res.render('indexTutorial', { 
      permissions: req.session.user.permissions,
      userAccessData: Object.values(userAccessData),
      deviceStatusData: Object.values(deviceStatusData),
      userData: Object.values(usersData)
    });
  } catch (error) {
    console.error('Error accessing Firebase:', error);
    res.status(500).send('Server error');
  }
});

app.get('/api/test', (req, res) => {
  res.json({ message: 'Server is working fine' });
});

// Start server
app.listen(PORT, () => {
  console.log(`Server running on http://localhost:${PORT}`);
});