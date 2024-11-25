// Permissions variable to control access to the Admin section
const userPermissions = '<%= permissions %>'; // Replace with actual permission-checking method if needed
// Import the MQTT library

const userSource = new EventSource('/api/userAccess-updates');
const deviceSource = new EventSource('/api/deviceStatus-updates');

userSource.addEventListener('open', function () {
  console.log('User access SSE connection opened');
});

deviceSource.addEventListener('open', function () {
  console.log('Device status SSE connection opened');
});

userSource.addEventListener('message', function (e) {
  console.log('UserAccess table update received:', e.data);

  // Fetch the updated data and refresh the table
  fetch('/api/get-userAccess-data')
    .then(response => {
      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }
      return response.json();
    })
    .then(data => {
      console.log("Updated userAccess data:", data);
    
      // Get the table body element
      const tableBody = document.getElementById("useraccess").querySelector("tbody");
      if (!tableBody) {
        console.error("Table body not found. Ensure the table structure is correct in the HTML.");
        return;
      }
    
      // Clear existing rows
      tableBody.innerHTML = '';
    
      // Iterate over the data array and populate the table
      data.forEach(rowData => {
        const row = document.createElement("tr");
    
        // Create and populate the Username cell
        const usernameCell = document.createElement("td");
        usernameCell.textContent = rowData.user || "Unknown"; // Fallback to "Unknown" if user field is missing
        row.appendChild(usernameCell);
    
        // Create and populate the Time Accessed cell
        const timeAccessedCell = document.createElement("td");
        timeAccessedCell.textContent = new Date(rowData.timeAccessed).toLocaleString() || "Unknown"; // Format timestamp
        row.appendChild(timeAccessedCell);
    
        // Create and populate the Location cell
        const locationCell = document.createElement("td");
        locationCell.textContent = rowData.location || "Unknown"; // Fallback to "Unknown" if location field is missing
        row.appendChild(locationCell);
    
        // Append the row to the table body
        tableBody.appendChild(row);

        console.log("Updated table");
      });
    })
    .catch(error => console.error("Error updating table data:", error));    
});

deviceSource.addEventListener('message', function (e) {
  console.log('devicePing table update received:', e.data);

  // Fetch the updated data and refresh the table
  fetch('/api/get-deviceStatus-data')
    .then(response => {
      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }
      return response.json();
    })
    .then(data => {
      console.log("Updated devicePing data:", data);
    
      // Get the table body element
      const tableBody = document.getElementById("devicePing").querySelector("tbody");
      if (!tableBody) {
        console.error("Table body not found. Ensure the table structure is correct in the HTML.");
        return;
      }
    
      // Clear existing rows
      tableBody.innerHTML = '';
    
      // Iterate over the data array and populate the table
      data.forEach(rowData => {
        const row = document.createElement("tr");
    
        // Create and populate the Username cell
        const deviceIDCell = document.createElement("td");
        deviceIDCell.textContent = rowData.deviceID || "Unknown"; // Fallback to "Unknown" if user field is missing
        row.appendChild(deviceIDCell);
    
        // Create and populate the Time Accessed cell
        const lastPingCell = document.createElement("td");
        lastPingCell.textContent = new Date(rowData.lastPing).toLocaleString() || "Unknown"; // Format timestamp
        row.appendChild(lastPingCell);
    
        // Create and populate the Status cell
        const statusCell = document.createElement("td");
        const imageCell = document.createElement("img");

        // Dynamically set the image source and alt text based on the rowData's status
        if (rowData.status === "online") {
          imageCell.src = "online.png"; // Path to the "online" image
          imageCell.alt = "Online";
        } else {
          imageCell.src = "offline.png"; // Path to the "offline" image
          imageCell.alt = "Offline";
        }

        // Append the image to the status cell and the status cell to the row
        statusCell.appendChild(imageCell);
        row.appendChild(statusCell);
        // Append the row to the table body
        tableBody.appendChild(row);

        console.log("Updated table");
      });
    })
    .catch(error => console.error("Error updating table data:", error));    
});

userSource.addEventListener('error', function (e) {
  console.error('User access SSE connection error:', e);
  if (e.target.readyState === EventSource.CLOSED) {
    console.log('User access SSE connection closed');
  }
});

