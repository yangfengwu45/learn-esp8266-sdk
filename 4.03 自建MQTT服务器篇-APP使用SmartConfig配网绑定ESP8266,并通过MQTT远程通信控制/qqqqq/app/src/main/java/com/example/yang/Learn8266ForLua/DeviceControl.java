package com.example.yang.Learn8266ForLua;

import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.ImageButton;
import android.widget.TextView;

import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.json.JSONObject;

import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class DeviceControl extends AppCompatActivity {
    private String TAG = DeviceControl.class.getSimpleName().toString();
    TextView textView1, textView2, textView5;//显示温湿度数据文本,显示设备状态
    String DeviceId = "";//设备的ID
    ImageButton ImageButton1;//控制开关按钮
    MyHandler mHandler;
    private String ControlSwitchOn = "", ControlSwitchOff = "", QueryStatusData = "";


    //定时器用于查询设备的状态
    private Timer timerQueryStatus = null;
    private TimerTask timerTaskQueryStatus = null;

    //定时器用于轮训订阅主题
    private Timer timerSubscribeTopic = null;
    private TimerTask TimerTaskSubscribeTopic = null;

    private ExecutorService SingleThreadExecutor = Executors.newSingleThreadExecutor();
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.device_control);

        InitJsonData();

        textView1 = findViewById(R.id.textView1);
        textView2 = findViewById(R.id.textView2);
        textView5 = findViewById(R.id.textView5);

        ImageButton1 = findViewById(R.id.ImageButton1);
        ImageButton1.setTag(false);//默认是关闭

        mHandler = new MyHandler();

        /**
         * 获取跳转的数据
         */
        Intent intent1 = getIntent();
        DeviceId = intent1.getStringExtra("DeviceId");//获取跳转过来的数据

        /**
         * 按钮点击事件
         */
        ImageButton1.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if ((boolean) (ImageButton1.getTag()) == false) {
                    ImageButton1.setImageResource(R.mipmap.switch_button_on);
                    ImageButton1.setTag(true);
                    MyMqttClient.sharedCenter().setSendData("user/" + DeviceId, ControlSwitchOn);
                } else {
                    ImageButton1.setImageResource(R.mipmap.switch_button_off);
                    ImageButton1.setTag(false);
                    MyMqttClient.sharedCenter().setSendData("user/" + DeviceId, ControlSwitchOff);
                }
            }
        });

        /**
         * MQTT接收数据
         */
        MyMqttClient.sharedCenter().setOnServerReadStringCallback(new MyMqttClient.OnServerReadStringCallback() {
            @Override
            public void callback(String Topic, MqttMessage Msg, byte[] MsgByte) {
                if (Topic.indexOf(DeviceId) != -1) {//是这个设备的数据
                    Message msg = mHandler.obtainMessage();
                    msg.what = 1;
                    msg.obj = Msg.toString();
                    mHandler.sendMessage(msg);
                }
            }
        });

        /**
         * 连接上服务器
         */
        MyMqttClient.sharedCenter().setOnServerConnectedCallback(new MyMqttClient.OnServerConnectedCallback() {
            @Override
            public void callback() {
                startTimerSubscribeTopic();//定时订阅主题
            }
        });

        /**
         * 订阅主题成功回调
         */
        MyMqttClient.sharedCenter().setOnServerSubscribeCallback(new MyMqttClient.OnServerSubscribeSuccessCallback() {
            @Override
            public void callback(String Topic, int qos) {
                if (Topic.equals("device/"+DeviceId)){
                    stopTimerSubscribeTopic();//订阅到主题,停止订阅
                }
            }
        });
        startTimerSubscribeTopic();//定时订阅主题
        startTimerQueryStatus();//请求设备状态
    }

    /**
     * 封装json数据
     */
    private void InitJsonData(){
        /**
         * 封装json数据:控制继电器吸合
         */
        try {
            JSONObject jsonObject = new JSONObject();
            jsonObject.put("data", "switch");
            jsonObject.put("bit", "1");
            jsonObject.put("status", "1");
            ControlSwitchOn = jsonObject.toString();
        } catch (Exception e) {
        }
        /**
         * 封装json数据:控制继电器断开
         */
        try {
            JSONObject jsonObject = new JSONObject();
            jsonObject.put("data", "switch");
            jsonObject.put("bit", "1");
            jsonObject.put("status", "0");
            ControlSwitchOff = jsonObject.toString();
        } catch (Exception e) {
        }
        /**
         * 封装json数据:查询继电器状态
         */
        try {
            JSONObject jsonObject = new JSONObject();
            jsonObject.put("data", "switch");
            jsonObject.put("bit", "1");
            jsonObject.put("status", "-1");
            QueryStatusData = jsonObject.toString();
        } catch (Exception e) {
        }
    }

    class MyHandler extends Handler {
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            if (msg.what == 1){
                String data = (String) msg.obj;
                Log.e(TAG, "handleMessage: "+data );
                //"{\"data\":\"TH\",\"bit\":\"1\",\"temperature\":\"25\",\"humidity\":\"50\"}"
                try {
                    JSONObject jsonObject = new JSONObject(data);
                    String datatype = jsonObject.getString("data");

                    if (datatype.equals("TH")){//温湿度数据
                        String text = textView5.getText().toString();
                        if (text.equals("离线")){
                            textView5.setText("在线");
                        }

                        String bit = jsonObject.getString("bit");
                        String temperature = jsonObject.getString("temperature");
                        String humidity = jsonObject.getString("humidity");
                        textView1.setText(temperature+"℃");
                        textView2.setText(humidity+"%");
                    }
                    else if (datatype.equals("switch")){//开关数据
                        stopTimerQueryStatus();//停止定时器
                        String bit = jsonObject.getString("bit");
                        String status = jsonObject.getString("status");
                        if (status.equals(new String("1"))){
                            ImageButton1.setImageResource(R.mipmap.switch_button_on);
                            ImageButton1.setTag(true);
                            textView5.setText("在线(继电器吸合)");
                        }else if (status.equals(new String("0"))){
                            ImageButton1.setImageResource(R.mipmap.switch_button_off);
                            ImageButton1.setTag(false);
                            textView5.setText("在线(继电器断开)");
                        }
                    }
                    else if(datatype.equals("status"))
                    {
                        String status = jsonObject.getString("status");
                        if (status.equals("online")){
                            textView5.setText("在线");
                        }
                        else if (status.equals("offline")){
                            startTimerQueryStatus();//启动定时器
                            textView5.setText("离线");
                        }
                    }
                }catch (Exception e){}
            }
        }
    }

    /**
     * 定时器每隔1S尝试订阅主题
     */
    private void startTimerSubscribeTopic(){
        if (timerSubscribeTopic == null) {
            timerSubscribeTopic = new Timer();
        }
        if (TimerTaskSubscribeTopic == null) {
            TimerTaskSubscribeTopic = new TimerTask() {
                @Override
                public void run() {
                    MyMqttClient.sharedCenter().setSubscribe("device/"+DeviceId, 0);//订阅主题
                    Log.e(TAG, "Subscribe: "+ "device/"+DeviceId);
                }
            };
        }
        if(timerSubscribeTopic != null && TimerTaskSubscribeTopic != null )
            timerSubscribeTopic.schedule(TimerTaskSubscribeTopic, 0, 1000);
    }

    private void stopTimerSubscribeTopic(){
        if (timerSubscribeTopic != null) {
            timerSubscribeTopic.cancel();
            timerSubscribeTopic = null;
        }
        if (TimerTaskSubscribeTopic != null) {
            TimerTaskSubscribeTopic.cancel();
            TimerTaskSubscribeTopic = null;
        }
    }



    //定时器每隔3S查询一次设备状态
    private void startTimerQueryStatus(){
        if (timerQueryStatus == null) {
            timerQueryStatus = new Timer();
        }
        if (timerTaskQueryStatus == null) {
            timerTaskQueryStatus = new TimerTask() {
                @Override
                public void run() {
                    MyMqttClient.sharedCenter().setSendData("user/"+DeviceId,QueryStatusData);//请求设备状态
                    Log.e(TAG, "SendData: "+ "device/"+DeviceId);
                }
            };
        }
        if(timerQueryStatus != null && timerTaskQueryStatus != null )
            timerQueryStatus.schedule(timerTaskQueryStatus, 500, 3000);
    }

    private void stopTimerQueryStatus(){
        if (timerQueryStatus != null) {
            timerQueryStatus.cancel();
            timerQueryStatus = null;
        }
        if (timerTaskQueryStatus != null) {
            timerTaskQueryStatus.cancel();
            timerTaskQueryStatus = null;
        }
    }

    //当活动不再可见时调用
    @Override
    protected void onStop()
    {
        super.onStop();
        stopTimerQueryStatus();
        stopTimerSubscribeTopic();
        MyMqttClient.sharedCenter().setUnSubscribe("device/"+DeviceId);
        Log.e(TAG, "onStop: " );
    }

    /**
     * 当处于停止状态的活动需要再次展现给用户的时候，触发该方法
     */
    @Override
    protected void onRestart() {
        super.onRestart();
        startTimerSubscribeTopic();//定时订阅主题
        startTimerQueryStatus();//请求设备状态
        Log.e(TAG, "onRestart: " );
    }

    /**
     * 该方法的触发表示所属活动将被展现给用户
     */
    @Override
    protected void onStart() {
        super.onStart();
        Log.e(TAG, "onStart: " );
    }

    /**
     * 当一个活动和用户发生交互的时候，触发该方法
     */
    @Override
    protected void onResume() {
        super.onResume();
        Log.e(TAG, "onResume: " );
    }

    /**
     * 当一个正在前台运行的活动因为其他的活动需要前台运行而转入后台运行的时候，触发该方法。这时候需要将活动的状态持久化，比如正在编辑的数据库记录等
     */
    @Override
    protected void onPause() {
        super.onPause();
        Log.e(TAG, "onPause: " );
        stopTimerQueryStatus();
        stopTimerSubscribeTopic();
    }
    /**
     * 当活动销毁的时候，触发该方法。
     * 和 onStop 方法一样，如果内存紧张，系统会直接结束这个活动而不会触发该方法
     */
    @Override
    protected void onDestroy() {
        super.onDestroy();
        Log.e(TAG, "onDestroy: " );
    }
}
