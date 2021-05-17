using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO.Ports;


namespace Zonnestroom_UART
{
   
    public partial class FormMain : Form
    {
        static string StatusText;
        DateTime now = DateTime.Now;

        public FormMain()
        {
            InitializeComponent();
        }

        private void FormMain_Load(object sender, EventArgs e)
        {
            string[] ports = SerialPort.GetPortNames();

            foreach (string port in ports)
            {
                comboBoxSerialPort.Items.Add(port);
            }
        }

        private void button_connect_to_device_Click(object sender, EventArgs e)
        {
            if (comboBoxSerialPort.Text.Trim().Length == 0)
            {
                MessageBox.Show("Select a port and try again", Text, MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return;
            }
            serialPort1.PortName = comboBoxSerialPort.Text.Trim();
            serialPort1.BaudRate = Convert.ToInt32("9600");
            serialPort1.Open();

            if (serialPort1.IsOpen)
            {
                button_connect_to_device.Enabled = false;
                button_disconnect_to_device.Enabled = true;
                //zend Q om data ingestelde waarde van consumer en pusles te verkrijgen
                serialPort1.Write("Q000000000000000");
            }
        }

        private void button_disconnect_to_device_Click(object sender, EventArgs e)
        {
            if (serialPort1.IsOpen)
            {
                serialPort1.Close();
                button_disconnect_to_device.Enabled = false;
                button_connect_to_device.Enabled = true;
            }
        }

        private void serialPort1_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            StatusText = serialPort1.ReadLine();
            Invoke(new EventHandler(DisplayThreadText));
        }

        private void WriteStatusLine(string text)
        {
            var words = text.Split(':');
            if(words[0] == "K")
            {
                label6.Text = words[1];
            }
            else if (words[0] == "D")
            {
                label7.Text = words[1];
            }
            else if (words[0] == "C")
            {
                textBox_consumer.Text = words[1];
            }
            else if (words[0] == "P")
            {
                textBox_pulses.Text = words[1];
            }
        }

        private void DisplayThreadText(object sender, EventArgs e)
        {
            WriteStatusLine(StatusText);
        }


        private void button1_Click(object sender, EventArgs e)
        {


            if (serialPort1.IsOpen && textBox_consumer.Text.Length > 0 && textBox_pulses.Text.Length > 0)
            {
                int consumer_value_length = textBox_consumer.Text.Length;
                int textBox_pulses_length = textBox_pulses.Text.Length;
                string verzenden_value_consumer = "";
                string verzenden_value_pulses = "";
                if (consumer_value_length <= 7)
                {
                    int value = 7 - consumer_value_length;
                    switch (value)
                    {
                        case 0:
                            verzenden_value_consumer = "c" + textBox_consumer.Text;
                            break;
                        case 1:
                            verzenden_value_consumer = "c?" + textBox_consumer.Text;
                            break;
                        case 2:
                            verzenden_value_consumer = "c??" + textBox_consumer.Text;
                            break;
                        case 3:
                            verzenden_value_consumer = "c???" + textBox_consumer.Text;
                            break;
                        case 4:
                            verzenden_value_consumer = "c????" + textBox_consumer.Text;
                            break;
                        case 5:
                            verzenden_value_consumer = "c?????" + textBox_consumer.Text;
                            break;
                        case 6:
                            verzenden_value_consumer = "c??????" + textBox_consumer.Text;
                            break;
                    }

                }

                if (textBox_pulses_length <= 7)
                {
                    int value = 7 - textBox_pulses_length;
                    switch (value)
                    {
                        case 0:
                            verzenden_value_pulses = "p" + textBox_pulses.Text;
                            break;
                        case 1:
                            verzenden_value_pulses = "p?" + textBox_pulses.Text;
                            break;
                        case 2:
                            verzenden_value_pulses = "p??" + textBox_pulses.Text;
                            break;
                        case 3:
                            verzenden_value_pulses = "p???" + textBox_pulses.Text;
                            break;
                        case 4:
                            verzenden_value_pulses = "p????" + textBox_pulses.Text;
                            break;
                        case 5:
                            verzenden_value_pulses = "p?????" + textBox_pulses.Text;
                            break;
                        case 6:
                            verzenden_value_pulses = "p??????" + textBox_pulses.Text;
                            break;
                    }

                }


                serialPort1.Write(verzenden_value_consumer + verzenden_value_pulses);

                // serialPort1.Write(verzenden_value_pulses);
                MessageBox.Show("Values are set:" + "\n" + "Consumer => " + textBox_consumer.Text + "\n" + "Pulses => " + textBox_pulses.Text, Text, MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
            else
            {
                MessageBox.Show("Select a port or set the values and try again", Text, MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return;
            }
        }
    }
}
