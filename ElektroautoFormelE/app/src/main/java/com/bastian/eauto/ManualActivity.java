package com.bastian.eauto;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.widget.Button;
import android.widget.EditText;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.TextView;

import android.annotation.SuppressLint;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.text.Editable;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import java.util.Timer;
import java.util.TimerTask;

import okhttp3.OkHttpClient;
import okhttp3.WebSocket;
import okhttp3.WebSocketListener;
import okio.ByteString;

public class ManualActivity extends AppCompatActivity {

    private EditText etValue;
    private SeekBar sbValue;
    private Button buttonArm, buttonDisarm, buttonSet;
    private TextView tvTelemetry;
    private Spinner spMode;

    public boolean res_armed = false;
    public int res_ctrlMode = 0, res_throttle = 0, res_rps = 0, res_slip = 0, res_velocity1 = 0, res_velocity2 = 0, res_acceleration = 0;

    private Activity.TaskHandle autoSend;

    public boolean newValue = false;
    public int value = 0;

    private static int requestUpdateMS = 50;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_manual);

    }
}
