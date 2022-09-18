
#define bk_bat 27
#define bat_1 39
#define bat_2 36
#define bt1_r1 1200
#define bt1_r2 470
#define bt2_r1 1200
#define bt2_r2 470
#define bt1_thresshold 6000
#define bt2_thresshold 6000
#define bt1_ratio (bt1_r1+bt1_r2)/bt1_r2
#define bt2_ratio (bt2_r1+bt2_r2)/bt2_r2
uint32_t bat1_voltage, bat2_voltage;
byte sel_bat;
uint32_t vadc1, vadc2;
void handle_batteries(){
    vadc1 = analogReadMilliVolts(bat_1);
    vadc2 = analogReadMilliVolts(bat_2);
    // bat1_voltage = (vadc1 * bt1_r2) / (bt1_r1 + bt1_r2);
    // bat2_voltage = (vadc2 * bt2_r2) / (bt2_r1 + bt2_r2);
    bat1_voltage = vadc1 * bt1_ratio;
    bat2_voltage = vadc2 * bt2_ratio;

    if(bat1_voltage < bt1_thresshold && sel_bat == 0 && bat2_voltage > bt1_thresshold){
        sel_bat = 1;
    }else if (bat1_voltage > bt1_thresshold + 1000 && sel_bat == 1){
        sel_bat = 0;
    }

    digitalWrite(bk_bat, sel_bat? 1:0);

}