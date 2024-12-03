using Firebase.Database;
using Firebase.Database.Query;
using System;
using System.Threading.Tasks;
using System.Windows;

namespace YourAppNamespace
{
    public partial class App : Application
    {
        private static readonly FirebaseClient firebaseClient = new FirebaseClient(
            "https://elec520-58761-default-rtdb.europe-west1.firebasedatabase.app/",
            new FirebaseOptions
            {
                AuthTokenAsyncFactory = () => Task.FromResult("AIzaSyBaEAAnRaCB5Q6HrW_rg3PWlU_gjdxw5ZY")
            });

        public static FirebaseClient FirebaseClient => firebaseClient;

        protected override void OnStartup(StartupEventArgs e)
        {
            base.OnStartup(e);
            // Additional initialization if needed
        }
    }
}
