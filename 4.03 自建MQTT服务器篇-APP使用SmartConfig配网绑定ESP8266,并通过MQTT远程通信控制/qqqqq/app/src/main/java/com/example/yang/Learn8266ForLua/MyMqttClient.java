package com.example.yang.Learn8266ForLua;

import android.util.Log;


import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.MqttPersistenceException;
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class MyMqttClient {
    private static final String TAG  = MyMqttClient.class.getSimpleName();
    private static MyMqttClient myMqttClient;

//    private static String ClientId = "112233445566|securemode=3,signmethod=hmacsha1|";
//    private static String MqttUserString = "Mqtt&a1m7er1nJbQ";
//    private static String MqttPwdString = "8B286A9E99B49E19A0964589E8F3C2DBB1C1A8DE";
//    private static String MqttIPString = "a1m7er1nJbQ.iot-as-mqtt.cn-shanghai.aliyuncs.com";
//    private static int MqttPort = 1883;
    private static String ClientId = "";
    private static String MqttUserString = "yang";
    private static String MqttPwdString = "11223344";
    private static String MqttIPString = "mnif.cn";
    private static int MqttPort = 1883;



    private static MqttClient mqttClient;
    private static MqttConnectOptions mqttConnectOptions;
    private boolean ConnectFlage = true;

    private OnServerConnectedCallback ConnectedCallback;//连接到服务器
    private OnServerDisConnectedCallback DisConnectedCallback;//与服务器断开连接
    private OnServerReadStringCallback ReadStringCallback;//接收信息回调(字符串)
    private OnServerSubscribeSuccessCallback SubscribeSuccessCallback;//订阅成功

    // 创建一个单任务线程池
    private ExecutorService SingleThreadExecutor = Executors.newSingleThreadExecutor();
    private MyMqttClient() {//构造函数私有化
        super();
    }
    //提供一个全局的静态方法
    public static MyMqttClient sharedCenter() {
        if (myMqttClient == null) {
            Log.e(TAG, "sharedCenter: myMqttClient" );
            synchronized (MyMqttClient.class) {
                if (myMqttClient == null) {
                    myMqttClient = new MyMqttClient();
                    mqttClient = null;
                    //Log.e(TAG, "sharedCenter: new MyMqttClient()" );
                }
            }
        }
        return myMqttClient;
    }


    public void InitMqttOptions(){
        mqttConnectOptions = new MqttConnectOptions();//MQTT的连接设置
        //设置是否清空session,这里如果设置为false表示服务器会保留客户端的连接记录，这里设置为true表示每次连接到服务器都以新的身份连接
        mqttConnectOptions.setCleanSession(true);
        mqttConnectOptions.setUserName(MqttUserString);//设置连接的用户名
        mqttConnectOptions.setPassword(MqttPwdString.toCharArray());//设置连接的密码
        mqttConnectOptions.setConnectionTimeout(10);// 设置连接超时时间 单位为秒
        mqttConnectOptions.setKeepAliveInterval(60);
    }

    /*初始化Mqtt连接*/
    public void InitMqttConnect() {
        try {
            long time=System.currentTimeMillis();
            String Str = time+"";
            Str = Str.substring(Str.length()-4,Str.length());

            if (ClientId.length()>0){
                Str = ClientId;
            }else {
                Str = MainActivity.TelephonyIMEI+Str;
            }
            Log.e(TAG, Str);


            mqttClient = new MqttClient("tcp://"+MqttIPString+":"+MqttPort,Str,new MemoryPersistence());
            mqttClient.setCallback(new MqttCallback() {
                @Override
                public void messageArrived(String arg0, MqttMessage arg1) throws Exception {
                    // TODO Auto-generated method stub
                    byte[] ReadBuffer = arg1.toString().getBytes("UTF-8");
                    if (ReadStringCallback!=null){
                        ReadStringCallback.callback(arg0,arg1,ReadBuffer);
                    }
                }
                @Override
                public void deliveryComplete(IMqttDeliveryToken arg0) {
                }
                @Override
                public void connectionLost(Throwable arg0) {
                    if (DisConnectedCallback!=null)
                        DisConnectedCallback.callback(arg0);
                    try { mqttClient.disconnect();} catch (Exception e) {}
                    try {mqttClient.close();} catch (Exception e) {}
                    mqttClient = null;
                    ConnectFlage = true;
                    setConnect();
                }
            });
        } catch (Exception e) {
        }
    }


    public void setConnect(){
        new Thread(new Runnable() {
            @Override
            public void run() {
                while(ConnectFlage) {
                    try {
                        if (mqttClient == null || !mqttClient.isConnected()) {
                            InitMqttConnect();
                            InitMqttOptions();
                            mqttClient.connect(mqttConnectOptions);
                            if (mqttClient.isConnected()) {
                                if (ConnectedCallback != null)
                                    ConnectedCallback.callback();
                                ConnectFlage = false;
                                Log.e(TAG, "run: Connect Success");
                            }
                        }
                    } catch (Exception e) {
                        if (DisConnectedCallback!=null)
                            DisConnectedCallback.callback(e);
                        try{
                            Thread.sleep(3000);
                        }catch (Exception e1){}
                        Log.e(TAG, e.toString());
                    }
                }
            }
        }).start();
    }


    /**
     * 订阅主题
     * @param Topic  订阅的主题
     * @param qos    消息等级
     */
    public void setSubscribe(final String Topic,final int qos) {
        SingleThreadExecutor.execute(
            new Thread(new Runnable() {
            @Override
            public void run() {
                try{
                    if (mqttClient!=null) {
                        mqttClient.subscribe(Topic,qos);
                        if (SubscribeSuccessCallback !=null){
                            SubscribeSuccessCallback.callback(Topic,qos);
                        }
                    }
                }
                catch (MqttException e){}
            }
        }));

    }


    /**
     * 取消订阅主题
     * @param Topic  取消订阅的主题
     */
    public void setUnSubscribe(final String Topic) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                try{
                    if (mqttClient!=null){
                        mqttClient.unsubscribe(Topic);
                    }
                }
                catch (MqttException e){}
            }
        }).start();
    }

    public void setDisConnect() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                Log.e(TAG, "run: DisConnect");
                if (mqttClient!=null){
                    try { mqttClient.disconnect();} catch (Exception e) {}
                    try {mqttClient.close();} catch (Exception e) {}
                    myMqttClient = null;
                    //Log.e(TAG, "setDisConnect: OK");
                }
            }
        }).start();
    }

    public void setRstConnect()
    {
        new Thread(new Runnable() {
            @Override
            public void run() {
                try { mqttClient.disconnect();} catch (Exception e) {}
                try {mqttClient.close();} catch (Exception e) {}
                mqttClient = null;
                ConnectFlage = true;
                setConnect();
                //Log.e(TAG, "setDisConnect: OK");
            }
        }).start();
    }

    //发送数据
    public void setSendData(final String Topic, final byte[] bytes) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                MqttMessage msgMessage = new MqttMessage(bytes);
                try {
                    if (mqttClient!=null){
                        mqttClient.publish(Topic,msgMessage);
                    }
                } catch (MqttPersistenceException e) {
                } catch (MqttException e) {
                }catch (Exception e) {
                }
            }
        }).start();
    }

    //发送数据
    public void setSendData(final String Topic, final String bytes) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    if (mqttClient!=null) {
                        MqttMessage msgMessage = new MqttMessage(bytes.getBytes());
                        mqttClient.publish(Topic,msgMessage);
                        //Log.e(TAG, "SendData: Topic"+Topic+"\r\n"+"Message"+msgMessage );
                    }
                } catch (MqttPersistenceException e) {
                    Log.e(TAG, e.toString() );
                } catch (MqttException e) {Log.e(TAG, e.toString() );
                }catch (Exception e) {Log.e(TAG, e.toString() );
                }
            }
        }).start();
    }

    public interface OnServerConnectedCallback {//连接上服务器
        void callback();
    }
    public interface OnServerDisConnectedCallback {//和服务器断开
        void callback(Throwable e);
    }
    public interface OnServerReadStringCallback {
        void callback(String Topic, MqttMessage Msg, byte[] MsgByte);
    }

    //订阅主题
    public interface OnServerSubscribeSuccessCallback {
        void callback(String Topic,int qos);
    }

    public void setOnServerSubscribeCallback(OnServerSubscribeSuccessCallback SubscribeSuccessCallback) {
        this.SubscribeSuccessCallback = SubscribeSuccessCallback;
    }


    public void setOnServerConnectedCallback(OnServerConnectedCallback ConnectedCallback) {
        this.ConnectedCallback = ConnectedCallback;
    }

    public void setOnServerDisConnectedCallback(OnServerDisConnectedCallback DisConnectedCallback) {
        this.DisConnectedCallback = DisConnectedCallback;
    }

    public void setOnServerReadStringCallback(OnServerReadStringCallback ReadStringCallback) {
        this.ReadStringCallback = ReadStringCallback;
    }
}