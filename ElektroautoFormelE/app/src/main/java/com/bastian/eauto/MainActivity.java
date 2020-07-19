package com.bastian.eauto;

import androidx.appcompat.app.AppCompatActivity;

import android.annotation.SuppressLint;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.text.Editable;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.util.Timer;
import java.util.TimerTask;

import okhttp3.OkHttpClient;
import okhttp3.WebSocket;
import okhttp3.WebSocketListener;
import okio.ByteString;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

@SuppressLint("SetTextI18n")
public class MainActivity extends AppCompatActivity {

    private EditText editTextIP, editTextValue;
    private Button buttonArm, buttonDisarm, buttonSet, buttonSoftDisarm, buttonStartRace;
    private TextView textViewTelemetry;
    private ImageButton ibReconnect;
    private Spinner spinnerMode;
    private SeekBar seekBarValue;
    private Switch switchRaceMode;

    private boolean res_armed = false;
    private int res_ctrlMode = 0, res_throttle = 0, res_rps = 0, res_slip = 0, res_velocity1 = 0, res_velocity2 = 0, res_acceleration = 0, res_voltage = 0, res_temp = 0;

    private final int LOG_FRAMES = 5000, JSON_PACKET_SIZE = 100;
    private int[] throttle_log = new int[LOG_FRAMES], rps_log = new int[LOG_FRAMES], voltage_log = new int[LOG_FRAMES], acceleration_log = new int[LOG_FRAMES], temp_log = new int[LOG_FRAMES];

    SharedPreferences mPreferences;
    SharedPreferences.Editor mEditor;

