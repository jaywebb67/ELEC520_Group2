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

require('./mqttclient'); // This will trigger the MQTT client logic

// Middleware to parse JSON and URL-encoded data
app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: true }));

// Route setup
const updateUserRoute = require('./routes/updateUser');
const { router: getTableDataRoute } = require('./routes/getTableData');
const addUserRoute = require('./routes/addUser'); // Adjust path as needed
const mqttClient = require('./mqttclient'); // Import the MQTT client

// Middleware
app.use(bodyParser.urlencoded({ extended: true }));
app.use(express.static(path.join(__dirname, 'public')));
app.use('/api', updateUserRoute);
app.use('/api', getTableDataRoute); // Register the route for get-table-data
app.use('/api', addUserRoute);

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