deviceSource.addEventListener('error', function (e) {
  console.error('Device status SSE connection error:', e);
  if (e.target.readyState === EventSource.CLOSED) {
    console.log('Device status SSE connection closed');
  }
});



// Connect to the MQTT broker with authentication
const mqttClient = mqtt.connect('wss://40c06ef97ec5427eb54aa49e5c03c12c.s1.eu.hivemq.cloud:8884/mqtt', {
  username: 'dashboardweb', // Replace with your MQTT username
  password: 'Dashboardwebpass1', // Replace with your MQTT password
});

mqttClient.on('connect', function () {
  console.log('MQTT Connected');

  // Subscribe to topics for temperature and IMU data
  mqttClient.subscribe('ELEC520/temperature', function (err) {
    if (!err) console.log('Subscribed to ELEC520/temperature');
  });

  mqttClient.subscribe('ELEC520/imu', function (err) {
    if (!err) console.log('Subscribed to ELEC520/imu');
  });
});

mqttClient.on('error', function (err) {
  console.error('MQTT Connection Error:', err);
});


// Handle incoming MQTT messages
mqttClient.on('message', function (topic, message) {
  const payload = JSON.parse(message.toString());
  console.log(`Received message on ${topic}:`, payload);

  if (topic === 'ELEC520/temperature') {
    plotTemperature(payload);
  } else if (topic === 'ELEC520/imu') {
    plotIMU(payload);
  }
});

// Plot temperature data on the chart
function plotTemperature(jsonValue) {
  const keys = Object.keys(jsonValue);

  keys.forEach((key, i) => {
    const x = (new Date()).getTime(); // Current timestamp
    const y = Number(jsonValue[key]);

    if (chartT.series[i].data.length > 19) {
      chartT.series[i].addPoint([x, y], true, true, true); // Shift old points
    } else {
      chartT.series[i].addPoint([x, y], true, false, true);
    }
  });
}

// Plot IMU data on the chart
function plotIMU(jsonValue) {
  const axes = ['x', 'y', 'z'];

  axes.forEach((axis, i) => {
    const x = (new Date()).getTime(); // Current timestamp
    const y = Number(jsonValue[axis]);

    if (chartI.series[i].data.length > 19) {
      chartI.series[i].addPoint([x, y], true, true, true); // Shift old points
    } else {
      chartI.series[i].addPoint([x, y], true, false, true);
    }
  });
}

// Create Temperature Chart
var chartT = new Highcharts.Chart({
  chart:{
    renderTo:'chart-temperature',
    backgroundColor: 'white',
    borderColor: 'white'
  },
  series: [
    {
      name: 'Temperature',
      type: 'line',
      color: '#101D42',
      marker: {
        symbol: 'circle',
        radius: 3,
        fillColor: '#101D42',
      }
    },
    {
      name: 'Humidty',
      type: 'line',
      color: '#00A6A6',
      marker: {
        symbol: 'square',
        radius: 3,
        fillColor: '#00A6A6',
      }
    },
  ],
  title: {
    text: undefined
  },
  xAxis: {
    type: 'datetime',
    dateTimeLabelFormats: { second: '%H:%M:%S' }
  },
  yAxis: {
    title: {
      text: 'Temperature Celsius Degrees'
    }
  },
  credits: {
    enabled: false
  }
});

// Create Temperature Chart
var chartI = new Highcharts.Chart({
  chart:{
    renderTo:'chart-IMU',
    backgroundColor: 'white',
    borderColor: 'white'
  },
  series: [
    {
      name: 'X-Axis',
      type: 'line',
      color: '#101D42',
      marker: {
        symbol: 'circle',
        radius: 3,
        fillColor: '#101D42',
      }
    },
    {
      name: 'Y-Axis',
      type: 'line',
      color: '#00A6A6',
      marker: {
        symbol: 'square',
        radius: 3,
        fillColor: '#00A6A6',
      }
    },
    {
      name: 'Z-Axis',
      type: 'line',
      color: '#8B2635',
      marker: {
        symbol: 'triangle',
        radius: 3,
        fillColor: '#8B2635',
      }
    },
  ],
  title: {
    text: undefined
  },
  xAxis: {
    type: 'datetime',
    dateTimeLabelFormats: { second: '%H:%M:%S' }
  },
  yAxis: {
    title: {
      text: 'IMU Amplitude'
    }
  },
  credits: {
    enabled: false
  }
});

