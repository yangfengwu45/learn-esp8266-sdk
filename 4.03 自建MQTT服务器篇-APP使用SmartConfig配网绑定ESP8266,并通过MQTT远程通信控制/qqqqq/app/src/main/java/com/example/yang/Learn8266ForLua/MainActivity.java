package com.example.yang.Learn8266ForLua;

import android.Manifest;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.provider.Settings;
import android.support.annotation.NonNull;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.Toast;
import java.util.ArrayList;

import ru.alexbykov.nopermission.PermissionHelper;

public class MainActivity extends AppCompatActivity {
    private String TAG = MainActivity.class.getSimpleName().toString();
    public static String TelephonyIMEI = "";//获取手机IMEI
    MyHandler mHandler;
    ListView ListView1;//使用ListView显示多个设备
    static ArrayList<String> ArrayListClientId = new ArrayList<String>();//存储ListView的数据
    ArrayAdapter<String> adapter;//为ListView添加一个适配器
    boolean ListViewLongClick = false;//是不是长按
    DataBase dataBase;//数据库

    private PermissionHelper permissionHelper;//权限申请


    String[] PermissionString={//需要提醒用户申请的权限
            Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.CAMERA,
            Manifest.permission.ACCESS_FINE_LOCATION,
            Manifest.permission.READ_PHONE_STATE,
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
            Manifest.permission.REQUEST_INSTALL_PACKAGES,
    };
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        ListView1 = findViewById(R.id.ListView1);

        TelephonyIMEI = getTelephonyIMEI(getApplicationContext());//获取手机 IMEI

        dataBase = new DataBase(getApplicationContext(),"DataBase",1);//数据库

        mHandler = new MyHandler();
        permissionHelper = new PermissionHelper(this);//权限申请使用

        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);//右上角菜单
        setSupportActionBar(toolbar);

        //给ListView添加适配器
        adapter = new ArrayAdapter<String>(MainActivity.this,android.R.layout.simple_list_item_1,ArrayListClientId);
        ListView1.setAdapter(adapter);

        //ListView 点击
        ListView1.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                if (!ListViewLongClick){
                    Intent intent = new Intent(MainActivity.this,
                            DeviceControl.class);
                    intent.putExtra("DeviceId",ArrayListClientId.get(position));//获取点击设备的ClientId
                    startActivity(intent);//跳转到控制页面
                }
                ListViewLongClick = false;
            }
        });

        //ListView 长按
        ListView1.setOnItemLongClickListener(new AdapterView.OnItemLongClickListener() {
            @Override
            public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {
                ListViewLongClick = true;
                showNormalDialog(position);//长按显示对话框
                return false;
            }
        });


        //使用任务取出数据库里保存的数据
        new Thread(new Runnable() {
            @Override
            public void run() {
                Cursor cursor =  dataBase.query(DataBase.DeviceClientID);//获取数据库保存的ClientID
                if (cursor!=null){
                    while (cursor.moveToNext()){//获取全部
                        String DeviceId = cursor.getString(cursor.getColumnIndex(DataBase.DeviceClientID));//获取
                        if (ArrayListClientId.indexOf(DeviceId)==-1){
                            ArrayListClientId.add(DeviceId);//添加到ListView
                        }
                    }
                }
                Message msg = mHandler.obtainMessage();//取出来了所有的数据以后,刷新
                msg.what = 1;
                mHandler.sendMessage(msg);
            }
        }).start();

        MyMqttClient.sharedCenter().setConnect();//连接MQTT
    }

    //使用 Handler
    class MyHandler extends Handler {
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            if (msg.what == 1){
                adapter.notifyDataSetChanged();//刷新显示页面
            }
        }
    }

    /**
     * 设置了MainActivity是singleTask,所以用onNewIntent来接收跳转数据
     * 绑定成功,跳转到主页面会进入此函数处理绑定的数据
     */
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        try {
            String DeviceId  = intent.getStringExtra("data");//获取数据
            Log.e(TAG, "onNewIntent: "+DeviceId );
            if (DeviceId!=null){
                if (ArrayListClientId.indexOf(DeviceId)==-1){//没有这个设备
                    dataBase.insert(DeviceId);//插入数据库
                    ArrayListClientId.add(DeviceId);//添加到ListView
                    adapter.notifyDataSetChanged();//刷新显示
                }else {
                    Toast.makeText(MainActivity.this,"设备已存在:"+DeviceId,Toast.LENGTH_SHORT).show();
                }
            }
        }
        catch (Exception e) {
            Log.e(TAG, "onNewIntent: "+e.toString() );
        }
    }


    private void showNormalDialog(final int index){
        final AlertDialog.Builder normalDialog =
                new AlertDialog.Builder(MainActivity.this);
        normalDialog.setTitle("删除设备");
        normalDialog.setMessage("确定要删除设备吗？");
        normalDialog.setPositiveButton("删除", new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                dataBase.delete(ArrayListClientId.get(index));//数据库删除选中的ClientId
                ArrayListClientId.remove(index);//从ListView删除
                adapter.notifyDataSetChanged();
            }
        });
        normalDialog.setNegativeButton("取消",
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {

                    }
                });
        // 显示
        normalDialog.show();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int id = item.getItemId();
        if (id == R.id.action_settings) {//添加设备
            Intent intent = new Intent(MainActivity.this,
                    ActivityBindDevices.class);
            startActivity(intent);
            return true;
        }
        return super.onOptionsItemSelected(item);
    }


    /*获取手机IMEI号*/
    private String getTelephonyIMEI(Context context) {
        String id = "IMEI";
        //android.telephony.TelephonyManager
        TelephonyManager mTelephony = (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);
        if (ContextCompat.checkSelfPermission(context, Manifest.permission.READ_PHONE_STATE) == PackageManager.PERMISSION_GRANTED)
        {
            if (mTelephony.getDeviceId() != null)
            {
                id = mTelephony.getDeviceId();
            }
        }
        else
        {
            //android.provider.Settings;
            id = Settings.Secure.getString(context.getApplicationContext().getContentResolver(), Settings.Secure.ANDROID_ID);
        }
        return id;
    }


    /** 当活动不再可见时调用 */
    @Override
    protected void onStop()
    {
        super.onStop();
    }


    /** 当活动即将可见时调用 */
    @Override
    protected void onStart()
    {
        super.onStart();
        checkPermission();
    }

    private void checkPermission()
    {
        permissionHelper.check(PermissionString).onSuccess(new Runnable() {
            @Override
            public void run() {
                Log.e(TAG, "run: 允许" );
            }
        }).onDenied(new Runnable() {
            @Override
            public void run() {
//                permissionHelper.startApplicationSettingsActivity();
                Log.e(TAG, "run: 权限被拒绝" );
            }
        }).onNeverAskAgain(new Runnable() {
            @Override
            public void run() {
                Log.e(TAG, "run: 权限被拒绝，下次不会在询问了" );
            }
        }).run();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        permissionHelper.onRequestPermissionsResult(requestCode, permissions, grantResults);
    }

}