    private MainActivity.TaskHandle autoSend;
    private boolean newValue = false;
    private boolean rmUserChanged = true;
    private int value = 0;
    private static int requestUpdateMS = 40;

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
                if (txt.startsWith("{")){
                    try {
                        JSONObject root = new JSONObject(txt);
                        final int firstPos = root.getInt("firstIndex");
                        Log.d("random", "got packet with firstPos" + firstPos);
                        JSONArray throttleArray = root.getJSONArray("throttle");
                        JSONArray accelerationArray = root.getJSONArray("acceleration");
                        JSONArray rpsArray = root.getJSONArray("rps");
                        JSONArray voltageArray = root.getJSONArray("voltage");
                        JSONArray temperatureArray = root.getJSONArray("temperature");
                        for (int pos = 0; pos < JSON_PACKET_SIZE; pos++){
                            throttle_log[firstPos + pos] = throttleArray.getInt(pos);
                            acceleration_log[firstPos + pos] = accelerationArray.getInt(pos);
                            rps_log[firstPos + pos] = rpsArray.getInt(pos);
                            voltage_log[firstPos + pos] = voltageArray.getInt(pos);
                            temp_log[firstPos + pos] = temperatureArray.getInt(pos);
                        }
                        boolean isFinished = true;
                        for (int i = 0; i < LOG_FRAMES && isFinished; i+=JSON_PACKET_SIZE){
                            if (throttle_log[i] == -1) isFinished = false;
                        }
                        Log.d("random", "isFinished: " + isFinished);
                        if (isFinished){
                            JSONObject output = new JSONObject();
                            JSONArray finalThrottleArray = new JSONArray(throttle_log);
                            JSONArray finalAccelerationArray = new JSONArray(acceleration_log);
                            JSONArray finalRpsArray = new JSONArray(rps_log);
                            JSONArray finalVoltageArray = new JSONArray(voltage_log);
                            JSONArray finalTemperatureArray = new JSONArray(temp_log);
                            output.put("throttle", finalThrottleArray);
                            output.put("acceleration", finalAccelerationArray);
                            output.put("rps", finalRpsArray);
                            output.put("voltage", finalVoltageArray);
                            output.put("temperature", finalTemperatureArray);
                            File jsonOutputFile = new File((MainActivity.this).getExternalFilesDir(null), "output.json");
                            Log.d("random", "file name is " + jsonOutputFile.toString());
                            FileOutputStream fos = new FileOutputStream(jsonOutputFile);
                            OutputStreamWriter osw = new OutputStreamWriter(fos);
                            BufferedWriter bufferedWriter = new BufferedWriter(osw);
                            bufferedWriter.write(output.toString());
                            bufferedWriter.close();
                            Log.d("random", output.toString());
                            throttle_log = new int[LOG_FRAMES];
                            acceleration_log = new int[LOG_FRAMES];
                            rps_log = new int[LOG_FRAMES];
                            voltage_log = new int[LOG_FRAMES];
                            temp_log = new int[LOG_FRAMES];
                            for (int i = 0; i < LOG_FRAMES; i+=JSON_PACKET_SIZE){
                                throttle_log[i] = -1;
                            }
                        }
                    } catch (JSONException | IOException e) {
                        Log.e("random", "error");
                        e.printStackTrace();
                        Log.e("random", e.toString());
                    }
                    return;
                }
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
                        case 'e':
                            if (val > 0 != switchRaceMode.isChecked()){
                                changeRaceModeToggle(val > 0);
                            }
                            break;
                        case 'u':
                            res_voltage = val;
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
                        case 'p':
                            res_temp = val;
                            break;
                        default:
                            Toast.makeText(MainActivity.this, "Da stimmt was mit der Antwort nicht! Unbekanntes Attribut: " + s.charAt(0), Toast.LENGTH_SHORT).show();
                            break;
                    }
                }

                textViewTelemetry.setText("Status: " + (res_armed ? "Armed" : "Disarmed") + "\nModus: " + getResources().getStringArray(R.array.ctrlModeOptions)[res_ctrlMode] +
                        "\nSpannung: " + ((float)res_voltage / 100)  + "V\nThrottle: " + res_throttle + "\nRPS: " + res_rps + "\nSchlupf: " + res_slip + "%\nGeschwindigkeit (MPU): " + ((float)res_velocity1 / 1000.0) +
                        "m/s\nGeschwindigkeit (Räder): " + ((float)res_velocity2 / 1000.0) + "m/s\nBeschleunigung: " + res_acceleration + " rel. Einheiten\nTemperatur: " + res_temp + "°C");
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

    private void wsStart() {
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
        Log.d("random", Environment.getExternalStorageState().toString());

        editTextValue = findViewById(R.id.editTextValue);
        seekBarValue = findViewById(R.id.seekBarDuration);
        buttonArm = findViewById(R.id.buttonArm);
        buttonDisarm = findViewById(R.id.buttonDisarm);
        buttonSoftDisarm = findViewById(R.id.buttonSoftDisarm);
        buttonSet = findViewById(R.id.buttonSet);
        textViewTelemetry = findViewById(R.id.textViewTelemetry);
        spinnerMode = findViewById(R.id.spinnerMode);
        ibReconnect = findViewById(R.id.ibReconnect);
        editTextIP = findViewById(R.id.editTextIP);
        buttonStartRace = findViewById(R.id.buttonStartRace);
        switchRaceMode = findViewById(R.id.switchRaceMode);

        mPreferences = PreferenceManager.getDefaultSharedPreferences(this);
        mEditor = mPreferences.edit();

        editTextIP.setText(mPreferences.getString("IP", "192.168.0.75"));

        for (int i = 0; i < LOG_FRAMES; i+=JSON_PACKET_SIZE){
            throttle_log[i] = -1;
        }

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
        buttonSoftDisarm.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (res_armed) {
                    sendSoftDisarm();
                } else {
                    Toast.makeText(MainActivity.this, "Bereits disarmed", Toast.LENGTH_SHORT).show();
                }
            }
        });//Soft disarm
        buttonSet.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                onSetPressed();
            }
        });//Setzt die momentan eingetragene Dauer
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
        editTextValue.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                onSetPressed();
                return false;
            }
        });

        seekBarValue.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
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
        });

        //switchRaceMode.setClickable(false);

        switchRaceMode.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (rmUserChanged){
                    // transmit rm change to arduino
                    changeRaceModeToggle(!isChecked);
                    if (!res_armed || isChecked)
                        sendRaceMode(isChecked);
                } else {
                    buttonArm.setVisibility(isChecked ? View.GONE : View.VISIBLE);
                    buttonStartRace.setVisibility(isChecked ? View.VISIBLE : View.GONE);
                }
            }
        });

        wsStart();

        autoSend = setInterval(new Runnable() {
            @Override
            public void run() {
                sendRequest();
            }
        }, requestUpdateMS);
    }

    private void onSetPressed() {
        if (res_armed) {
            setValue(getEditableValue(editTextValue.getText()));
        } else {
            editTextValue.setText("0");
            Toast.makeText(MainActivity.this, "Zuerst Arming durchführen!", Toast.LENGTH_SHORT).show();
        }
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

    private void changeRaceModeToggle (boolean state){
        rmUserChanged = false;
        switchRaceMode.setChecked(state);
        rmUserChanged = true;
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
            // spinnerMode.setSelection(0);
            // seekBarValue.setMax(2000);
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
        seekBarValue.setProgress(value);
    }

    public void sendArmed(boolean a){
        setValue(0);
        String text = (a ? "c!a1" : "c!a0");
        send(text);
    }

    public void sendSoftDisarm(){
        setValue(0);
        String text = "c!m1!v0";
        send(text);
        setTimeout(new Runnable() {
            @Override
            public void run() {
                sendArmed(false);
            }
        }, 1000);
    }

    public void sendRaceMode (boolean r){
        String text = (r ? "c!r1" : "c!r0");
        send(text);
    }

    public void sendRequest(){
        if (newValue){
            String text = "c!m" + spinnerMode.getSelectedItemPosition() + "!v" + value;
            send(text);
        }
        newValue = false;
    }
}