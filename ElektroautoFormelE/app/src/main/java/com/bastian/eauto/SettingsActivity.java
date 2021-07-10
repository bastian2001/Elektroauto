package com.bastian.eauto;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.Toast;

public class SettingsActivity extends AppCompatActivity {

    private LinearLayout page;
    private String[] commandArray;

    @Override
    public void onBackPressed() {
        Intent intent = new Intent();
        intent.putExtra("commands", commandArray);
        setResult(0, intent);
        super.onBackPressed();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_settings);

        page = findViewById(R.id.llSettings);
        Button buttonReset = findViewById(R.id.buttonReset);
        buttonReset.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                setResult(1);
                finish();
            }
        });
        String[] settings = getIntent().getStringExtra("settings").split("\n");
        commandArray = new String[settings.length];
        for (int i = 0; i < settings.length; i++){
            final int j = i;
            final String setting = settings[i];
            String[] data = setting.split("_");
            if (data.length != 6){
                Toast.makeText(this, "something is wrong with " + setting, Toast.LENGTH_SHORT).show();
                continue;
            }
            final String type = data[0];
            final String key = data[1];
            final String name = data[2];
            final String minS = data[3];
            final String maxS = data[4];
            final String valueS = data[5];
            switch (type){
                case "int":
                {
                    int min = 0, max = 1, value = 0;
                    try {
                        min = Integer.parseInt(minS);
                        max = Integer.parseInt(maxS);
                        value = Integer.parseInt(valueS);
                    } catch (NumberFormatException e){
                        e.printStackTrace();
                    }
                    SettingInt settingIntView = new SettingInt(this, name, min, max, value);
                    page.addView(settingIntView);
                    settingIntView.setInputListener(() -> {
                        commandArray[j] = key + ":" + settingIntView.getValue() + "NOMESSAGE";
                    });
                }
                    break;
                case "float":
                    double min = 0, max = 1, value = 0;
                    try {
                        min = Double.parseDouble(minS);
                        max = Double.parseDouble(maxS);
                        value = Double.parseDouble(valueS);
                    } catch (NumberFormatException e){
                        e.printStackTrace();
                    }
                    SettingFloat settingFloatView = new SettingFloat(this, name, min, max, value);
                    page.addView(settingFloatView);
                    settingFloatView.setInputListener(() -> {
                        commandArray[j] = key + ":" + settingFloatView.getValue() + "NOMESSAGE";
                    });
                    break;
                default:
                    Toast.makeText(this, "Error with type", Toast.LENGTH_SHORT).show();
            }
        }
    }
}