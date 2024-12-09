using Firebase.Database;
using Firebase.Database.Query;
using System.Collections.Generic;
using System.Threading.Tasks;
using System.Linq;
using System.Net.NetworkInformation;
using System.Windows;

namespace TerminalWPF
{
    public class FirebaseService
    {
        private static readonly FirebaseClient firebaseClient = new FirebaseClient(
            "https://elec520-58761-default-rtdb.europe-west1.firebasedatabase.app/",
            new FirebaseOptions
            {
                AuthTokenAsyncFactory = () => Task.FromResult("AIzaSyBaEAAnRaCB5Q6HrW_rg3PWlU_gjdxw5ZY")
            });

        public static async Task WriteDataAsync<T>(string path, T data)
        {
            if (NetworkInterface.GetIsNetworkAvailable())
            {
                await firebaseClient
                          .Child(path)
                          .PutAsync(data);
            }
        }

        public static async Task<List<User>> ReadUsersAsync(string path)
        {
            if (NetworkInterface.GetIsNetworkAvailable())
            {
                var result = await firebaseClient
                                 .Child(path)
                                 .OnceAsync<User>();

                var users = new List<User>();
                foreach (var item in result)
                {
                    users.Add(item.Object);
                }
                return users;
            } else
            {
                return new List<User>();
            }
        }
    }
}