// Function to close the sidebar if clicking outside of it
function closeNavOnClickOutside(event) {
  const sidebar = document.getElementById("mySidebar");
  const menuButton = document.getElementById("menuButton");

  // Check if click is outside the sidebar and menu button
  if (sidebar.style.width === "250px" && !sidebar.contains(event.target) && !menuButton.contains(event.target)) {
      toggleNav();
  }
}

document.addEventListener('DOMContentLoaded', function() {
  const userPermissions = document.body.getAttribute('data-permissions');

  console.log("User Permissions in JS:", userPermissions); // Log for debugging

  // Only show Admin link if the user has admin permissions
  const adminLink = document.getElementById("admin-link");
  const codeLink = document.getElementById("gateCode-link");
  if (userPermissions !== 'admin' && adminLink) {
      adminLink.style.display = 'none';
      adminLink.onclick = "";
      codeLink.style.display = 'block';
  } else {
      adminLink.style.display = 'block';
      adminLink.onclick = showAdmin; // Correctly assign the function reference
      codeLink.style.display = 'none';
  }
  showDashboard();
});


// Add event listener for clicks on the whole document
document.addEventListener("click", closeNavOnClickOutside);

document.getElementById('logoutButton').addEventListener('click', function(event) {
  event.preventDefault(); // Prevent the default anchor behavior

  fetch('/logout', {
      method: 'POST',
      credentials: 'include'
  })
  .then(response => {
      if (response.redirected) {
          sessionStorage.removeItem('isLoggedIn'); // Clear session storage
          window.location.href = response.url; // Redirect to login page
      }
  })
  .catch(error => console.error('Error logging out:', error));
});

document.getElementById('gateCode-link').addEventListener('click', async function (event) {
  event.preventDefault(); // Prevent the default anchor behavior
  closeSidebar(); // Close the sidebar after clicking

  try {
    // Get the logged-in user's username
    const loggedInUser = sessionStorage.getItem('user'); // Replace with your actual method to get the logged-in user's key
    if (!loggedInUser) {
      alert("No logged-in user found.");
      return;
    }

    // Fetch user data from the server
    const response = await fetch(`/api/get-user/${loggedInUser}`);
    if (!response.ok) {
      throw new Error(`Error fetching user data: ${response.statusText}`);
    }

    const userData = await response.json();

    // Populate the existing popup with user data
    document.getElementById('userUsername').value = userData.username || '';
    document.getElementById('editPasswordUser').value = userData.password || '';
    document.getElementById('editGateCodeUser').value = userData.gateCode || '';
    document.getElementById('userRoomAccess').value = userData.location || '';
    document.getElementById('editPermissionsUser').value = userData.permissions || 'default';

    // Show the popup
    document.getElementById('userOverlayPopUp').style.display = 'block';
    document.getElementById('userPopUp').style.display = 'block';
  } catch (error) {
    console.error("Error:", error);
    alert("An error occurred while fetching user data.");
  }
});


// Add event listener for the save button
async function saveUserChanges() {
  // Get user inputs
  const usernameInput = document.getElementById('userUsername');
  if (!usernameInput || !usernameInput.value) {
    alert("User information is missing.");
    return;
  }

  const updatedData = {
    username: usernameInput.value, // Ensure this is correctly populated
    password: document.getElementById('editPasswordUser').value,
  };

  try {
    // Make API call to update the user
    const response = await fetch(`/api/update-user/${updatedData.username}`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(updatedData),
    });

    if (response.ok) {
      // Success message
      document.getElementById('successMessageUser').style.display = 'block';
      document.getElementById('successTextUser').innerText = 'User updated successfully!';
      setTimeout(closeEditPopup,2000);
    } else {
      // Handle API errors
      const errorData = await response.json();
      throw new Error(errorData.message || 'Failed to update user.');
    }
  } catch (error) {
    // Display error message
    const errorElement = document.getElementById('errorMessagePopUpUser');
    const errorTextElement = document.getElementById('errorTextPopUpUser');
    
    if (errorElement && errorTextElement) {
      errorElement.style.display = 'block';
      errorTextElement.innerText = error.message;
    } else {
      console.error("Error displaying error message:", error);
    }
  }
}



