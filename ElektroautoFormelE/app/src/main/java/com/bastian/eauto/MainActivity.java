package com.bastian.eauto;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.SwitchCompat;
import androidx.core.content.ContextCompat;

import android.Manifest;
import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.util.Calendar;
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

    //constants
    private final int REQUEST_UPDATE_MS = 20;
    private final int LOG_FRAMES = 5000;
    private final int PING_AMOUNT = 20;

    //SharedPrefs
    SharedPreferences mPreferences;
    SharedPreferences.Editor mEditor;

    //WebSockets
    WebSocket ws;
    private OkHttpClient client;
    private boolean gotTelemetry = false;

    //views
    private EditText editTextIP, editTextValue, editTextCommand;
    private Button buttonArm, buttonDisarm, buttonSet, buttonSoftDisarm, buttonStartRace;
    private TextView textViewTelemetry;
    private ImageButton ibReconnect, ibPing, ibSettings;
    private Spinner spinnerMode;
    private SeekBar seekBarValue;
    private SwitchCompat switchRaceMode;
    private Button buttonSend;
    private ImageButton buttonForward, buttonBackward, buttonDirectionOne, buttonDirectionTwo, buttonRed, buttonGreen, buttonBlue;

    //main variables
    private int espMode = 0, modeBeforeSoftDisarm = 0;
    private int res_throttle = 0, res_rps = 0, res_slip = 0, res_velocity1 = 0, res_velocity2 = 0, res_acceleration = 0, res_voltage = 0, res_temp = 0;
    private MainActivity.TaskHandle autoSend;
    private boolean newSeekbarValueAvailable = false;
    private int newSeekbarValue = -1;
    private boolean rmUserChanged = true, modeUserChanged = true;
    boolean seekbarTouch = false;
    private boolean firstStartup = true;
    private boolean redLED = false, greenLED = false, blueLED = false;

    //ping
    private long pMillis = 0;
    private int pingCounter = 0;
    private int[] pingArray = new int[PING_AMOUNT];

    //log variables
    private int[] throttle_log_0 = new int[LOG_FRAMES], throttle_log_1 = new int[LOG_FRAMES],
            erpm_log_0 = new int[LOG_FRAMES], erpm_log_1 = new int[LOG_FRAMES],
            voltage_log_0 = new int[LOG_FRAMES], voltage_log_1 = new int[LOG_FRAMES],
            temp_log_0 = new int[LOG_FRAMES], temp_log_1 = new int[LOG_FRAMES],
            acceleration_log = new int[LOG_FRAMES],
            temp_log_bmi = new int[LOG_FRAMES];

    private static TaskHandle setTimeout(final Runnable r, long delay) {
        final Handler h = new Handler();
        h.postDelayed(r, delay);
        return new TaskHandle() {
            public void invalidate() { h.removeCallbacks(r); }
        };
    }

    public static TaskHandle setInterval(final Runnable r, long interval) {
        final Timer t = new Timer();
        final Handler h = new Handler();
        t.scheduleAtFixedRate(new TimerTask() {
            @Override
            public void run() { h.post(r); }
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
        ibSettings = findViewById(R.id.ibSettings);
        editTextIP = findViewById(R.id.editTextIP);
        buttonStartRace = findViewById(R.id.buttonStartRace);
        switchRaceMode = findViewById(R.id.switchRaceMode);
        buttonSend = findViewById(R.id.buttonSend);
        editTextCommand = findViewById(R.id.editTextCommand);
        buttonForward = findViewById(R.id.ibForward);
        buttonBackward = findViewById(R.id.ibBackward);
        buttonDirectionOne = findViewById(R.id.ibDirectionOne);
        buttonDirectionTwo = findViewById(R.id.ibDirectionTwo);
        buttonRed = findViewById(R.id.ibRed);
        buttonGreen = findViewById(R.id.ibGreen);
        buttonBlue = findViewById(R.id.ibBlue);

        mPreferences = PreferenceManager.getDefaultSharedPreferences(this);
        mEditor = mPreferences.edit();

        editTextIP.setText(mPreferences.getString("IP", "192.168.0.111"));

        buttonArm.setOnClickListener(_v -> sendArmed (true));//Arm
        buttonDisarm.setOnClickListener(_V -> sendArmed(false));//Disarm
        buttonSoftDisarm.setOnClickListener(_v -> sendSoftDisarm());//Soft disarm
        buttonSet.setOnClickListener(_v -> onSetPressed());//Setzt die momentan eingetragene Dauer
        ibReconnect.setOnClickListener(_v -> {
            textViewTelemetry.setText("Verbindung wird aufgebaut...");
            ws.close(1000, "Goodbye !");
            wsStart();
            mEditor.putString("IP", editTextIP.getText().toString());
            mEditor.apply();
        }); //reconnect
        ibPing.setOnClickListener(_v -> {
            pMillis = System.currentTimeMillis();
            pingCounter = 0;
            wsSend("PING");
        });//ping
        ibSettings.setOnClickListener(_v -> {
            wsSend("SETTINGS");
        });

        editTextValue.setOnEditorActionListener((_v, _actionId, _event) -> {
            onSetPressed();
            return false;
        });

        seekBarValue.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar _seekBar, int i, boolean b) {
                if (b) {
                    newSeekbarValueAvailable = true;
                    newSeekbarValue = i;
                }
            }

            @Override public void onStartTrackingTouch(SeekBar _seekBar) { seekbarTouch = true; }

            @Override public void onStopTrackingTouch(SeekBar _seekBar) { seekbarTouch = false; }
        });

        spinnerMode.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> _adapterView, View _v, int i, long l) {
                if (firstStartup)
                    return;
                if (modeUserChanged) {
                    wsSend("MODE:" + i);
                }
                modeUserChanged = true;
            }

            @Override
            public void onNothingSelected(AdapterView<?> _adapterView) {
            }
        });

        switchRaceMode.setOnCheckedChangeListener((_buttonView, isChecked) -> {
            if (rmUserChanged){
                // transmit rm change to arduino
                changeRaceModeToggle(!isChecked);
                sendRaceMode(isChecked);
            } else {
                buttonArm.setVisibility(isChecked ? View.GONE : View.VISIBLE);
                buttonStartRace.setVisibility(isChecked ? View.VISIBLE : View.GONE);
            }
        });

        buttonStartRace.setOnClickListener(_v -> wsSend("STARTRACE"));

        buttonSend.setOnClickListener(_v -> {
            wsSend(editTextCommand.getText().toString());
        });

        buttonForward.setOnClickListener(_v -> {sendRawData("00FF", 10);});
        buttonBackward.setOnClickListener(_v -> {sendRawData("0110", 10);});
        buttonDirectionOne.setOnClickListener(_v -> {sendRawData("029B", 10);});
        buttonDirectionTwo.setOnClickListener(_v -> {sendRawData("02B9", 10);});
        buttonRed.setOnClickListener(_v -> {
            if(redLED)
                sendRawData("001A", 1);
            else
                sendRawData("0016", 1);
        });//on: 02DF, off: 0356
        buttonGreen.setOnClickListener(_v -> {
            if(greenLED)
                sendRawData("001B", 1);
            else
                sendRawData("0017", 1);
            //greenLED = !greenLED;
        });//on: 02FD, off: 0374
        buttonBlue.setOnClickListener(_v -> {
            if(blueLED)
                sendRawData("001C", 1);
            else
                sendRawData("0018", 1);
            //blueLED = !blueLED;
        });//on: 0312, off: 0399

        wsStart();

        autoSend = setInterval(this::sendRequest, REQUEST_UPDATE_MS);

        setTimeout(() -> firstStartup = false, 200);
    }

    private void wsLog(final String txt) {
        runOnUiThread(() -> {
            Log.d("WSCommunication", txt);
            textViewTelemetry.setText(txt);
        });
    }

    @Override
    protected void onStart() {
        if (ContextCompat.checkSelfPermission(MainActivity.this, Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_DENIED){
            String[] perm = {Manifest.permission.WRITE_EXTERNAL_STORAGE};
            requestPermissions(perm, 0);
        }
        super.onStart();
    }

    private void sendRawData(String s, int repeat){
        String message = "RAWDATA:";
        for (int i = 0; i < repeat; i++){
            message += s;
            if (i != repeat - 1){
                message += " ";
            }
        }
        wsSend(message);
    }

    private void changeModeSpinner(int position){
        if (spinnerMode.getSelectedItemPosition() != position) {
            modeUserChanged = false;
            spinnerMode.setSelection(position, false);
        }
    }

    private void onWSText(final String txt) {
        gotTelemetry = true;
        runOnUiThread(() -> {
            if (txt.startsWith("TELEMETRY")) {
                String telemetryText = txt.substring(txt.indexOf(" ") + 1);
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
                            }
                            break;
                        case 'r':
                            res_rps = val;
                            if (mode == 1 && !seekbarTouch && !editTextIsInFocus && !telemetryText.contains("o")){
                                seekBarValue.setProgress(val);
                            }
                            break;
                        case 's':
                            res_slip = val;
                            if (mode == 2 && !seekbarTouch && !editTextIsInFocus && !telemetryText.contains("o")){
                                seekBarValue.setProgress(val);
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
                            if (!editTextIsInFocus)
                                editTextValue.setText("" + val);
                            if (!seekbarTouch){
                                seekBarValue.setProgress(val);
                            }
                            break;
                        case 'q':
                            if (!editTextIsInFocus){
                                editTextValue.setText("" + val);
                            }
                            break;
                        default:
                            Toast.makeText(MainActivity.this, "Da stimmt was mit der Antwort nicht! Unbekanntes Attribut: " + s.charAt(0), Toast.LENGTH_SHORT).show();
                            break;
                    }
                }

                textViewTelemetry.setText("Status: " + (res_armed ? "Armed" : "Disarmed") + "\nSpannung: " + ((float) res_voltage / 100) + " V\nThrottle: " + res_throttle
                        + "\nRPS: " + res_rps + "\nSchlupf: " + res_slip + "%\nGeschwindigkeit (MPU): " + ((float) res_velocity1 / 1000.0) + " m/s\nGeschwindigkeit (Räder): "
                        + ((float) res_velocity2 / 1000.0) + " m/s\nBeschleunigung: " + res_acceleration/1000.0 + " m/s²\nTemperatur: " + res_temp + "°C");
            }
            else if (txt.startsWith("MESSAGE ")) {
                Toast.makeText(getApplicationContext(), txt.substring(txt.indexOf(' ') + 1), Toast.LENGTH_SHORT).show();
            } else if (txt.startsWith("MESSAGEBEEP")){
                Toast.makeText(getApplicationContext(), txt.substring(txt.indexOf(' ') + 1), Toast.LENGTH_SHORT).show();
                final MediaPlayer mp = MediaPlayer.create(getApplicationContext(), R.raw.alarm);
                mp.start();
            }
            else if (txt.startsWith("SET ")) {
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
                        case "REDLED":
                            redLED = value > 0;
                            buttonRed.setImageResource(redLED ? R.drawable.ic_red_on : R.drawable.ic_red_off);
                            break;
                        case "GREENLED":
                            greenLED = value > 0;
                            buttonGreen.setImageResource(greenLED ? R.drawable.ic_green_on : R.drawable.ic_green_off);
                            break;
                        case "BLUELED":
                            blueLED = value > 0;
                            buttonBlue.setImageResource(blueLED ? R.drawable.ic_blue_on : R.drawable.ic_blue_off);
                            break;
                        case "ARMED":
                        default:
                            break;
                    }
                } catch (ArrayIndexOutOfBoundsException e){
                    e.printStackTrace();
                }
            }
            else if (txt.startsWith("BLOCK")){
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
            }
            else if (txt.startsWith("UNBLOCK")){
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
            }
            else if (txt.startsWith("PONG")){
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
                    Toast.makeText(getApplicationContext(), "Ping-Ergebnisse\nMenge: " + PING_AMOUNT + "\nDurchschnitt: " + avgPing + "\nMin: " + minPing + "\nMax: " + maxPing, Toast.LENGTH_SHORT).show();
                } else {
                    pMillis = System.currentTimeMillis();
                    wsSend("PING");
                }
            }
            else if (txt.startsWith("MAXVALUE")) {
                int value = 0;
                try {
                    value = Integer.parseInt(txt.replaceAll("[^0123456789]", ""));
                } catch (NumberFormatException ignored){}
                seekBarValue.setMax(value);
            }
            else if (txt.startsWith("SETTINGS")) {
                Intent settingsIntent = new Intent(this, SettingsActivity.class);
                settingsIntent.putExtra("settings", txt.substring(txt.indexOf('\n') + 1));
                startActivityForResult(settingsIntent, 0);
            }

        });
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        if (resultCode == 0 && requestCode == 0 && data != null){
            for (String command : data.getStringArrayExtra("commands")){
                if (command != null) wsSend(command);
            }
            wsSend("SAVE");
            setTimeout(() -> {
                gotTelemetry = false;
            }, 500);
            setTimeout(() -> {
                if (!gotTelemetry)
                    wsStart();
            }, 1000);
        }
        if (requestCode == 0 && resultCode == 1){
            wsSend("RESTORE");
            setTimeout(() -> {
                ws.close(1000, "goodbye");
                wsStart();
            }, 1000);
        }
        super.onActivityResult(requestCode, resultCode, data);
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

    private void changeRaceModeToggle (boolean state){
        rmUserChanged = false;
        switchRaceMode.setChecked(state);
        rmUserChanged = true;
    }

    public void sendArmed(boolean a){
        wsSend(a ? "ARMED:YES" : "ARMED:NO");
    }

    public void sendSoftDisarm(){
        modeBeforeSoftDisarm = espMode;
        wsSend("MODE:RPS");
        setTimeout(() -> wsSend("VALUE:0"), 2);
        setTimeout(() -> {
            sendArmed(false);
            wsSend("MODE:" + modeBeforeSoftDisarm);
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
        public void onOpen(WebSocket _webSocket, okhttp3.Response response) {
            runOnUiThread(() -> {
                wsSend("DEVICE:APP");
                wsSend("TELEMETRY:ON");
                switchRaceMode.setEnabled(true);
                seekBarValue.setEnabled(true);
                editTextValue.setEnabled(true);
                changeRaceModeToggle(false);
            });
        }
        @Override
        public void onMessage(WebSocket _webSocket, String text) {
            onWSText(text);
        }
        @Override
        public void onMessage(WebSocket _webSocket, ByteString bytes) {
            onWSBin(bytes);
        }
        @Override
        public void onClosing(WebSocket webSocket, int code, String reason) {
        }
        @Override
        public void onFailure(WebSocket webSocket, Throwable t, okhttp3.Response response) {
            t.printStackTrace();
            wsLog("Error: " + t.getMessage());
        }
    }

    void onWSBin(final ByteString bin) {
        runOnUiThread(() -> {
            byte[] bytes = bin.toByteArray();
            for (int i = 0; i < LOG_FRAMES; i++){
                throttle_log_0[i] = (((bytes[i * 2 + 1]) & 0xFF) << 8) | (bytes[i * 2] & 0xFF);
                throttle_log_1[i] = (((bytes[i * 2 + LOG_FRAMES * 2 + 1]) << 8) & 0xFF) | (bytes[i * 2 + LOG_FRAMES * 2] & 0xFF);
                erpm_log_0[i] = (((bytes[i * 2 + LOG_FRAMES * 4 + 1]) & 0xFF) << 8) | (bytes[i * 2 + LOG_FRAMES * 4] & 0xFF);
                erpm_log_1[i] = (((bytes[i * 2 + LOG_FRAMES * 6 + 1]) & 0xFF) << 8) | (bytes[i * 2 + LOG_FRAMES * 6] & 0xFF);
                voltage_log_0[i] = (((bytes[i * 2 + LOG_FRAMES * 8 + 1]) & 0xFF) << 8) | (bytes[i * 2 + LOG_FRAMES * 8] & 0xFF);
                voltage_log_1[i] = (((bytes[i * 2 + LOG_FRAMES * 10 + 1]) & 0xFF) << 8) | (bytes[i * 2 + LOG_FRAMES * 10] & 0xFF);
                temp_log_bmi[i] = (bytes[i + LOG_FRAMES * 12] & 0xFF);
                temp_log_bmi[i] = (bytes[i + LOG_FRAMES * 13] & 0xFF);
                acceleration_log[i] = (((bytes[i * 2 + LOG_FRAMES * 14 + 1]) & 0xFF) << 8) | (bytes[i * 2 + LOG_FRAMES * 14] & 0xFF);
                temp_log_1[i] = (((bytes[i * 2 + LOG_FRAMES * 16 + 1]) & 0xFF) << 8) | (bytes[i * 2 + LOG_FRAMES * 16] & 0xFF);
            }

            try {
                JSONObject output = new JSONObject();
                JSONArray finalThrottle0Array = new JSONArray(throttle_log_0);
                JSONArray finalThrottle1Array = new JSONArray(throttle_log_1);
                JSONArray finalERPM0Array = new JSONArray(erpm_log_0);
                JSONArray finalERPM1Array = new JSONArray(erpm_log_1);
                JSONArray finalVoltage0Array = new JSONArray(voltage_log_0);
                JSONArray finalVoltage1Array = new JSONArray(voltage_log_1);
                JSONArray finalTemperature0Array = new JSONArray(temp_log_0);
                JSONArray finalTemperature1Array = new JSONArray(temp_log_1);
                JSONArray finalAccelerationArray = new JSONArray(acceleration_log);
                JSONArray finalTemperatureBMIArray = new JSONArray(temp_log_bmi);
                output.put("throttle0", finalThrottle0Array);
                output.put("throttle1", finalThrottle1Array);
                output.put("erpm0", finalERPM0Array);
                output.put("erpm1", finalERPM1Array);
                output.put("voltage0", finalVoltage0Array);
                output.put("voltage1", finalVoltage1Array);
                output.put("temperature0", finalTemperature0Array);
                output.put("temperature1", finalTemperature1Array);
                output.put("acceleration", finalAccelerationArray);
                output.put("temperatureBMI", finalTemperatureBMIArray);
                String fileName = "log " + Calendar.getInstance().get(Calendar.YEAR) + "-" + prefixZero(Calendar.getInstance().get(Calendar.MONTH) + 1, 2) + "-"
                        + prefixZero(Calendar.getInstance().get(Calendar.DATE), 2) + "-" + prefixZero(Calendar.getInstance().get(Calendar.HOUR_OF_DAY), 2)
                        + ":" + prefixZero(Calendar.getInstance().get(Calendar.MINUTE), 2) + ":"
                        + prefixZero(Calendar.getInstance().get(Calendar.SECOND), 2) + ".json";
                File folder;
                boolean permissionGranted = ContextCompat.checkSelfPermission(MainActivity.this, Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED;
                if (permissionGranted) {
                    folder = new File(Environment.getExternalStorageDirectory(), "Formel-E-Logs");
                } else {
                    folder = getExternalFilesDir("Formel-E-Logs");
                }
                if (!folder.exists()) {
                    folder.mkdir();
                }
                File file = new File(folder, fileName);
                if (file.exists()) {
                    String p = file.getAbsolutePath();
                    int i;
                    for (i = 2; i < 100; i++) {
                        if (!(new File(p + "_" + i).exists())) break;
                    }
                    if (i == 100) {
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
                    Toast.makeText(MainActivity.this, permissionGranted ? "Output in Interner Speicher -> Formel-E-Logs -> " + fileName + " gespeichert" : "Da die Berechtigung nicht erteilt wurde, wurde der Log unter " + file.getAbsolutePath() + " gespeichert.", Toast.LENGTH_LONG).show();
                } else {
                    Toast.makeText(MainActivity.this, "Fehler beim Speichern der Datei", Toast.LENGTH_SHORT).show();
                }

                throttle_log_0 = new int[LOG_FRAMES];
                throttle_log_1 = new int[LOG_FRAMES];
                erpm_log_0 = new int[LOG_FRAMES];
                erpm_log_1 = new int[LOG_FRAMES];
                voltage_log_0 = new int[LOG_FRAMES];
                voltage_log_1 = new int[LOG_FRAMES];
                temp_log_0 = new int[LOG_FRAMES];
                temp_log_1 = new int[LOG_FRAMES];
                acceleration_log = new int[LOG_FRAMES];
                temp_log_bmi = new int[LOG_FRAMES];
            } catch (JSONException e) {
                e.printStackTrace();
            } catch (IOException e) {
                Toast.makeText(MainActivity.this, "Fehler beim Speichern der Datei", Toast.LENGTH_SHORT).show();
                e.printStackTrace();
            }
        });
    }
}