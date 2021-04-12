package com.example.yang.Learn8266ForLua;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;

public class DataBase extends SQLiteOpenHelper {

	private final static String TABLE_NAME = "DeviceTopic";//
	private final static String ID = "id"; //
	private SQLiteDatabase db = null;
	
	public final static String DeviceClientID = "deviceClientID";//

	public DataBase(Context context, String DATABASE_NAME, int DATABASE_VERSION) {
        super(context,DATABASE_NAME, null, DATABASE_VERSION); 
    }
	
	@Override
	public void onCreate(SQLiteDatabase db) {
		// TODO Auto-generated method stub
		String sql = "CREATE TABLE " + TABLE_NAME +
				" (" +
				ID + " INTEGER primary key autoincrement, " + 
				DeviceClientID + " text "+
				")";
				db.execSQL(sql); 
	}

	@Override
	public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
		// TODO Auto-generated method stub
		String sql = "DROP TABLE IF EXISTS " + TABLE_NAME;
		db.execSQL(sql); 
		onCreate(db); 
	}
	
	
	/**
	 * insert data
	 * @param deviceClientID
	 * @return
	 */
	public long insert(String deviceClientID)
	{ 
		db = getWritableDatabase();
		/* ContentValues */ 
		ContentValues cv = new ContentValues();
		cv.put(DeviceClientID, deviceClientID);

		long row = db.insert(TABLE_NAME, null, cv); 
		db.close();
		return row; 
	} 
	
	/**
	 * delete data
	 * @param deviceClientID
	 * @return
	 */
	public long delete(String deviceClientID)
	{ 
		db = getWritableDatabase();
		String where = DeviceClientID + " = ?";
		String[] whereValue ={deviceClientID};
		long row = db.delete(TABLE_NAME, where, whereValue); 
		db.close();
		return row;
	} 

	/**
	 * query data
	 * @param QueryContent
	 * @return
	 */
	public Cursor query(String QueryContent)
	{
		db = getWritableDatabase();
		Cursor cursor = null;
		String str = null;

		str = "select "+QueryContent +" from "+ TABLE_NAME;
		cursor = db.rawQuery(str,null);

		return cursor;
	}
	
}