function showDashboard() {
    // Show the dashboard section and hide the admin section
    document.getElementById("dashboard").style.display = "block";
    document.getElementById("adminSection").style.display = "none";
    document.getElementById("userPopUp").style.display = "none";
    closeSidebar(); // Close the sidebar after clicking
}

function showAdmin() {
    // Show the admin section and hide the dashboard section
    document.getElementById("dashboard").style.display = "none";
    document.getElementById("adminSection").style.display = "block";
    document.getElementById("userPopUp").style.display = "none";
    closeSidebar(); // Close the sidebar after clicking
}

function closeSidebar() {
    const sidebar = document.getElementById("mySidebar");
    const mainContent = document.getElementById("main");
    const menuButton = document.getElementById("menuButton");
    const body = document.body;

    // Set the sidebar to closed state
    sidebar.style.width = "0";
    mainContent.style.marginLeft = "0";
    menuButton.innerHTML = "☰";
    body.classList.remove("sidebar-open"); // Remove overlay if present
}

// Update toggleNav to open and close the sidebar
function toggleNav() {
    const sidebar = document.getElementById("mySidebar");
    if (sidebar.style.width === "250px") {
        closeSidebar();
    } else {
        // Open the sidebar
        sidebar.style.width = "250px";
        document.getElementById("main").style.marginLeft = "250px";
        document.getElementById("menuButton").innerHTML = "&times;";
        document.body.classList.add("sidebar-open"); // Add overlay if open
    }
}

function showPassword(passwordText) {
  // Check if passwordText is an input (use type toggle) or a span (use webkitTextSecurity)
  if (passwordText.tagName === "INPUT") {
    // Toggle input type
    if (passwordText.type === "password") {
      passwordText.type = "text"; // Show password
    } else {
      passwordText.type = "password"; // Hide password
    }
  } else {
    // Toggle webkitTextSecurity for non-input elements like <span>
    if (passwordText.style.webkitTextSecurity === "disc") {
      passwordText.style.webkitTextSecurity = "none"; // Show password
    } else {
      passwordText.style.webkitTextSecurity = "disc"; // Hide password
    }
  }
}


function openEditPopup(username, password, location, permissions) {
  // Populate the form fields
  document.getElementById("editUsername").value = username;
  document.getElementById("editPassword").value = password;
  document.getElementById("editRoomAccess").value = location;
  document.getElementById("editPermissions").value = permissions;

  // Display the popup
  document.getElementById("editUserPopup").style.display = "block";
  document.getElementById("overlayPopUp").style.display = "block";
}

function closeEditPopup() {
  // Hide the popup
  document.getElementById("errorMessagePopUp").style.display = "none";
  document.getElementById("editUserPopup").style.display = "none";
  document.getElementById("overlayPopUp").style.display = "none";
  document.getElementById("successMessage").style.display = "none";
  document.getElementById("errorMessage").style.display = "none";
  document.getElementById("editPassword").type = "password";
  document.getElementById("showEditPassword").checked = false;
  document.getElementById('userOverlayPopUp').style.display = 'none';
  document.getElementById('userPopUp').style.display = 'none';
  document.getElementById('successMessageUser').style.display = 'none';
  document.getElementById('errorMessagePopUpUser').style.display = 'none';
  document.getElementById("editPasswordUser").type = "password";
  document.getElementById("showEditPasswordUser").checked = false;
  document.getElementById("editGateCodeUser").type = "password";
  document.getElementById("showGateCode").checked = false;
}

