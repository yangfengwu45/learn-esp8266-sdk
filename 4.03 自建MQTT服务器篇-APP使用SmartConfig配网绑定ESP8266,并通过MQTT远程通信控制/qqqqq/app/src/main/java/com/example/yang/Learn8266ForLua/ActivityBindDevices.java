package com.example.yang.Learn8266ForLua;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.location.LocationManager;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

import com.espressif.iot.esptouch.EsptouchTask;
import com.espressif.iot.esptouch.IEsptouchListener;
import com.espressif.iot.esptouch.IEsptouchResult;
import com.espressif.iot.esptouch.IEsptouchTask;
import com.espressif.iot.esptouch.util.ByteUtil;
import com.espressif.iot.esptouch.util.TouchNetUtil;

import java.lang.ref.WeakReference;
import java.util.List;

public class ActivityBindDevices extends AppCompatActivity {
    private static String TAG = MainActivity.class.getSimpleName().toString();
    EditText editTextSSID,editTextPassward;
    Button buttonAdd;

    private boolean mReceiverRegistered = false;
    String BssidStr="";//存储路由器的MAC地址
    private EsptouchAsyncTask4 mTask;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_smartconfig);

        editTextSSID = findViewById(R.id.editText3);//WI-Fi名称
        editTextPassward = findViewById(R.id.editText4);//密码
        buttonAdd = findViewById(R.id.button);//按钮
        buttonAdd.setTag("OK");//设置按钮标识
        buttonAdd.setOnClickListener(new View.OnClickListener() {//按钮点击
            @Override
            public void onClick(View v) {
                if (buttonAdd.getTag().equals("OK"))
                {
                    byte[] ssid = editTextSSID.getTag() == null ? ByteUtil.getBytesByString(editTextSSID.getText().toString()) : (byte[]) editTextSSID.getTag();
                    byte[] password = ByteUtil.getBytesByString(editTextPassward.getText().toString());
                    byte[] bssid = TouchNetUtil.parseBssid2bytes(BssidStr);
                    byte[] deviceCount = "1".getBytes();
                    byte[] broadcast = {1};//

                    if (mTask != null) {
                        mTask.cancelEsptouch();
                    }
                    mTask = new EsptouchAsyncTask4(ActivityBindDevices.this);
                    mTask.execute(ssid, bssid, password, deviceCount, broadcast);
                }
                else if(buttonAdd.getTag().equals("5G"))
                {
                    new AlertDialog.Builder(ActivityBindDevices.this)
                            .setMessage("暂不支持5G信号绑定!")
                            .setNegativeButton("确认", new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) { }})
                            .show();
                }
                else if(buttonAdd.getTag().equals("err"))
                {
                    new AlertDialog.Builder(ActivityBindDevices.this)
                            .setMessage("请先用手机连接Wi-Fi")
                            .setNegativeButton("取消", new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) { }})
                            .setPositiveButton("前往设置", new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    startActivity(new Intent(Settings.ACTION_WIFI_SETTINGS)); //直接进入手机中的wifi网络设置界面
                                }
                            })
                            .show();
                }
                else if(buttonAdd.getTag().equals("GPS"))
                {
                    new AlertDialog.Builder(ActivityBindDevices.this)
                            .setMessage("1.无法获取路由器名称 \n" +
                                    "2.尝试打开GPS以后获取")
                            .setNegativeButton("取消", new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) { }})
                            .setPositiveButton("前往打开", new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    Intent intent = new Intent(android.provider.Settings.ACTION_LOCATION_SOURCE_SETTINGS);
                                    startActivityForResult(intent, 0);
                                }
                            })
                            .show();
                }
            }
        });
        registerBroadcastReceiver();//注册广播接收事件
    }
    /**
     * 状态改变广播接收函数
     */
    private BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action == null) {
                return;
            }
            WifiManager wifiManager = (WifiManager) context.getApplicationContext()
                    .getSystemService(WIFI_SERVICE);//WI-Fi状态
            assert wifiManager != null;

            switch (action) {
                case WifiManager.NETWORK_STATE_CHANGED_ACTION://WI-Fi状态
                case LocationManager.PROVIDERS_CHANGED_ACTION://
                    onWifiChanged(wifiManager.getConnectionInfo());
                    break;
            }
        }
    };

    private void onWifiChanged(WifiInfo info) {
        boolean disconnected = info == null || info.getNetworkId() == -1 || "<unknown ssid>".equals(info.getSSID());
        if (disconnected) {
            editTextSSID.setText("");//Wi-Fi名称设置为null
            editTextSSID.setTag(null);//Wi-Fi名称控件标识设置为null
            buttonAdd.setTag("err");//设置按钮的标识
            if (isSDKAtLeastP()) {//检测API版本 9.0 及其以上
                checkLocation(); }//检测是不是GPS没有打开
        } else {//一切OK,显示路由器名称
            String ssid = info.getSSID();
            if (ssid.startsWith("\"") && ssid.endsWith("\"")) {
                ssid = ssid.substring(1, ssid.length() - 1);
            }
            editTextSSID.setText(ssid);
            editTextSSID.setTag(ByteUtil.getBytesByString(ssid));
            byte[] ssidOriginalData = TouchNetUtil.getOriginalSsidBytes(info);
            editTextSSID.setTag(ssidOriginalData);
            BssidStr = info.getBSSID();
            buttonAdd.setTag("OK");
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                int frequency = info.getFrequency();
                if (frequency > 4900 && frequency < 5900) {
                    // Connected 5G wifi. Device does not support 5G
                    buttonAdd.setTag("5G");
                }
            }
        }
    }

    /**
     * 检测GPS是否打开
     */
    private void checkLocation() {
        boolean enable;
        LocationManager locationManager = (LocationManager) getSystemService(Context.LOCATION_SERVICE);
        enable = locationManager != null && locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER);
        if (!enable) {//没有打开GPS
            buttonAdd.setTag("GPS");//设置按钮的标识
            Log.e(TAG, "连接了Wi-Fi,9.0以上需要打开GPS");
        }
    }

    private boolean isSDKAtLeastP() {
        return Build.VERSION.SDK_INT >= 28;
    }
    private void registerBroadcastReceiver() {
        IntentFilter filter = new IntentFilter(WifiManager.NETWORK_STATE_CHANGED_ACTION);//接收网络改变广播
        if (isSDKAtLeastP()) {
            filter.addAction(LocationManager.PROVIDERS_CHANGED_ACTION);//此事件可以获取到是不是GPS开关状态改变
        }
        registerReceiver(mReceiver, filter);//注册
        mReceiverRegistered = true;
    }



    private static class EsptouchAsyncTask4 extends AsyncTask<byte[], IEsptouchResult, List<IEsptouchResult>> {
        private WeakReference<ActivityBindDevices> mActivity;

        private final Object mLock = new Object();
        private ProgressDialog mProgressDialog;
        private AlertDialog mResultDialog;
        private IEsptouchTask mEsptouchTask;

        EsptouchAsyncTask4(ActivityBindDevices activity) {
            mActivity = new WeakReference<>(activity);
        }
        void cancelEsptouch() {
            cancel(true);
            if (mProgressDialog != null) {
                mProgressDialog.dismiss();
            }
            if (mResultDialog != null) {
                mResultDialog.dismiss();
            }
            if (mEsptouchTask != null) {
                mEsptouchTask.interrupt();
            }
        }
        @Override
        protected void onPreExecute() {
            Activity activity = mActivity.get();
            mProgressDialog = new ProgressDialog(activity);
            mProgressDialog.setMessage("正在搜索,请等待...");
            mProgressDialog.setCanceledOnTouchOutside(false);
            mProgressDialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
                @Override
                public void onCancel(DialogInterface dialog) {
                    synchronized (mLock) {
                        if (mEsptouchTask != null) {
                            mEsptouchTask.interrupt();
                        }
                    }
                }
            });
            mProgressDialog.setButton(DialogInterface.BUTTON_NEGATIVE, "取消", new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialogInterface, int i) {
                    synchronized (mLock) {
                        if (mEsptouchTask != null) {
                            mEsptouchTask.interrupt();
                        }
                    }
                }
            });
            mProgressDialog.show();
        }

        @Override
        protected void onProgressUpdate(IEsptouchResult... values) {
            Context context = mActivity.get();
            if (context != null) {
                IEsptouchResult result = values[0];
//                Log.i(TAG, "EspTouchResult: " + result);
                String text = result.getBssid() + " is connected to the wifi";
//                Toast.makeText(context, text, Toast.LENGTH_SHORT).show();
            }
        }

        @Override
        protected List<IEsptouchResult> doInBackground(byte[]... params) {
            ActivityBindDevices activity = mActivity.get();
            int taskResultCount;
            synchronized (mLock) {
                byte[] apSsid = params[0];
                byte[] apBssid = params[1];
                byte[] apPassword = params[2];
                byte[] deviceCountData = params[3];
                byte[] broadcastData = params[4];
                taskResultCount = deviceCountData.length == 0 ? -1 : Integer.parseInt(new String(deviceCountData));
                Context context = activity.getApplicationContext();
                mEsptouchTask = new EsptouchTask(apSsid, apBssid, apPassword, context);
                mEsptouchTask.setPackageBroadcast(broadcastData[0] == 1);
                mEsptouchTask.setEsptouchListener(new IEsptouchListener() {
                    @Override
                    public void onEsptouchResultAdded(IEsptouchResult result) {
                        publishProgress(result);
                    }
                });
            }
            return mEsptouchTask.executeForResults(taskResultCount);
        }

        @Override
        protected void onPostExecute(List<IEsptouchResult> result) {
            ActivityBindDevices activity = mActivity.get();
            activity.mTask = null;
            mProgressDialog.dismiss();
            if (result == null) {
                mResultDialog = new AlertDialog.Builder(activity)
                        .setMessage("启动失败,请重试!")
                        .setPositiveButton(android.R.string.ok, null)
                        .show();
                mResultDialog.setCanceledOnTouchOutside(false);
                return;
            }

            // check whether the task is cancelled and no results received
            IEsptouchResult firstResult = result.get(0);
            if (firstResult.isCancelled()) {
                return;
            }
            // the task received some results including cancelled while
            // executing before receiving enough results

            if (!firstResult.isSuc()) {
                mResultDialog = new AlertDialog.Builder(activity)
                        .setMessage("绑定失败!")
                        .setPositiveButton(android.R.string.ok, null)
                        .show();
                mResultDialog.setCanceledOnTouchOutside(true);
                return;
            }
            try
            {
                Log.e(TAG, "onPostExecute: "+result.get(0).getBssid() );
                Log.e(TAG, "onPostExecute: "+result.get(0).getInetAddress().getHostAddress() );

                String macAddress = result.get(0).getBssid();//获取Wi-Fi的MAC
                macAddress = macAddress.replaceAll(".{2}(?!$)","$0:");//每隔两个字符加 : 号

                Intent intent = new Intent(activity, MainActivity.class);
                intent.putExtra("data",macAddress);
                activity.startActivity(intent);
                activity.finish();
            }
            catch (Exception e)
            {
                Log.e(TAG, "onPostExecute: " +e.toString());
            }
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Log.e(TAG, "onDestroy: " );
        if (mReceiverRegistered) {
            unregisterReceiver(mReceiver);//取消注册
        }
    }
}
