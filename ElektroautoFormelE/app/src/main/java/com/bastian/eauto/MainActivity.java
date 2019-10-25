package com.bastian.eauto;

import androidx.appcompat.app.AppCompatActivity;

import android.annotation.SuppressLint;
import android.os.Bundle;
import android.os.Handler;
import android.text.Editable;
import android.util.Log;
import android.view.View;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.EditText;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import com.android.volley.Request;
import com.android.volley.RequestQueue;
import com.android.volley.Response;
import com.android.volley.VolleyError;
import com.android.volley.toolbox.StringRequest;
import com.android.volley.toolbox.Volley;

import java.util.Timer;
import java.util.TimerTask;

import okhttp3.OkHttpClient;
import okhttp3.WebSocket;
import okhttp3.WebSocketListener;
import okio.ByteString;

@SuppressLint("SetTextI18n")
public class MainActivity extends AppCompatActivity {

    public EditText editTextValue;
    public SeekBar seekBarValue;
    public Button buttonArm, buttonDisarm, buttonSet;
    public TextView textViewTelemetry;
    public Spinner spinnerMode;

    public boolean res_armed = false;
    public int res_ctrlMode = 0, res_throttle = 0, res_rps = 0, res_slip = 0, res_velocity1 = 0, res_velocity2 = 0, res_acceleration = 0;

    public TaskHandle autoSend;
    
    public boolean newValue = false;
    public int value = 0;

    private OkHttpClient client;
    WebSocket ws;
    private final class EchoWebSocketListener extends WebSocketListener {
        private static final int NORMAL_CLOSURE_STATUS = 1000;
        @Override
        public void onOpen(WebSocket webSocket, okhttp3.Response response) {
            ws.send("d1");
        }
        @Override
        public void onMessage(WebSocket webSocket, String text) {
            output(text);
        }
        @Override
        public void onMessage(WebSocket webSocket, ByteString bytes) {
            output("Receiving bytes : " + bytes.hex());
        }
        @Override
        public void onClosing(WebSocket webSocket, int code, String reason) {
            webSocket.close(NORMAL_CLOSURE_STATUS, null);
            output("Closing : " + code + " / " + reason);
        }
        @Override
        public void onFailure(WebSocket webSocket, Throwable t, okhttp3.Response response) {
            output("Error : " + t.getMessage());
            start();
        }
    }
    private void output(final String txt) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Log.d("WSCommunication", "received: " + txt.replaceAll("\n", ""));
                String[] response_separated = txt.split("\n");
                for (String s : response_separated) {
                    int val = 0;
                    try {
                        val = Integer.parseInt(s.substring(1, s.length() - 1));
                    } catch (Exception e) {
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
                            Toast.makeText(MainActivity.this, "Da stimmt was mit der Antwort nicht!", Toast.LENGTH_SHORT).show();
                            break;
                    }
                }

                textViewTelemetry.setText("Status: " + (res_armed ? "Armed" : "Disarmed") + "\nModus: " + getResources().getStringArray(R.array.ctrlModeOptions)[res_ctrlMode] +
                        "\nThrottle: " + res_throttle + "\nRPS:" + res_rps + "\nSchlupf: " + res_slip + "%\nGeschwindigkeit (MPU): " + ((float)res_velocity1 / 1000.0) +
                        "m/s\nGeschwindigkeit (Räder): " + ((float)res_velocity2 / 1000.0) + "m/s\nBeschleunigung:" + res_acceleration);
            }
        });
    }
    private void start() {
        okhttp3.Request request = new okhttp3.Request.Builder().url("ws://192.168.0.213").build();
        EchoWebSocketListener listener = new EchoWebSocketListener();
        ws = client.newWebSocket(request, listener);
    }
    private void send(String s){
        ws.send(s);
        Log.d("WSCommunication", "sent: " + s);
    }

    @Override
    protected void onDestroy() {
        client.dispatcher().executorService().shutdown();
        super.onDestroy();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        editTextValue = findViewById(R.id.editTextDuration);
        seekBarValue = findViewById(R.id.seekBarDuration);
        buttonArm = findViewById(R.id.buttonArm);
        buttonDisarm = findViewById(R.id.buttonDisarm);
        buttonSet = findViewById(R.id.buttonSet);
        textViewTelemetry = findViewById(R.id.textViewTelemetry);
        spinnerMode = findViewById(R.id.spinnerMode);

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
        buttonSet.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (res_armed) {
                    setValue(getEditableValue(editTextValue.getText()));
                } else {
                    editTextValue.setText("0");
                    Toast.makeText(MainActivity.this, "Zuerst Arming durchführen!", Toast.LENGTH_SHORT).show();
                }
            }
        });//Setzt die momentan eingetragene Dauer

        seekBarValue.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int i, boolean b) {
                if (b)
                    setValue(i);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                if (!res_armed)
                    Toast.makeText(MainActivity.this, "Zuerst Arming durchführen!", Toast.LENGTH_SHORT).show();
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

            }
        });

        spinnerMode.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> adapterView, View view, int i, long l) {
                if (i < 2){
                    seekBarValue.setMax(1000);
                } else {
                    seekBarValue.setMax(20);
                }
                setValue(seekBarValue.getProgress());
            }

            @Override
            public void onNothingSelected(AdapterView<?> adapterView) {

            }
        });

        start();

        autoSend = setInterval(new Runnable() {
            @Override
            public void run() {
                sendRequest();
            }
        }, 50);
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
        if (!res_armed){
            spinnerMode.setSelection(0);
            seekBarValue.setMax(1000);
            value = 0;
        } else {
            value = Math.max(0, value);
            if (spinnerMode.getSelectedItemPosition() < 2){
                value = Math.min (1000, value);
            } else {
                value = Math.min (20, value);
            }
            newValue = true;
        }
        this.value = value;
        editTextValue.setText("" + value);
        seekBarValue.setProgress(value);
    }

    public void sendArmed(boolean a){
        setValue(0);
        spinnerMode.setSelection(0);
        String text = (a ? "ARM" : "DISARM");
        send(text);
    }

    public void sendRequest(){
        String text = "s=" + (newValue ? 1 : 0);
        if (newValue){
            text = text + "&m=" + spinnerMode.getSelectedItemPosition() + "&v=" + value;
        }
        newValue = false;
        send(text);
    }
}