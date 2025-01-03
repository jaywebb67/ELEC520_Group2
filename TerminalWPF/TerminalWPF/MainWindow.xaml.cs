﻿using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Net.NetworkInformation;
using System.Windows;
using System.Windows.Controls;

namespace TerminalWPF
{
    public partial class MainWindow : Window
    {
        private static readonly Random random = new Random();
        private List<User> users = new List<User>();
        private User logged_in = null;
        private const string logged_out = "Logged out";
        


        public MainWindow()
        {
            InitializeComponent();
            LoadUsers();
            DisplayLogin.Text = logged_out;
            InternetConnectionStatus.Text = NetworkInterface.GetIsNetworkAvailable() ? "Connected"  : "Not connected";
        }


        private async void LoadUsers()
        {
            InternetConnectionStatus.Text = NetworkInterface.GetIsNetworkAvailable() ? "Connected" : "Not connected";
            users = await FirebaseService.ReadUsersAsync("users");
        }

        private async void OnCreateUserClick(object sender, RoutedEventArgs e)
        {
            InternetConnectionStatus.Text = NetworkInterface.GetIsNetworkAvailable() ? "Connected" : "Not connected";
            if (logged_in != null && logged_in.permissions == "admin")
            {
                string username = UsernameTextBox.Text;
                string gateCode = GenerateUniqueGateCode();
                string password = PasswordBox.Password;
                string permissions = "default";
                string locations;

                if (string.IsNullOrEmpty(username) || string.IsNullOrEmpty(password) || gateCode == "no available code")
                {
                    UserDisplayTextBlock.Text = "Please enter a username, password, and ensure there are available codes.";
                    return;
                } else if (users.Any(u => u.username == username)){
                    UserDisplayTextBlock.Text = "Username already exists, please try another.";
                    return;
                }

                MessageBoxResult result = MessageBox.Show("Is the new user an admin?", "Create User", MessageBoxButton.YesNo, MessageBoxImage.Question);
                if (result == MessageBoxResult.Yes) { permissions = "admin"; MessageBox.Show("User is an admin, access granted to all locations", "Create User", MessageBoxButton.OK, MessageBoxImage.Information); }


                
                locations = result == MessageBoxResult.Yes ? "All" : GetRooms();
                

                var user = new User { username = username, gateCode = gateCode, password = password, permissions = permissions, location = locations};
                await FirebaseService.WriteDataAsync($"users/{username}", user);
                UserDisplayTextBlock.Text = $"User Created: Username = {user.username}, Gate Code = {user.gateCode}";
                users = await FirebaseService.ReadUsersAsync("users");
            } else
            {
                MessageBox.Show("Only admins may create new users", "User creation failed", MessageBoxButton.OK, MessageBoxImage.Warning);
            }

            ClearInputBoxes();
        }

        private string GetRooms()
        {
            InternetConnectionStatus.Text = NetworkInterface.GetIsNetworkAvailable() ? "Connected" : "Not connected";
            string rooms = string.Empty;
            InputDialog inputDialog = new InputDialog();
            inputDialog.ShowDialog();

            if (inputDialog.Finished)
            {
                rooms = inputDialog.Rooms;
            }

            inputDialog.Close();

            return rooms;
        }


        private async void UpdateUsers()
        {
            InternetConnectionStatus.Text = NetworkInterface.GetIsNetworkAvailable() ? "Connected" : "Not connected";
            users = await FirebaseService.ReadUsersAsync("users");
        }

        private void OnGateDataClick(object sender, RoutedEventArgs e)
        {
            UpdateUsers();
            DisplayTextBlock.Text = logged_in == null ? logged_out : logged_in.gateCode;
        }

        private void OnRoomDataClick(object sender, RoutedEventArgs e)
        {
            InternetConnectionStatus.Text = NetworkInterface.GetIsNetworkAvailable() ? "Connected" : "Not connected";
            UpdateUsers();
            DisplayTextBlock.Text = logged_in == null ? logged_out : logged_in.location;
        }



        private void OnLoginClick(object sender, RoutedEventArgs e)
        {
            InternetConnectionStatus.Text = NetworkInterface.GetIsNetworkAvailable() ? "Connected" : "Not connected";
            UpdateUsers();
            
            User attempt_login = users.Where(u => u.username == UsernameTextBox.Text).FirstOrDefault();
            
            if (attempt_login != null)
            {
                if (attempt_login.password == PasswordBox.Password)
                {
                    logged_in = attempt_login;
                    DisplayLogin.Text = "Logged in as: " + logged_in.username;
                }
                else
                {
                    MessageBox.Show("Incorrect password", "Login Failed", MessageBoxButton.OK, MessageBoxImage.Warning);
                    DisplayLogin.Text = logged_out;
                }
            } else if (NetworkInterface.GetIsNetworkAvailable())
            {
                MessageBox.Show("User not recognised", "Login Failed", MessageBoxButton.OK, MessageBoxImage.Warning);
                DisplayLogin.Text = logged_out;
            } else
            {
                MessageBox.Show("Not connected to the internet", "Login Failed", MessageBoxButton.OK, MessageBoxImage.Warning);
                DisplayLogin.Text = logged_out;
            }
            ClearInputBoxes();
        }

        private void OnLogoutClick(object sender, RoutedEventArgs e)
        {
            InternetConnectionStatus.Text = NetworkInterface.GetIsNetworkAvailable() ? "Connected" : "Not connected";
            logged_in = null;
            DisplayLogin.Text = logged_out;
            ClearTextInformation();
        }

        private string GenerateUniqueGateCode()
        {
            InternetConnectionStatus.Text = NetworkInterface.GetIsNetworkAvailable() ? "Connected" : "Not connected";
            UpdateUsers();
            string newCode = "";
            bool isUnique = false;

            if (users.Count < 1000)
            {
                while (!isUnique)
                {
                    newCode = random.Next(0, 1000000).ToString("D6"); // Generates a 6-digit number with leading zeros

                    isUnique = !users.Any(user => user.gateCode == newCode);
                }
                return newCode;
            }
            else
            {
                return "no available code";
            }
        }

        private void ClearInputBoxes()
        {
            UsernameTextBox.Text = string.Empty;
            PasswordBox.Password = string.Empty;
        }

        private void ClearTextInformation()
        {
            UserDisplayTextBlock.Text = string.Empty;
            DisplayTextBlock.Text = string.Empty;
        }
    }
}
