package com.bastian.eauto;

import androidx.appcompat.app.AppCompatActivity;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.SharedPreferences;
import android.hardware.input.InputManager;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.telephony.PhoneNumberFormattingTextWatcher;
import android.text.Editable;
import android.util.Log;
import android.util.MonthDisplayHelper;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
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
import java.time.Year;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Objects;
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

    private static int requestUpdateMS = 40;
    private final int LOG_FRAMES = 5000, JSON_PACKET_SIZE = 100;
    SharedPreferences mPreferences;
    SharedPreferences.Editor mEditor;
    WebSocket ws;
    private EditText editTextIP, editTextValue;
    private Button buttonArm, buttonDisarm, buttonSet, buttonSoftDisarm, buttonStartRace;
    private TextView textViewTelemetry;
    private ImageButton ibReconnect, ibPing;
    private Spinner spinnerMode;
    private SeekBar seekBarValue;
    private Switch switchRaceMode;
    private int espMode = 0;
    private int res_throttle = 0, res_rps = 0, res_slip = 0, res_velocity1 = 0, res_velocity2 = 0, res_acceleration = 0, res_voltage = 0, res_temp = 0;
    private int[] throttle_log = new int[LOG_FRAMES], rps_log = new int[LOG_FRAMES], voltage_log = new int[LOG_FRAMES], acceleration_log = new int[LOG_FRAMES], temp_log = new int[LOG_FRAMES];
    private MainActivity.TaskHandle autoSend;
    private boolean newSeekbarValueAvailable = false;
    private int newSeekbarValue = -1;
    private boolean rmUserChanged = true, modeUserChanged = true;
    private OkHttpClient client;
    boolean seekbarTouch = false;
    private long pMillis = 0;
    private static int PING_AMOUNT = 20;
    private int pingCounter = 0;
    private int[] pingArray = new int[PING_AMOUNT];

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

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        editTextValue = findViewById(R.id.editTextValue);
        seekBarValue = findViewById(R.id.seekBarDuration);
        buttonArm = findViewById(R.id.buttonArm);
        buttonDisarm = findViewById(R.id.buttonDisarm);
        buttonSoftDisarm = findViewById(R.id.buttonSoftDisarm);
        buttonSet = findViewById(R.id.buttonSet);
        textViewTelemetry = findViewById(R.id.textViewTelemetry);
        spinnerMode = findViewById(R.id.spinnerMode);
        ibReconnect = findViewById(R.id.ibReconnect);
        ibPing = findViewById(R.id.ibPing);
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
                sendArmed (true);
            }
        });//Arm
        buttonDisarm.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                sendArmed(false);
            }
        });//Disarm
        buttonSoftDisarm.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                sendSoftDisarm();
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
                wsStart();
                mEditor.putString("IP", editTextIP.getText().toString());
                mEditor.commit();
            }
        }); //reconnect
        ibPing.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                pMillis = System.currentTimeMillis();
                pingCounter = 0;
                wsSend("PING");
            }
        });//ping

        editTextValue.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                onSetPressed();
                return false;
            }
        });

        seekBarValue.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar seekBar, int i, boolean b) {
                if (b) {
                    editTextValue.setText("" + i);
                    newSeekbarValueAvailable = true;
                    newSeekbarValue = i;
                }
            }

            @Override public void onStartTrackingTouch(SeekBar seekBar) {
                seekbarTouch = true;
            }

            @Override public void onStopTrackingTouch(SeekBar seekBar) {
                seekbarTouch = false;
            }
        });

        spinnerMode.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> adapterView, View view, int i, long l) {
                int r = (int) (Math.random() * 1000);
                Log.d("random", "listener: " + modeUserChanged);
                if (modeUserChanged) {
                    Log.d("random", "listener: sending mode " + i + ", espmode is " + espMode + ", " + r);
                    changeModeSpinner(espMode);
                    wsSend("MODE:" + i);
                    Log.d("random", "listener: sent mode " + i + " and changed back to " + espMode + ", " + r);
                } else {
                    Log.d("random", "listener: new mode is " + i + ", espmode is " + espMode + ", " + r);
                    if (i <= 1) {
                        seekBarValue.setMax(2000);
                    } else {
                        seekBarValue.setMax(20);
                    }
                    modeUserChanged = true;
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> adapterView) {
            }
        });

        switchRaceMode.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (rmUserChanged){
                    // transmit rm change to arduino
                    changeRaceModeToggle(!isChecked);
                    sendRaceMode(isChecked);
                } else {
                    buttonArm.setVisibility(isChecked ? View.GONE : View.VISIBLE);
                    buttonStartRace.setVisibility(isChecked ? View.VISIBLE : View.GONE);
                }
            }
        });

        buttonStartRace.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                wsSend("STARTRACE");
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

    private void wsLog(final String txt) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Log.d("WSCommunication", txt);
                textViewTelemetry.setText(txt);
            }
        });
    }

    private void changeModeSpinner(int position){
        int r = (int) (Math.random() * 1000);
        Log.d("random", "changing mode to " + position + ", espMode is " + espMode + ", " + r);
        modeUserChanged = false;
        Log.d("random", "set: " + modeUserChanged);
        spinnerMode.setSelection(position, false);
        Log.d("random", r + ", did it!");
    }

    private void output(final String txt) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (txt.startsWith("{")){
                    try {
                        JSONObject root = new JSONObject(txt);
                        final int firstPos = root.getInt("firstIndex");
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
                            String fileName = "log " + Calendar.getInstance().get(Calendar.YEAR) + "-" + prefixZero(Calendar.getInstance().get(Calendar.MONTH), 2) + "-"
                                    + prefixZero(Calendar.getInstance().get(Calendar.DATE), 2) + "-" + prefixZero(Calendar.getInstance().get(Calendar.HOUR_OF_DAY), 2)
                                    + ":" + prefixZero(Calendar.getInstance().get(Calendar.MINUTE), 2) + ":"
                                    + prefixZero(Calendar.getInstance().get(Calendar.SECOND), 2) + ".json";
                            File folder = new File(Environment.getExternalStorageDirectory(), "Formel-E-Logs");
                            if (!folder.exists()){
                                folder.mkdir();
                            }
                            File file = new File(folder, fileName);
                            if (file.exists()){
                                String p = file.getAbsolutePath();
                                int i;
                                for (i = 2; i < 100; i++){
                                    if (!(new File(p + "_" + i).exists())) break;
                                }
                                if (i == 100){
                                    file = null;
                                } else {
                                    file = new File(p + "_" + i);
                                }
                            }
                            if (file != null) {
                                FileOutputStream fos = new FileOutputStream(file);
                                OutputStreamWriter osw = new OutputStreamWriter(fos);
                                BufferedWriter bufferedWriter = new BufferedWriter(osw);
                                bufferedWriter.write(output.toString());
                                bufferedWriter.close();
                                Toast.makeText(MainActivity.this, "Output in Interner Speicher -> Formel-E-Logs -> " + fileName + " gespeichert", Toast.LENGTH_LONG).show();
                            } else {
                                Toast.makeText(MainActivity.this, "Fehler beim Speichern der Datei", Toast.LENGTH_SHORT).show();
                            }

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
                        e.printStackTrace();
                    }
                } else if (txt.startsWith("TELEMETRY")) {
                    String telemetryText = txt.substring(txt.indexOf(" ") + 1);
                    wsLog("received: " + txt);
                    String[] response_separated = telemetryText.split("!");
                    boolean res_armed = false;
                    boolean editTextIsInFocus = editTextValue.hasFocus();
                    int mode = spinnerMode.getSelectedItemPosition();
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
                            case 'u':
                                res_voltage = val;
                                break;
                            case 't':
                                res_throttle = val;
                                if (mode == 0 && !seekbarTouch && !editTextIsInFocus && !telemetryText.contains("o")){
                                    seekBarValue.setProgress(val);
                                    editTextValue.setText("" + val);
                                }
                                break;
                            case 'r':
                                res_rps = val;
                                if (mode == 1 && !seekbarTouch && !editTextIsInFocus && !telemetryText.contains("o")){
                                    seekBarValue.setProgress(val);
                                    editTextValue.setText("" + val);
                                }
                                break;
                            case 's':
                                res_slip = val;
                                if (mode == 2 && !seekbarTouch && !editTextIsInFocus && !telemetryText.contains("o")){
                                    seekBarValue.setProgress(val);
                                    editTextValue.setText("" + val);
                                }
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
                            case 'o':
                                if (!seekbarTouch && !editTextIsInFocus){
                                    seekBarValue.setProgress(val);
                                    editTextValue.setText("" + val);
                                }
                                break;
                            default:
                                Toast.makeText(MainActivity.this, "Da stimmt was mit der Antwort nicht! Unbekanntes Attribut: " + s.charAt(0), Toast.LENGTH_SHORT).show();
                                break;
                        }
                    }

                    textViewTelemetry.setText("Status: " + (res_armed ? "Armed" : "Disarmed") + "\nSpannung: " + ((float) res_voltage / 100) + "V\nThrottle: " + res_throttle
                            + "\nRPS: " + res_rps + "\nSchlupf: " + res_slip + "%\nGeschwindigkeit (MPU): " + ((float) res_velocity1 / 1000.0) + "m/s\nGeschwindigkeit (Räder): "
                            + ((float) res_velocity2 / 1000.0) + "m/s\nBeschleunigung: " + res_acceleration + " rel. Einheiten\nTemperatur: " + res_temp + "°C");
                } else if (txt.startsWith("MESSAGE")){
                    Toast.makeText(getApplicationContext(), "Nachricht: " + txt.substring(txt.indexOf(' ') + 1), Toast.LENGTH_LONG).show();
                } else if (txt.startsWith("SET")) {
                    String str = txt.toUpperCase();
                    String[] args = str.split(" ");
                    Log.d("random", "received: " + str);
                    try {
                        int value = 0;
                        try {
                            value = Integer.parseInt(args[2].replaceAll("[^0123456789]", ""));
                        } catch (NumberFormatException ignored){}
                        switch(args[2]){
                            case "OFF":
                            case "FALSE":
                            case "THROTTLE":
                                value = 0;
                                break;
                            case "ON":
                            case "TRUE":
                            case "RPS":
                                value = 1;
                                break;
                            case "SLIP":
                                value = 2;
                        }
                        switch (args[1]) {
                            case "RACEMODE":
                            case "RACEMODETOGGLE":
                                changeRaceModeToggle(value > 0);
                                switchRaceMode.setEnabled(true);
                                break;
                            case "MODE":
                            case "MODESPINNER":
                                int r = (int) (Math.random() * 1000);
                                Log.d("random", "from WS: setting mode " + value + " mode was " + espMode + " before, " + r);
                                espMode = value;
                                changeModeSpinner(value);
                                spinnerMode.setEnabled(true);
                                break;
                            case "VALUE":
                                seekBarValue.setProgress(value);
                                editTextValue.setText(value + "");
                                seekBarValue.setEnabled(true);
                                editTextValue.setEnabled(true);
                                break;
                            case "ARMED":
                            default:
                                break;
                        }
                    } catch (ArrayIndexOutOfBoundsException e){
                        e.printStackTrace();
                    }
                } else if (txt.startsWith("BLOCK")){
                    String str = txt.toUpperCase();
                    String[] args = str.split(" ");
                    try {
                        int value = 0;
                        try {
                            Integer.parseInt(args[2].replaceAll("[^0123456789]", ""));
                        } catch (NumberFormatException ignored) {}
                        switch (args[2]) {
                            case "OFF":
                                value = 0;
                                break;
                            case "ON":
                                value = 1;
                                break;
                        }
                        switch (args[1]) {
                            case "RACEMODE":
                            case "RACEMODETOGGLE":
                                changeRaceModeToggle(value > 0);
                                switchRaceMode.setEnabled(false);
                                break;
                            case "VALUE":
                                seekBarValue.setProgress(value);
                                editTextValue.setText(value + "");
                                seekBarValue.setEnabled(false);
                                editTextValue.setEnabled(false);
                                break;
                        }
                    } catch (ArrayIndexOutOfBoundsException e){
                        e.printStackTrace();
                    }
                } else if (txt.startsWith("UNBLOCK")){
                    String str = txt.toUpperCase();
                    String[] args = str.split(" ");
                    try {
                        switch (args[1]) {
                            case "RACEMODE":
                            case "RACEMODETOGGLE":
                                switchRaceMode.setEnabled(true);
                                break;
                            case "VALUE":
                                seekBarValue.setEnabled(true);
                                editTextValue.setEnabled(true);
                                break;
                        }
                    } catch (ArrayIndexOutOfBoundsException e){
                        e.printStackTrace();
                    }
                } else if (txt.startsWith("PONG")){
                    int ping = (int)(System.currentTimeMillis() - pMillis);
                    pingArray[pingCounter] = ping;
                    if (++pingCounter == PING_AMOUNT){
                        int minPing = pingArray[0];
                        int maxPing = pingArray[0];
                        long totalPing = 0;
                        for (int i = 1; i < PING_AMOUNT; i++) {
                            if (pingArray[i] < minPing)
                                minPing = pingArray[i];
                            if (pingArray[i] > maxPing)
                                maxPing = pingArray[i];
                            totalPing += pingArray[i];
                        }
                        int avgPing = (int)(totalPing / PING_AMOUNT);
                        Toast.makeText(getApplicationContext(), "Ping-Ergebnisse\nMenge: " + PING_AMOUNT + "\nDurchschnitt: " + avgPing + "\nMin: " + minPing + "\nMax: " + maxPing, Toast.LENGTH_LONG).show();
                    } else {
                        pMillis = System.currentTimeMillis();
                        wsSend("PING");
                    }
                }
            }
        });
    }

    private String prefixZero(int num, int minLength){
        int minNum = (int) Math.pow(10, minLength - 1);
        String output = "";
        while (num < minNum){
            output += "0";
            minNum /= 10;
        }
        output += num;
        return output;
    };

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

    private void wsSend(String s){
        ws.send(s);
        Log.d("WSCommunication", "sent: " + s);
    }

    private void sendTelemetry(boolean b){
        wsSend(b ? "TELEMETRY:ON" : "TELEMETRY:OFF");
    }

    @Override
    protected void onDestroy() {
        ws.close(1000, "Goodbye !");
        super.onDestroy();
    }

    private void onSetPressed() {
        wsSend("VALUE:" + editTextValue.getText());
        View view = this.getCurrentFocus();
        if (view != null){
            view.clearFocus();
            view = this.getCurrentFocus();
            if (view != null){
                InputMethodManager imm = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
                imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
            }
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

    public void sendArmed(boolean a){
        wsSend(a ? "ARMED:YES" : "ARMED:NO");
    }

    public void sendSoftDisarm(){
        wsSend("MODE:RPS");
        setTimeout(new Runnable() {
            @Override
            public void run() {
                wsSend("VALUE:0");
            }
        }, 5);
        setTimeout(new Runnable() {
            @Override
            public void run() {
                sendArmed(false);
            }
        }, 1000);
    }

    public void sendRaceMode (boolean r){
        wsSend(r ? "RACEMODE:ON" : "RACEMODE:OFF");
    }

    public void sendRequest(){
        if (newSeekbarValueAvailable && newSeekbarValue != -1){
            wsSend("VALUE:" + newSeekbarValue);
        }
        newSeekbarValueAvailable = false;
    }

    private interface TaskHandle {
        //void invalidate();
    }

    private final class EchoWebSocketListener extends WebSocketListener {
        private static final int NORMAL_CLOSURE_STATUS = 1000;
        @Override
        public void onOpen(WebSocket webSocket, okhttp3.Response response) {
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    wsSend("DEVICE:APP");
                    wsSend("TELEMETRY:ON");
                    switchRaceMode.setEnabled(true);
                    seekBarValue.setEnabled(true);
                    editTextValue.setEnabled(true);
                    changeRaceModeToggle(false);
                }
            });
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
            wsLog("Closing: " + code + " / " + reason);
        }
        @Override
        public void onFailure(WebSocket webSocket, Throwable t, okhttp3.Response response) {
            t.printStackTrace();
            wsLog("Error: " + t.getMessage());
        }
    }
}