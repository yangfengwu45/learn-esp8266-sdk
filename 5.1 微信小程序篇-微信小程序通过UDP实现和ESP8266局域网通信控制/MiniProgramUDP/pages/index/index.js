//index.js
//获取应用实例
var util = require("../../utils/util.js");

var udp;
var IPAddress = "192.168.4.1";
//var IPAddress = "192.168.0.104";
var Port = 5556;

const app = getApp()

Page({
  data: {
    temperature: "00",//温度
    humidity: "00",   //湿度
    SwitchOn: "/images/switch_button_on.png",  //开关ON图片
    SwitchOff: "/images/switch_button_off.png",//开关OFF图片
    SwitchOnCmd: "{\"data\":\"switch\",\"bit\":\"1\",\"status\":\"1\"}",    //控制继电器吸合
    SwitchOffCmd: "{\"data\":\"switch\",\"bit\":\"1\",\"status\":\"0\"}",   //控制继电器断开
    SwitchQueryCmd: "{\"data\":\"switch\",\"bit\":\"1\",\"status\":\"-1\"}",//查询继电器状态
    SwitchTag: false,
  },
  onShow: function () {
    let _this = this;
    console.log("ControlonShow");

    //初始化显示温湿度:00,按钮状态:关,设备状态:离线
    this.setData
      ({
        temperaturetext: this.data.temperature,
        humiditytext: this.data.humidity,
        SwitchBackgroundImage: this.data.SwitchOff,
      })

    udp = wx.createUDPSocket()//启用UDP
    udp.bind()


    //UDP接收到消息
    udp.onMessage(function (res) {

      console.log(str)
      let str = util.newAb2Str(res.message);//接收消息
      console.log('str===' + str)


      if (str != null) {
        let json = JSON.parse(str);//解析JSON数据
        if (json != null) {
          if (json.data == "TH")//是温湿度数据
          {
            _this.data.temperature = json.temperature;
            _this.data.humidity = json.humidity;
            if (_this.data.temperature != null && _this.data.humidity != null) {
              _this.setData
                ({
                  temperaturetext: _this.data.temperature,
                  humiditytext: _this.data.humidity,
                })
            }
          }
          else if (json.data == "switch")//接收的是开关数据
          {
            if (json.status == "1")//开关接通
            {
              _this.data.DeviceStatusValue = "在线(继电器吸合)"
              _this.setData
                ({
                  SwitchBackgroundImage: _this.data.SwitchOn,
                })
              _this.data.SwitchTag = true;
            }
            else if (json.status == "0")//开关断开
            {
              _this.data.DeviceStatusValue = "在线(继电器断开)"
              _this.setData
                ({
                  SwitchBackgroundImage: _this.data.SwitchOff,
                })
              _this.data.SwitchTag = false;
            }
          }
        }
      }
    });

  },
  /**
 * 页面控件--控制开关按钮
 */
  Switch: function () {
    if (this.data.SwitchTag == true) //开关状态是开
    {
      this.setData
        ({
          SwitchBackgroundImage: this.data.SwitchOff //设置显示开关OFF图片
        })
      this.data.SwitchTag = false;//记录图片的状态
      udp.send
        ({
          address: IPAddress,
          port: Port,
          message: "{\"data\":\"switch\",\"bit\":\"1\",\"status\":\"0\"}"
        });
      //发送关指令
    }
    else //开关状态是关
    {
      this.setData
        ({
          SwitchBackgroundImage: this.data.SwitchOn //设置显示开关ON图片
        })
      this.data.SwitchTag = true;//记录图片的状态
      udp.send
        ({
          address: IPAddress,
          port: Port,
          message: "{\"data\":\"switch\",\"bit\":\"1\",\"status\":\"1\"}"
        });
      //发送开指令
    }
  },
})
