package com.bastian.eauto;

import android.content.Context;
import android.os.Build;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.SeekBar;
import android.widget.TextView;

import androidx.annotation.RequiresApi;

public class SettingInt extends FrameLayout {
    private String description = "";
    private int value = 0, min = 0, max = 1;
    private SeekBar sb;
    private TextView tv;
    private EditText et;
    private boolean sbTouch = false;
    Runnable r;

    @RequiresApi(api = Build.VERSION_CODES.O)
    public SettingInt(Context context, String description, int min, int max, int startValue) {
        super(context);

        LayoutInflater li = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        li.inflate(R.layout.sample_setting_int, this);

        if (startValue < min) startValue = min;
        else if (startValue > max) startValue = max;

        value = startValue;
        this.min = min;
        this.max = max;

        sb = this.findViewById(R.id.seekBar);
        tv = this.findViewById(R.id.textView);
        et = this.findViewById(R.id.editText);

        sb.setMax(max);
        sb.setMin(min);
        sb.setProgress(value, true);
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
                    int newValue = 0;
                    try {
                        newValue = Integer.parseInt(editable.toString().replaceAll("[^1234567890]", ""));
                    } catch (NumberFormatException e){
                        e.printStackTrace();
                    }
                    if (newValue < min) newValue = min;
                    else if (newValue > max) newValue = max;
                    sb.setProgress(newValue);
                    value = newValue;
                    inputEvent();
                }
            }
        });
        sb.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int i, boolean b) {
                if (b){
                    et.setText(String.valueOf(i));
                    value = i;
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

    public void setValue(int newValue) {
        if (newValue < min) newValue = min;
        else if (newValue > max) newValue = max;
        value = newValue;
        sb.setProgress(value);
        et.setText(String.valueOf(value));
    }
    public void setMin(int newMin){
        min = newMin;
        if (value < min){
            setValue(min);
            inputEvent();
        }
        sb.setMin(min);
    }
    public void setMax(int newMax){
        max = newMax;
        if (value > max) {
            setValue(max);
            inputEvent();
        }
        sb.setMax(max);
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
    public int getMin(){
        return min;
    }
    public int getMax(){
        return max;
    }
    public int getValue(){
        return value;
    }

    private void inputEvent(){
        r.run();
    }
}