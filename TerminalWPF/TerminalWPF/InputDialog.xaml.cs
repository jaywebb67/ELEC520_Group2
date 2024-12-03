using System.Windows;

namespace TerminalWPF
{
    public partial class InputDialog : Window
    {
        public string InputText { get; private set; }

        public InputDialog()
        {
            InitializeComponent();
        }

        private void OnOkButtonClick(object sender, RoutedEventArgs e)
        {
            InputText = InputTextBox.Text;
            this.DialogResult = true;
        }
    }
}
