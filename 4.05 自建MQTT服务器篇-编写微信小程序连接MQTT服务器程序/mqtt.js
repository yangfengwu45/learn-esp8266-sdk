var mqtt = require("../utils/mqtt.min.js");


var client = null;

var timeout = null;//定时器
var interval = 100;//定时间隔

var publish_flag = false;
var publish_timeout=0;

var subscribe_flag = false;
var subscribe_timeout=0;

var subscribe_multiple_flag = false
var subscribe_multiple_timeout=0;


var client_state = false;//false:offline;true:online

var onMessageArrivedCallBack;//其它函数注册接收MQTT消息回调
var onConnectionSuccessCallBack;//连接MQTT成功回调
var onConnectionLostCallBack;//连接MQTT失败/掉线回调


var host = 'wxs://mnifdv.cn/mqtt'; //地址

const options = {
  keepalive: 30, //30s
  ssl: true,
  clean: true, //cleanSession不保持持久会话
  protocolVersion: 4,//MQTT v3.1.1
  username: 'yang',  //用户名
  password: '11223344',  //密码
  clientId: (+new Date()) + '' + Math.ceil(Math.random() * 1000000000),
  reconnectPeriod: 4 * 1000,
  connectTimeout: 3 * 1000,
  /**遗嘱 */
  /*
  will: {
    topic: 'test',
    payload: 'test',
    qos: 0,
    retain: false
  },*/
}


/**
* @brief  //连接MQTT成功回调
* @param  
* @param  None
* @param  None
* @retval None
* @example 
**/
var SetonConnectionSuccessCallBack = function SetonConnectionSuccessCallBack(fun) {
  onConnectionSuccessCallBack = fun;
}
/**
* @brief  //连接MQTT失败/掉线回调
* @param  
* @param  None
* @param  None
* @retval None
* @example 
**/
var SetonConnectionLostCallBack = function SetonConnectionLostCallBack(fun) {
  onConnectionLostCallBack = fun;
}



/**
* @brief  //MQTT发送消息,方式1
* @param  topic    发布的主题
* @param  payload  发布的消息
* @param  qos      消息等级
* @param  retained 是否需要服务器保留
* @param  SuccessFun 发送消息成功回调函数 SuccessFun(topic, payload, qos, retained)
* @retval none
* @example publishTopic("topic","ssssssssss",0,0,SuccessFun); 发布的消息为 "ssssssssss"
**/
var publishTopic = function publishTopic(topic, payload, qos, retained, SuccessFun) {
  if (client_state){
    let opts = {};
    opts.qos = qos;
    opts.retain = retained;

    publish_flag = true;

    // console.log("启动publish_timeout");
    // publish_timeout = setTimeout(function () {
    //   if (client_state) {
    //     try { client.reconnect(); } catch (e) { }
    //     client_state = false;
    //   }
    //   console.log("publish_timeout");
    // }, 3000);

    client.publish(topic, payload, opts, function (err,err1) {
      console.log("发布消息:" + topic +" "+ payload);
      publish_flag = false;
      console.log("清除publish_timeout" + err + " " + err1);



      if (SuccessFun !=null){
        SuccessFun(topic, payload, qos, retained);
      }
    });
  }
}


/**
* @brief  //MQTT接收的数据函数回调
* @param  
* @param  None
* @param  None
* @retval None
* @example 
**/
var SetonMessageArrivedCallBack = function SetonMessageArrivedCallBack(fun) {
  onMessageArrivedCallBack = fun;
}


/**
* @brief  //订阅主题
* @param  topic      订阅的主题
* @param  q          消息等级
* @param  SuccessFun 订阅成功回调函数 SuccessFun(e)
* @param  FailureFun 订阅失败回调函数 FailureFun(e)
* @retval none
* @example subscribeTopic("1111",0,SuccessFun,FailureFun);
**/
var subscribeTopic = function subscribeTopic(topic, q, SuccessFun, FailureFun) {
  if (client_state) {

    subscribe_flag = true;

    // subscribe_timeout = setTimeout(function () {
    //   if (client_state){
    //     try { client.reconnect(); } catch (e) { }
    //     client_state = false;
    //   }
    //   if (FailureFun != null) FailureFun({ topic: topic, qos: q });
    //   console.log("subscribe_timeout");
    // }, 3000);

    //解决重复订阅返回订阅失败
    client.unsubscribe(topic, function (err, granted) {
      //console.log("取消订阅:" + err, granted);
    });

    client.subscribe(topic, { qos: q }, function (err, granted) {
      try {
        console.log("订阅:" + err, granted, granted.length);

        subscribe_flag = false;

        // clearTimeout(subscribe_timeout);

        if (granted.length != 0) {
          if (SuccessFun != null) SuccessFun(granted[0]);
        } else {
          if (FailureFun != null) FailureFun({ topic: topic, qos: q });
        }
      } catch (e) { 
        
      }
    });
  }
}


/**
* @brief  //订阅主题
* @param  filter      { 'topic': { qos: 0 }, 'topic2': { qos: 1 } }
* @param  none
* @param  SuccessFun 订阅成功回调函数 SuccessFun(e)
* @param  FailureFun 订阅失败回调函数 FailureFun(e)
* @retval none
* @example
**/
var subscribeTopicMultiple = function subscribeTopicMultiple(filter, SuccessFun, FailureFun) {

  if (client_state) {
    // subscribe_multiple_timeout = setTimeout(function () {
    //   if (client_state) {
    //     try { client.reconnect(); } catch (e) { }
    //     client_state = false;
    //   }
    //   if (FailureFun != null) FailureFun(filter);
    //   console.log("subscribe_multiple_timeout");
    // }, 200);

    let i = 0;
    let topic = [];
    Object.keys(filter).forEach(function (key) {
      //console.log(filter, filter[key]);
      topic[i] = filter[key];
      i++;
    });


    //解决重复订阅返回订阅失败
    client.unsubscribe(topic, function (err) {
      //console.log("取消多重订阅:" + err);
    });

    subscribe_multiple_flag = true;

    client.subscribe(filter, function (err, granted) {
      try {
        console.log("多重订阅:" + err, granted, granted.length);

        subscribe_multiple_flag = false;

        // clearTimeout(subscribe_multiple_timeout);

        if (granted.length != 0) {
          if (SuccessFun != null) SuccessFun(filter);
        } else {
          if (FailureFun != null) FailureFun(filter);
        }
      } catch (e) { }
    });
  }
}


