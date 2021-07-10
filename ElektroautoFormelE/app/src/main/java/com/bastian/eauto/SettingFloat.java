package com.bastian.eauto;

import android.content.Context;
import android.os.Build;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.LayoutInflater;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.SeekBar;
import android.widget.TextView;

import androidx.annotation.RequiresApi;

public class SettingFloat extends FrameLayout {
    private String description = "";
    private double value, min, max;
    private SeekBar sb;
    private TextView tv;
    private EditText et;
    private boolean sbTouch = false;
    Runnable r;

    private static double map (double value, double pMin, double pMax, double nMin, double nMax){
        return (value - pMin) / (pMax - pMin) * (nMax - nMin) + nMin;
    }

    @RequiresApi(api = Build.VERSION_CODES.O)
    public SettingFloat(Context context, String description, double iMin, double iMax, double startValue) {
        super(context);

        LayoutInflater li = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        li.inflate(R.layout.sample_setting_float, this);

        if (startValue < iMin) startValue = iMin;
        else if (startValue > iMax) startValue = iMax;

        value = startValue;
        this.min = iMin;
        this.max = iMax;

        sb = this.findViewById(R.id.seekBar);
        tv = this.findViewById(R.id.textView);
        et = this.findViewById(R.id.editText);

        sb.setProgress((int) map(value, min, max, 0, 2000), true);
        tv.setText(description);
        et.setText(String.valueOf(value));

        et.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {

            }

            @Override
            public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {

            }

            @Override
            public void afterTextChanged(Editable editable) {
                if (et.hasFocus() && !sbTouch) {
                    double newValue = 0;
                    try {
                        newValue = Double.parseDouble(editable.toString());
                    } catch (NumberFormatException e){
                        e.printStackTrace();
                    }
                    if (newValue < min) newValue = min;
                    else if (newValue > max) newValue = max;
                    Log.d("TAG", "aktiv");
                    sb.setProgress((int) map(newValue, min, max, 0, 2000));
                    value = newValue;
                    inputEvent();
                }
            }
        });
        sb.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int i, boolean b) {
                if (b){
                    double newValue = map(i, 0, 2000, min, max);
                    et.setText(String.valueOf(newValue));
                    value = newValue;
                    inputEvent();
                }
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                sbTouch = true;
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                sbTouch = false;
            }
        });
    }

    public void setValue(double newValue) {
        if (newValue < min) newValue = min;
        else if (newValue > max) newValue = max;
        value = newValue;
        sb.setProgress((int) map(value, min, max, 0, 2000));
        et.setText(String.valueOf(value));
    }
    public void setMin(double newMin){
        min = newMin;
        if (value < min){
            setValue(min);
            inputEvent();
        }
    }
    public void setMax(double newMax){
        max = newMax;
        if (value > max) {
            setValue(max);
            inputEvent();
        }
    }
    public void setDescription (String newDescription){
        description = newDescription;
        tv.setText(newDescription);
    }

    public void setInputListener(Runnable r){
        this.r = r;
    }

    public String getDescription(){
        return description;
    }
    public double getMin(){
        return min;
    }
    public double getMax(){
        return max;
    }
    public double getValue(){
        return value;
    }

    private void inputEvent(){
        r.run();
    }
}