function updateUserTableData() {
  fetch('/api/get-table-data')
    .then(response => response.json())
    .then(data => {
      console.log("Raw response:", data);

      // Assuming `data` is an array of objects representing table rows
      const tableBody = document.getElementById("adminUsers").querySelector("tbody");
      tableBody.innerHTML = ''; // Clear existing rows

      data.forEach(rowData => {
        const row = document.createElement("tr");

        // Create cells in the order of User, Password, Permissions, Edit
        const usernameCell = document.createElement("td");
        usernameCell.textContent = rowData.username;
        row.appendChild(usernameCell);

        const passwordCell = document.createElement("td");
        passwordCell.id = "userPassword";
        const passwordText = document.createElement("span");
        passwordText.className = "password-text";
        passwordText.style.webkitTextSecurity = "disc";
        passwordText.textContent = rowData.password;
        
        const showPasswordCheckbox = document.createElement("input");
        showPasswordCheckbox.type = "checkbox";
        showPasswordCheckbox.className = "show-password-checkbox";
        showPasswordCheckbox.onclick = function () {
          showPassword(passwordText);
        };
        
        passwordCell.appendChild(passwordText);
        passwordCell.appendChild(showPasswordCheckbox);
        row.appendChild(passwordCell);

        const codeKeyCell = document.createElement("td");
        codeKeyCell.id = "roomKey";
        const codeKeyText = document.createElement("span");
        codeKeyText.className = "password-text";
        codeKeyText.style.webkitTextSecurity = "disc";
        codeKeyText.textContent = rowData.gateCode;
        
        const showGateCodeCheckbox = document.createElement("input");
        showGateCodeCheckbox.type = "checkbox";
        showGateCodeCheckbox.className = "show-password-checkbox";
        showGateCodeCheckbox.onclick = function () {
          showPassword(codeKeyText);
        };
        
        codeKeyCell.appendChild(codeKeyText);
        codeKeyCell.appendChild(showGateCodeCheckbox);
        row.appendChild(codeKeyCell);

        const locationCell = document.createElement("td");
        locationCell.textContent = rowData.location;
        row.appendChild(locationCell);

        const permissionsCell = document.createElement("td");
        permissionsCell.textContent = rowData.permissions;
        row.appendChild(permissionsCell);

        const editCell = document.createElement("td");
        const editLink = document.createElement("a");
        editLink.className = "editpsw";
        editLink.textContent = "Edit";
        editLink.onclick = function () {
          openEditPopup(rowData.username, rowData.password, rowData.location, rowData.permissions);
        };
        editCell.appendChild(editLink);
        row.appendChild(editCell);

        tableBody.appendChild(row);
      });
    })
    .catch(error => console.error('Error updating table data:', error));
}


function delay(time) {
  return new Promise(resolve => setTimeout(resolve, time));
}

function saveChanges() {
  const username = document.getElementById("editUsername").value;
  const newPassword = document.getElementById("editPassword").value;
  const newRoomAccess = document.getElementById("editRoomAccess").value;
  const newPermissions = document.getElementById("editPermissions").value;

  console.log('Updating user with data:', { username, newPassword, newPermissions });

  fetch('/api/update-user', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({
      username: username,
      password: newPassword,
      location: newRoomAccess,
      permissions: newPermissions,
    }),
  })
    .then(response => response.json())
    .then(data => {
      console.log('Response data:', data);

      if (data.success) {
        document.getElementById("successText").style.color = "#000";
        document.getElementById("successText").innerText = 'User updated successfully';
        document.getElementById("successMessage").style.display = "block";
        document.getElementById("errorMessagePopUp").style.display = "none";

        delay(1000)
          .then(() => updateUserTableData())
          .then(() => closeEditPopup())
          .catch(error => console.error('Error updating table or closing popup:', error));
      } else {
        console.error('Server response indicates failure:', data.message);
        document.getElementById("errorTextPopUp").innerText = data.message || 'Failed to save changes. Please try again.';
        document.getElementById("errorMessagePopUp").style.display = "block";
        document.getElementById("successMessage").style.display = "none";
      }
    })
    .catch(error => {
      console.error('Error in update-user request:', error);
      document.getElementById("errorTextPopUp").innerText = 'Failed to save changes. Please try again.';
      document.getElementById("errorMessagePopUp").style.display = "block";
      document.getElementById("successMessage").style.display = "none";
    });
}

// document.addEventListener('DOMContentLoaded', () => {
//   const form = document.getElementById("addUser");
//   console.log('Attempting to add user: ',{ username, newPassword, newPermissions });
//   form.onsubmit = function (event) {
//     event.preventDefault(); // Prevent the default form submission
//     addUser(); // Call the addUser function
//   };
// });


