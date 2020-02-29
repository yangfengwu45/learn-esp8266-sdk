using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace SerialPort
{
    public partial class Form1 : Form
    {
        String serialPortName;

        byte[] UsartReadBuff = new byte[1024];//接收数据缓存
        int UsartReadCnt = 0;//串口接收的数据个数
        int UsartReadCntCopy = 0;//用于拷贝串口接收的数据个数
        int UsartIdleCnt = 0;//空闲时间累加变量
        public Form1()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            string[] ports = System.IO.Ports.SerialPort.GetPortNames();//获取电脑上可用串口号
            comboBox1.Items.AddRange(ports);//给comboBox1添加数据
            comboBox1.SelectedIndex = comboBox1.Items.Count > 0 ? 0 : -1;//如果里面有数据,显示第0个

            comboBox2.Text = "115200";/*默认波特率:115200*/
            comboBox3.Text = "1";/*默认停止位:1*/
            comboBox4.Text = "8";/*默认数据位:8*/
            comboBox5.Text = "无";/*默认奇偶校验位:无*/
        }

        private void button1_Click(object sender, EventArgs e)
        {
            if (button1.Text == "打开串口"){//如果按钮显示的是打开
                try{//防止意外错误
                    serialPort1.PortName = comboBox1.Text;//获取comboBox1要打开的串口号
                    serialPortName = comboBox1.Text;
                    serialPort1.BaudRate = int.Parse(comboBox2.Text);//获取comboBox2选择的波特率
                    serialPort1.DataBits = int.Parse(comboBox4.Text);//设置数据位
                    /*设置停止位*/
                    if (comboBox3.Text == "1") { serialPort1.StopBits = StopBits.One; }
                    else if (comboBox3.Text == "1.5") { serialPort1.StopBits = StopBits.OnePointFive; }
                    else if (comboBox3.Text == "2") { serialPort1.StopBits = StopBits.Two; }
                    /*设置奇偶校验*/
                    if (comboBox5.Text == "无") { serialPort1.Parity = Parity.None; }
                    else if (comboBox5.Text == "奇校验") { serialPort1.Parity = Parity.Odd; }
                    else if (comboBox5.Text == "偶校验") { serialPort1.Parity = Parity.Even; }

                    serialPort1.Open();//打开串口
                    button1.Text = "关闭串口";//按钮显示关闭串口
                }
                catch (Exception err)
                {
                    MessageBox.Show("打开失败"+ err.ToString(), "提示!");//对话框显示打开失败
                }
            }
            else{//要关闭串口
                try{//防止意外错误
                    serialPort1.Close();//关闭串口
                }
                catch (Exception){}
                button1.Text = "打开串口";//按钮显示打开
            }
        }


        protected override void WndProc(ref Message m)
        {
            if (m.Msg == 0x0219){//设备改变
                if (m.WParam.ToInt32() == 0x8004){//usb串口拔出
                    string[] ports = System.IO.Ports.SerialPort.GetPortNames();//重新获取串口
                    comboBox1.Items.Clear();//清除comboBox里面的数据
                    comboBox1.Items.AddRange(ports);//给comboBox1添加数据
                    if (button1.Text == "关闭串口"){//用户打开过串口
                        if (!serialPort1.IsOpen){//用户打开的串口被关闭:说明热插拔是用户打开的串口
                            button1.Text = "打开串口";
                            serialPort1.Dispose();//释放掉原先的串口资源
                            comboBox1.SelectedIndex = comboBox1.Items.Count > 0 ? 0 : -1;//显示获取的第一个串口号
                        }
                        else{
                            comboBox1.Text = serialPortName;//显示用户打开的那个串口号
                        }
                    }
                    else{//用户没有打开过串口
                        comboBox1.SelectedIndex = comboBox1.Items.Count > 0 ? 0 : -1;//显示获取的第一个串口号
                    }
                }
                else if (m.WParam.ToInt32() == 0x8000){//usb串口连接上
                    string[] ports = System.IO.Ports.SerialPort.GetPortNames();//重新获取串口
                    comboBox1.Items.Clear();
                    comboBox1.Items.AddRange(ports);
                    if (button1.Text == "关闭串口"){//用户打开过一个串口
                        comboBox1.Text = serialPortName;//显示用户打开的那个串口号
                    }
                    else{
                        comboBox1.SelectedIndex = comboBox1.Items.Count > 0 ? 0 : -1;//显示获取的第一个串口号
                    }
                }
            }
            base.WndProc(ref m);
        }

        private void serialPort1_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            int len = serialPort1.BytesToRead;//获取可以读取的字节数
            if ((UsartReadCnt + len) < 1024)//待接收的数据不得超出数组大小
            {
                //把数据读取到UsartReadBuff数组,每次接收数据偏移UsartReadCnt
                serialPort1.Read(UsartReadBuff, UsartReadCnt, len);
                UsartReadCnt = UsartReadCnt + len;
            }
            else
            {
                UsartReadCnt = 0;
            }
        }

        private void button2_Click(object sender, EventArgs e)
        {
            byte[] SendData = new byte[4];
            SendData[0] = 0xaa;
            SendData[1] = 0x55;
            SendData[2] = 0x01;
            SendData[3] = 0x01;
            if (button2.Text == "吸合")
            {
                SendData[3] = 0x01;
            }
            else if (button2.Text == "断开")
            {
                SendData[3] = 0x00;
            }
            try
            {
                serialPort1.Write(SendData, 0, SendData.Length);
            }
            catch (Exception) { }
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            if (UsartReadCnt!=0)//串口接收到数据
            {
                UsartIdleCnt++;//空闲变量累加
                if (UsartIdleCnt > 10)//串口超过10ms没有接收数据
                {
                    UsartIdleCnt = 0;
                    UsartReadCntCopy = UsartReadCnt;
                    UsartReadCnt = 0;

                    if (UsartReadBuff[0] == 0x55 && UsartReadBuff[1] == 0xaa)
                    {
                        if (UsartReadBuff[2] == 0x01)
                        {
                            if (UsartReadBuff[3] == 0x01)//继电器吸合
                            {
                                Invoke((new Action(() => {//C# 3.0以后代替委托的新方法
                                    label8.Text = "吸合";
                                    button2.Text = "断开";
                                })));
                            }
                            else if (UsartReadBuff[3] == 0x00)//继电器断开
                            {
                                label8.Text = "断开";
                                button2.Text = "吸合";
                            }
                        }
                    }
                }
            }
        }
    }
}


