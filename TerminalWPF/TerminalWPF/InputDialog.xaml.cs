using System.Windows;
using System.Windows.Input;
using System.Text.RegularExpressions;
using System.Windows.Controls;
using System.Diagnostics;

namespace TerminalWPF
{
    public partial class InputDialog : Window
    {
        public string InputText { get; private set; }
        public string Rooms {  get; private set; }
        public bool Finished { get; private set; }

        public InputDialog()
        {
            InitializeComponent();
            Finished = false;
            Rooms = string.Empty;
        }

        private void OnNextClick(object sender, RoutedEventArgs e)
        {
            InputText = InputTextBox.Text;

            if (InputText != string.Empty)
            {
                Rooms = Rooms + InputText + ", ";
                InputTextBox.Text = string.Empty;
                InputTextBox.Focus();
            }
        }

        private void OnFinishedClick(object sender, RoutedEventArgs e)
        {
            InputText = InputTextBox.Text;
            if (InputText == string.Empty)
            {
                Debug.Print(Rooms);
                if (Rooms != string.Empty)
                {
                    Rooms = Rooms.Remove(Rooms.Length - 2);
                }
            }
            else
            {
                Rooms = Rooms + InputText;
            }
            Finished = true;
            this.DialogResult = true;
        }

        private void InputTextBox_PreviewTextInput(object sender, TextCompositionEventArgs e)
        {
            e.Handled = !IsTextAllowed(e.Text);
        }

        private void InputTextBox_Pasting(object sender, DataObjectPastingEventArgs e)
        {
            if (e.DataObject.GetDataPresent(typeof(string)))
            {
                string text = (string)e.DataObject.GetData(typeof(string));
                if (!IsTextAllowed(text))
                {
                    e.CancelCommand();
                }
            }
            else
            {
                e.CancelCommand();
            }
        }

        private static bool IsTextAllowed(string text)
        {
            // Allow only alphabetic or numeric characters
            Regex regex = new Regex("[^a-zA-Z0-9]+");
            return !regex.IsMatch(text);
        }
    }
}
