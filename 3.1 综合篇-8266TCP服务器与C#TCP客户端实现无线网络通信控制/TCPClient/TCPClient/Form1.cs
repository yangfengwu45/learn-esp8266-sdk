using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Windows.Forms;

namespace TCPClient
{
    public partial class Form1 : Form
    {
        private Socket MySocket = null;// Socket

        public const int TCPBufferSize = 1460;//缓存的最大数据个数
        public byte[] TCPBuffer = new byte[TCPBufferSize];//缓存数据的数组

        public Form1()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            textBox1.Text = "192.168.1.93";
            textBox2.Text = "8080";
            /*
            IPHostEntry ipHostInfo = Dns.Resolve("host.contoso.com");
            IPAddress ipAddress = ipHostInfo.AddressList[0];
            */
        }


        /// <连接按钮点击事件>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void button1_Click(object sender, EventArgs e)
        {
            if (button1.Text == "连接"){
                //IP地址 和 端口号输入不为空
                if (string.IsNullOrEmpty(textBox1.Text) == false && string.IsNullOrEmpty(textBox2.Text) == false){
                    try{
                        IPAddress ipAddress = IPAddress.Parse(textBox1.Text);//获取IP地址
                        int Port = Convert.ToInt32(textBox2.Text);          //获取端口号
                        MySocket = new Socket(AddressFamily.InterNetwork,SocketType.Stream, ProtocolType.Tcp);
                        //使用 BeginConnect 异步连接
                        MySocket.BeginConnect(ipAddress, Port, new AsyncCallback(ConnectedCallback), MySocket);
                    }
                    catch (Exception){
                        MessageBox.Show("IP地址或端口号错误!", "提示");
                    }
                }
                else{
                    MessageBox.Show("IP地址或端口号错误!", "提示");
                }
            }
            else
            {
                try{
                    button1.Text = "连接";
                    MySocket.BeginDisconnect(false,null,null);//断开连接
                }
                catch (Exception){}
            }
        }
        /// <连接异步回调函数>
        /// 
        /// </summary>
        /// <param name="ar"></param>
        void ConnectedCallback(IAsyncResult ar)
        {
            Socket socket = (Socket)ar.AsyncState;//获取Socket
            try{
                socket.EndConnect(ar);
                //设置异步读取数据,接收的数据缓存到TCPBuffer,接收完成跳转ReadCallback函数
                socket.BeginReceive(TCPBuffer, 0, TCPBufferSize, 0,new AsyncCallback(ReadCallback), socket);
                Invoke((new Action(() =>
                {
                    button1.Text = "断开";
                })));
            }
            catch (Exception e){
            }
        }

        /// <接收到数据回调函数>
        /// 
        /// </summary>
        /// <param name="ar"></param>
        void ReadCallback(IAsyncResult ar)
        {
            Socket socket = (Socket)ar.AsyncState;//获取链接的Socket
            int CanReadLen = socket.EndReceive(ar);//结束异步读取回调,获取读取的数据个数

            if (CanReadLen > 0)
            {
                if (TCPBuffer[0] == 0x55 && TCPBuffer[1] == 0xaa)
                {
                    if (TCPBuffer[2] == 0x01)
                    {
                        if (TCPBuffer[3] == 0x01)//继电器吸合
                        {
                            Invoke((new Action(() => {//C# 3.0以后代替委托的新方法
                                label3.Text = "吸合";
                                button2.Text = "断开";
                            })));
                        }
                        else if (TCPBuffer[3] == 0x00)//继电器断开
                        {
                            Invoke((new Action(() => {//C# 3.0以后代替委托的新方法
                                label3.Text = "断开";
                                button2.Text = "吸合";
                            })));
                        }
                    }
                }
                //设置异步读取数据,接收的数据缓存到TCPBuffer,接收完成跳转ReadCallback函数
                socket.BeginReceive(TCPBuffer,0, TCPBufferSize, 0, new AsyncCallback(ReadCallback), socket);
            }
            else//异常
            {
                Invoke((new Action(() => //C# 3.0以后代替委托的新方法
                {
                    button1.Text = "连接";
                })));
                try
                {
                    MySocket.BeginDisconnect(false, null, null);//断开连接
                }
                catch (Exception) { }
            }
        }


        private void button2_Click_1(object sender, EventArgs e)
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
                MySocket.BeginSend(SendData, 0, SendData.Length, 0, null, null); //发送数据
            }
            catch (Exception) { }
        }
    }
}