/**
* @brief  //取消订阅主题
* @param  topic  string  or  ['topic1','topic2',]
* @param  
* @param  SuccessFun 订阅成功回调函数 SuccessFun(e)
* @retval none
* @example unSubscribeTopic("1111",SuccessFun);
**/
var unSubscribeTopic = function unSubscribeTopic(topic, SuccessFun) {
  
  if (client_state) {
    client.unsubscribe(topic, function (err) {
      if (SuccessFun != null){
        SuccessFun();
      }
      console.log("取消订阅:" + topic);
    });
  }
}


/**
* @brief  //定时器回调函数,轮训查询是否通信超时
* @param  none
* @param  none
* @param  none
* @retval none
* @example 
**/
function timeout_function(param) {
  if (publish_flag){
    publish_timeout = publish_timeout+1;
    if (publish_timeout >= 30){
      publish_timeout = 0;
      
      publish_flag = false;
      subscribe_flag = false;
      subscribe_multiple_flag = false;

      if (client_state) {
        try { client.reconnect(); } catch (e) { }
        client_state = false;
      }
    }
  } 
  else {
    publish_timeout = 0;
  }


  if (subscribe_flag) {
    subscribe_timeout = subscribe_timeout+1;
    if (subscribe_timeout>30){
      subscribe_timeout=0;

      publish_flag = false;
      subscribe_flag = false;
      subscribe_multiple_flag = false;

      if (client_state) {
        try { client.reconnect(); } catch (e) { }
        client_state = false;
      }
    }
  }
  else{
    subscribe_timeout=0;
  }


  if (subscribe_multiple_flag){
    subscribe_multiple_timeout = subscribe_multiple_timeout+1;
    if (subscribe_multiple_timeout>30){
      subscribe_multiple_timeout = 0;

      publish_flag = false;
      subscribe_flag = false;
      subscribe_multiple_flag = false;

      if (client_state) {
        try { client.reconnect(); } catch (e) { }
        client_state = false;
      }
    }
  }
  else{
    subscribe_multiple_timeout=0;
  }
} 

/**
* @brief  控制连接MQTT函数
* @param  
* @param  None
* @param  None
* @retval None
* @example 
**/
var ConnectMqtt = function ConnectMqtt() {//链接MQTT
  console.log(options);
  try { client.end(); } catch (e) { }

  client = mqtt.connect(host, options)
  client.on('connect', function () {
    client_state = true;
    console.log("connect");

    if (timeout != null)
    {
      clearInterval(timeout);
    }
    
    timeout = setInterval(timeout_function, interval, null);

    if (onConnectionSuccessCallBack != null) {//如果回调函数不是空
      onConnectionSuccessCallBack();//执行回调函数
    }
  });

  client.on('message', function (topic, message) {

    let args = {};
    args.destinationName = topic;
    args.payloadString = message;
    console.log(args);
    if (onMessageArrivedCallBack != null)//如果回调函数不是空
    {
      onMessageArrivedCallBack(args);//执行回调函数
    }
  })

  client.on('close', function () {
    console.log("close");
    if (onConnectionLostCallBack != null) //如果回调函数不是空
    {
      onConnectionLostCallBack("close");//执行回调函数
    }
  });

  client.on('disconnect', function () {
    console.log("disconnect");
    if (onConnectionLostCallBack != null) //如果回调函数不是空
    {
      onConnectionLostCallBack("disconnect");//执行回调函数
    }
  });

  client.on('reconnect', function () {
    console.log("reconnect");
    if (onConnectionLostCallBack != null) //如果回调函数不是空
    {
      onConnectionLostCallBack("reconnect");//执行回调函数
    }
  });

  client.on('offline', function () {
    console.log("offline");
    if (onConnectionLostCallBack != null) //如果回调函数不是空
    {
      onConnectionLostCallBack("offline");//执行回调函数
    }
  });

  client.on('error', function () {
    console.log("error");
    if (onConnectionLostCallBack != null) //如果回调函数不是空
    {
      onConnectionLostCallBack("error");//执行回调函数
    }
  });

}

/**
* @brief  启动网络状态监听(网络改变控制重连)
* @param  
* @param  None
* @param  None
* @retval None
* @example 
**/
wx.onNetworkStatusChange(function (res) {
  ConnectMqtt();
  if (res.networkType == "none") console.log("无网络");
  else console.log("网络类型:" + res.networkType);
})


module.exports = {
  ConnectMqtt: ConnectMqtt,//控制连接MQTT
  SetonConnectionSuccessCallBack: SetonConnectionSuccessCallBack,//连接上回调
  SetonConnectionLostCallBack: SetonConnectionLostCallBack,//连接失败/掉线回调
  SetonMessageArrivedCallBack: SetonMessageArrivedCallBack,//接收到消息回调
  publishTopic: publishTopic,//发布消息方式1
  subscribeTopic: subscribeTopic,//订阅主题
  unSubscribeTopic: unSubscribeTopic,
  subscribeTopicMultiple: subscribeTopicMultiple,//多重订阅主题
}