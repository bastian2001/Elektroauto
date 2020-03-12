package com.bastian.eauto;

import androidx.appcompat.app.AppCompatActivity;

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

@SuppressLint("SetTextI18n")
public class MainActivity extends AppCompatActivity {

    private EditText editTextIP;
    private Button buttonArm, buttonDisarm;
    private TextView textViewTelemetry;
    private ImageButton ibReconnect;

    private boolean res_armed = false;
    private int res_ctrlMode = 0, res_throttle = 0, res_rps = 0, res_slip = 0, res_velocity1 = 0, res_velocity2 = 0, res_acceleration = 0;

    SharedPreferences mPreferences;
    SharedPreferences.Editor mEditor;

    private OkHttpClient client;
    WebSocket ws;
    private final class EchoWebSocketListener extends WebSocketListener {
        private static final int NORMAL_CLOSURE_STATUS = 1000;
        @Override
        public void onOpen(WebSocket webSocket, okhttp3.Response response) {
            ws.send("s!d1");
        }
        @Override
        public void onMessage(WebSocket webSocket, String text) {
            output(text);
        }
        @Override
        public void onMessage(WebSocket webSocket, ByteString bytes) {
            output(bytes.hex());
        }
        @Override
        public void onClosing(WebSocket webSocket, int code, String reason) {
            webSocket.close(NORMAL_CLOSURE_STATUS, null);
            log("Closing: " + code + " / " + reason);
        }
        @Override
        public void onFailure(WebSocket webSocket, Throwable t, okhttp3.Response response) {
            log("Error: " + t.getMessage());
        }
    }
    private void log(final String txt) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Log.d("WSCommunication", txt);
                textViewTelemetry.setText(txt);
            }
        });
    }
    private void output(final String txt) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Log.d("WSCommunication", "received: " + txt);
                String[] response_separated = txt.split("!");
                for (String s : response_separated) {
                    int val = 0;
                    try {
                        val = Integer.parseInt(s.substring(1));
                    } catch (NumberFormatException e) {
                        Toast.makeText(MainActivity.this, "Da ist etwas mit der Antwort falsch! NFE", Toast.LENGTH_SHORT).show();
                    }
                    switch (s.charAt(0)) {
                        case 'a':
                            res_armed = val > 0;
                            break;
                        case 'm':
                            res_ctrlMode = val;
                            break;
                        case 't':
                            res_throttle = val;
                            break;
                        case 'r':
                            res_rps = val;
                            break;
                        case 's':
                            res_slip = val;
                            break;
                        case 'v':
                            res_velocity1 = val;
                            break;
                        case 'w':
                            res_velocity2 = val;
                            break;
                        case 'c':
                            res_acceleration = val;
                            break;
                        default:
                            Toast.makeText(MainActivity.this, "Da stimmt was mit der Antwort nicht! Unbekanntes Attribut", Toast.LENGTH_SHORT).show();
                            break;
                    }
                }

                textViewTelemetry.setText("Status: " + (res_armed ? "Armed" : "Disarmed") + "\nModus: " + getResources().getStringArray(R.array.ctrlModeOptions)[res_ctrlMode] +
                        "\nThrottle: " + res_throttle + "\nRPS: " + res_rps + "\nSchlupf: " + res_slip + "%\nGeschwindigkeit (MPU): " + ((float)res_velocity1 / 1000.0) +
                        "m/s\nGeschwindigkeit (Räder): " + ((float)res_velocity2 / 1000.0) + "m/s\nBeschleunigung: " + res_acceleration);
            }
        });
    }

    @Override
    protected void onResume() {
        sendTelemetry(true);
        super.onResume();
    }

    @Override
    protected void onPause() {
        sendTelemetry(false);
        super.onPause();
    }

    private void start() {
        okhttp3.Request request = new okhttp3.Request.Builder().url("ws://" + editTextIP.getText().toString()).build();
        EchoWebSocketListener listener = new EchoWebSocketListener();
        client = new OkHttpClient();
        ws = client.newWebSocket(request, listener);
    }
    private void send(String s){
        ws.send(s);
        Log.d("WSCommunication", "sent: " + s);
    }
    private void sendTelemetry(boolean b){
        send("s!t" + (b ? 1 : 0));
    }

    @Override
    protected void onDestroy() {
        ws.close(1000, "Goodbye !");
        super.onDestroy();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        //editTextValue = findViewById(R.id.editTextDuration);
        //seekBarValue = findViewById(R.id.seekBarDuration);
        buttonArm = findViewById(R.id.buttonArm);
        buttonDisarm = findViewById(R.id.buttonDisarm);
        //buttonSet = findViewById(R.id.buttonSet);
        textViewTelemetry = findViewById(R.id.textViewTelemetry);
        //spinnerMode = findViewById(R.id.spinnerMode);
        ibReconnect = findViewById(R.id.ibReconnect);
        editTextIP = findViewById(R.id.editTextIP);
        ScrollView scroll = (ScrollView)findViewById(R.id.sv1);
        scroll.setOnTouchListener(new OnTouchListener() {
            @SuppressLint("ClickableViewAccessibility")
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if (editTextIP.hasFocus()) {
                    editTextIP.clearFocus();
                }
                ScrollView sv = findViewById(R.id.sv1);
                sv.requestFocus();
                return false;
            }
        });

        mPreferences = PreferenceManager.getDefaultSharedPreferences(this);
        mEditor = mPreferences.edit();

        editTextIP.setText(mPreferences.getString("IP", "192.168.0.75"));

        buttonArm.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if(!res_armed) {
                    sendArmed (true);
                } else {
                    Toast.makeText(MainActivity.this, "Bereits armed", Toast.LENGTH_SHORT).show();
                }
            }
        });//Arm
        buttonDisarm.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (res_armed) {
                    sendArmed (false);
                } else {
                    Toast.makeText(MainActivity.this, "Bereits disarmed", Toast.LENGTH_SHORT).show();
                }
            }
        });//Disarm
        /*buttonSet.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (res_armed) {
                    setValue(getEditableValue(editTextValue.getText()));
                } else {
                    editTextValue.setText("0");
                    Toast.makeText(MainActivity.this, "Zuerst Arming durchführen!", Toast.LENGTH_SHORT).show();
                }
            }
        });//Setzt die momentan eingetragene Dauer*/
        ibReconnect.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                textViewTelemetry.setText("Verbindung wird aufgebaut...");
                ws.close(1000, "Goodbye !");
                start();
                mEditor.putString("IP", editTextIP.getText().toString());
                mEditor.commit();
            }
        }); //reconnect

        /*seekBarValue.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar seekBar, int i, boolean b) {
                if (b)
                    setValue(i);
            }

            @Override public void onStartTrackingTouch(SeekBar seekBar) {
                if (!res_armed)
                    Toast.makeText(MainActivity.this, "Zuerst Arming durchführen!", Toast.LENGTH_SHORT).show();
            }

            @Override public void onStopTrackingTouch(SeekBar seekBar) { }
        });

        spinnerMode.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> adapterView, View view, int i, long l) {
                if (i == 0) {
                    seekBarValue.setMax(2000);
                } else if (i == 1){
                    seekBarValue.setMax(1500);
                } else {
                    seekBarValue.setMax(20);
                }
                setValue(seekBarValue.getProgress());
            }

            @Override
            public void onNothingSelected(AdapterView<?> adapterView) {

            }
        });*/

        start();

        autoSend = setInterval(new Runnable() {
            @Override
            public void run() {
                sendRequest();
            }
        }, requestUpdateMS);
    }

    public int getEditableValue(Editable e){
        int i;
        try {
            i = Integer.parseInt(e.toString());
        } catch (Exception ex) {
            i = 0;
        }
        return i;
    }

    private interface TaskHandle {
        //void invalidate();
    }
    private static TaskHandle setTimeout(final Runnable r, long delay) {
        final Handler h = new Handler();
        h.postDelayed(r, delay);
        return new TaskHandle() {
            public void invalidate() {
                h.removeCallbacks(r);
            }
        };
    }
    public static TaskHandle setInterval(final Runnable r, long interval) {
        final Timer t = new Timer();
        final Handler h = new Handler();
        t.scheduleAtFixedRate(new TimerTask() {
            @Override
            public void run() {
                h.post(r);
            }
        }, interval, interval);
        return new TaskHandle() {
            public void invalidate() {
                t.cancel();
                t.purge();
            }
        };
    }

    public void setValue(int value){
        /*if (!res_armed){
            spinnerMode.setSelection(0);
            seekBarValue.setMax(2000);
            value = 0;
        } else {
            value = Math.max(0, value);
            if (spinnerMode.getSelectedItemPosition() == 0){
                value = Math.min (2000, value);
            } else if (spinnerMode.getSelectedItemPosition() == 1) {
                value = Math.min(1500, value);
            } else {
                value = Math.min (20, value);
            }
            newValue = true;
        }
        this.value = value;
        editTextValue.setText("" + value);
        seekBarValue.setProgress(value);*/
    }

    public void sendArmed(boolean a){
        setValue(0);
        //spinnerMode.setSelection(0);
        String text = (a ? "c!a1" : "c!a0");
        send(text);
    }

    public void sendRequest(){
        /*if (newValue){
            String text = "c!m" + spinnerMode.getSelectedItemPosition() + "!v" + value;
            send(text);
        }*/
        newValue = false;
    }
}