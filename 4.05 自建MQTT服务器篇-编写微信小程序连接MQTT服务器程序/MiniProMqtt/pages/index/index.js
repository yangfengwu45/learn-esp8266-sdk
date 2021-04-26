// index.js
// 获取应用实例
var MQTT = require("../../utils/mqtt.js");
var TimeNumber;//循环订阅设备主题定时器
const app = getApp()

Page({
  data: {
  },
  onLoad() {
    //订阅设备发布的主题
    try { clearInterval(TimeNumber); } catch (e) { }
    TimeNumber = setInterval(
      function () {
        /**订阅主题 */
        MQTT.subscribeTopic(
          "1111", //订阅1111
          0,//消息等级        
          function () {
            console.log("订阅成功");
            clearInterval(TimeNumber);//订阅成功结束定时器  
          }, 
          function () {
            console.log("订阅失败");
          }
        );//订阅主题
    }, 1000, "null");//启动定时器,循环订阅主题,直至订阅成功

    /**设置接收消息回调*/
    MQTT.SetonMessageArrivedCallBack(
      function(arg){
        console.log("主题:" + arg.destinationName + " 消息:" + arg.payloadString);


        MQTT.publishTopic(
          "2222", //主题
          "msg=====", //消息
          0, //消息等级
          false, //是否需要服务器保留消息
          function (arg) { //发送成功回调
            console.log("发送数据成功");
          }
        );

      }
    )

  },
})