function addUser() {
  const username = document.getElementById("uname").value;
  const newPassword = document.getElementById("psw").value;
  const newPermissions = document.getElementById("permissions").value;
  const newRooms = document.getElementById("roomAccess").value;



  console.log('Attempting to add user:', { username, newPassword, newPermissions });

  fetch('/api/add-user', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({
      username,
      password: newPassword,
      location: newRooms,
      permissions: newPermissions,
    }),
  })
    .then((response) => response.json())
    .then((data) => {
      if (data.success) {
        document.getElementById("errorMessage").style.display = "block";
        document.getElementById("errorText").innerText = 'User added successfully';
        document.getElementById("errorText").style.color = 'green';
        delay(1000)
        .then(() => updateUserTableData())
        .then(() => closeEditPopup())
        .catch(error => console.error('Error updating table or closing popup:', error));
      } else {
        document.getElementById("errorMessage").style.display = "block";
        document.getElementById("errorText").innerText = data.message || 'Failed to add user.';
        document.getElementById("errorText").style.color = 'red';
        delay(1000)
        .then(()=>document.getElementById("errorMessage").style.display = "none")
        .catch(error => console.error('Error updating table or closing popup:', error));
      }
    })
    .catch((error) => {
      console.error('Error:', error);
      document.getElementById("errorText").innerText = 'An error occurred. Please try again.';
      document.getElementById("errorText").style.color = 'red';
    });
}
function deviceConfig(){
  
  const devID = document.getElementById("dname").value;
  const newLocation = document.getElementById("devLocation").value;
  const newGateType = document.getElementById("gateType").value;
  let newBuilding;
  if(document.getElementById("devBuildingInput").style.display == "none"){
    newBuilding = newLocation;
  }
  else{
    newBuilding = document.getElementById("devBuilding").value;
  }
  console.log('Attempting to configure gate device:', { devID, location, gateType });

  fetch('/api/devConfig', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({
      devID,
      location: newLocation,
      gateType: newGateType,
      building: newBuilding,
    }),
  })
    .then((response) => response.json())
    .then((data) => {
      if (data.success) {
        document.getElementById("errorMessageDevice").style.display = "block";
        document.getElementById("errorTextDevice").innerText = 'User added successfully';
        document.getElementById("errorTextDevice").style.color = 'green';
        delay(1000)
        .then(() => document.getElementById("errorMessageDevice").style.display = "none")
        .then(() => document.getElementById("errorTextDevice").innerText = '')
        .catch(error => console.error('Error updating table or closing popup:', error));
      } else {
        document.getElementById("errorMessageDevice").style.display = "block";
        document.getElementById("errorTextDevice").innerText = data.message || 'Failed to add user.';
        document.getElementById("errorTextDevice").style.color = 'red';
        delay(1000)
        .then(()=>document.getElementById("errorMessageDevice").style.display = "none")
        .catch(error => console.error('Error closing popup:', error));
      }
    })
    .catch((error) => {
      console.error('Error:', error);
      document.getElementById("errorTextDevice").innerText = 'An error occurred. Please try again.';
      document.getElementById("errorTextDevice").style.color = 'red';
    });

}

// Function to toggle visibility of devBuildingInput based on gateType selection
document.getElementById("gateType").addEventListener("change", function () {
  const gateType = this.value; // Get the selected value
  const devBuildingInput = document.getElementById("devBuildingInput");

  if (gateType === "room") {
      // Show the devBuildingInput container for room
      devBuildingInput.style.display = "block";
  } else if (gateType === "entrance") {
      // Hide the devBuildingInput container for entrance
      devBuildingInput.style.display = "none";
  }
});

// Set initial state based on the default selection
document.addEventListener("DOMContentLoaded", function () {
  const gateType = document.getElementById("gateType").value;
  const devBuildingInput = document.getElementById("devBuildingInput");

  if (gateType === "room") {
      devBuildingInput.style.display = "block";
  } else {
      devBuildingInput.style.display = "none";
  }
});


if (!!window.EventSource) {
  var source = new EventSource('/events');

  source.addEventListener('open', function(e) {
    console.log("Events Connected");
  }, false);

  source.addEventListener('error', function(e) {
    if (e.target.readyState != EventSource.OPEN) {
      console.log("Events Disconnected");
    }
  }, false);

  source.addEventListener('message', function(e) {
    console.log("message", e.data);
  }, false);

}