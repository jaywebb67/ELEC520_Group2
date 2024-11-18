// Permissions variable to control access to the Admin section
const userPermissions = '<%= permissions %>'; // Replace with actual permission-checking method if needed

// Get current sensor readings when the page loads
window.addEventListener('load', getReadings);

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



//Plot temperature in the temperature chart
function plotTemperature(jsonValue) {

  var keys = Object.keys(jsonValue);
  console.log(keys);
  console.log(keys.length);

  for (var i = 0; i < keys.length; i++){
    var x = (new Date()).getTime();
    console.log(x);
    const key = keys[i];
    var y = Number(jsonValue[key]);
    console.log(y);

    if(chartT.series[i].data.length > 40) {
      chartT.series[i].addPoint([x, y], true, true, true);
    } else {
      chartT.series[i].addPoint([x, y], true, false, true);
    }

  }
}

// Function to get current readings on the webpage when it loads for the first time
function getReadings(){
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var myObj = JSON.parse(this.responseText);
      console.log(myObj);
      plotTemperature(myObj);
    }
  };
  xhr.open("GET", "/readings", true);
  xhr.send();
}

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
  if (userPermissions !== 'admin' && adminLink) {
      adminLink.style.display = 'none';
      adminLink.onclick = "";
  } else {
      adminLink.style.display = 'block';
      adminLink.onclick = showAdmin; // Correctly assign the function reference
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

function showDashboard() {
    // Show the dashboard section and hide the admin section
    document.getElementById("dashboard").style.display = "block";
    document.getElementById("adminSection").style.display = "none";
    closeSidebar(); // Close the sidebar after clicking
}

function showAdmin() {
    // Show the admin section and hide the dashboard section
    document.getElementById("dashboard").style.display = "none";
    document.getElementById("adminSection").style.display = "block";
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


function openEditPopup(username, password, permissions) {
  // Populate the form fields
  document.getElementById("editUsername").value = username;
  document.getElementById("editPassword").value = password;
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

        const permissionsCell = document.createElement("td");
        permissionsCell.textContent = rowData.permissions;
        row.appendChild(permissionsCell);

        const editCell = document.createElement("td");
        const editLink = document.createElement("a");
        editLink.className = "editpsw";
        editLink.textContent = "Edit";
        editLink.onclick = function () {
          openEditPopup(rowData.username, rowData.password, rowData.permissions);
        };
        editCell.appendChild(editLink);
        row.appendChild(editCell);

        tableBody.appendChild(row);
      });
    })
    .catch(error => console.error('Error updating table data:', error));
}

source.addEventListener('table_update', function(e) {
  console.log("table_update event received:", e.data);
  updateUserTableData(); // Fetch and display new table data
}, false);


function delay(time) {
  return new Promise(resolve => setTimeout(resolve, time));
}

function saveChanges() {
  const username = document.getElementById("editUsername").value;
  const newPassword = document.getElementById("editPassword").value;
  const newPermissions = document.getElementById("editPermissions").value;

  console.log('Updating user with data:', { username, newPassword, newPermissions });

  fetch('/api/update-user', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({
      username: username,
      password: newPassword,
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

  console.log('Attempting to add user:', { username, newPassword, newPermissions });

  fetch('/api/add-user', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({
      username,
      password: newPassword,
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
        document.getElementById("errorText").innerText = data.message || 'Failed to add user.';
        document.getElementById("errorText").style.color = 'red';
      }
    })
    .catch((error) => {
      console.error('Error:', error);
      document.getElementById("errorText").innerText = 'An error occurred. Please try again.';
      document.getElementById("errorText").style.color = 'red';
    });
}





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

  source.addEventListener('new_readings', function(e) {
    console.log("new_readings", e.data);
    var myObj = JSON.parse(e.data);
    console.log(myObj);
    plotTemperature(myObj);
  }, false);
}