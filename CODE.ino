#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <LiquidCrystal_I2C.h>

#define SS_PIN 10
#define RST_PIN 9

#define DT A0
#define SCK A1
#define sw 9

MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 20, 4);
Servo myservo;

long sample = 0;
float val = 0;
long count = 0;

bool potPlaced = false;
bool prevPotPlaced = false;

int y = 0;

String uid1 = "BA C3 D9 63";
String uid2 = "05 3A B0 C0 08 B0 C1";


unsigned long readCount(void)
{
    unsigned long Count = 0;
    pinMode(DT, OUTPUT);
    digitalWrite(DT, HIGH);
    digitalWrite(SCK, LOW);

    pinMode(DT, INPUT);
    while (digitalRead(DT));

    for (byte i = 0; i < 24; i++)
    {
        digitalWrite(SCK, HIGH);
        Count <<= 1;
        digitalWrite(SCK, LOW);
        if (digitalRead(DT)) Count++;
    }

    digitalWrite(SCK, HIGH);
    Count ^= 0x800000;
    digitalWrite(SCK, LOW);

    return Count;
}


void setup()
{
    SPI.begin();
    mfrc522.PCD_Init();

    Serial.begin(9600);

    myservo.attach(3);
    myservo.write(160);

    pinMode(SCK, OUTPUT);
    pinMode(sw, INPUT_PULLUP);
    pinMode(2, OUTPUT);

    lcd.init();
    lcd.backlight();

    calibrate();
}

void loop()
{
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Scan RFID Card...");
    Serial.println("Scan RFID Card...");

    while (!mfrc522.PICC_IsNewCardPresent())
    {
        delay(50);
    }

    if (!mfrc522.PICC_ReadCardSerial()) return;

    String content = "";
    for (byte i = 0; i < mfrc522.uid.size; i++)
    {
        content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
        content.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    content.toUpperCase();
    content = content.substring(1); 

    Serial.print("UID: ");
    Serial.println(content);

   
    if (content == uid1 || content == uid2)
    {
        Serial.println("AUTHORIZED");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Access Granted");

        digitalWrite(2, HIGH); delay(500);
        digitalWrite(2, LOW);  delay(500);
    }
    else
    {
        Serial.println("ACCESS DENIED");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("ACCESS DENIED");

        for (int i = 0; i < 3; i++)
        {
            digitalWrite(2, HIGH); delay(500);
            digitalWrite(2, LOW);  delay(500);
        }
        return;
    }


    while (true)
    {
        count = readCount();
        int w = -((count - sample) / val);

        potPlaced = (w > 3);

        if (!potPlaced)
        {
            lcd.clear();
            lcd.setCursor(0, 1);
            lcd.print("Place Container");
            Serial.println("Place Container");

            myservo.write(160);
            prevPotPlaced = false;

            delay(300);
            continue;
        }

       
        if (potPlaced && !prevPotPlaced)
        {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Container Placed");

            digitalWrite(2, HIGH); delay(500);
            digitalWrite(2, LOW);  delay(500);

            lcd.clear();
            lcd.setCursor(0, 1);
            lcd.print("Enter Quantity:");
            Serial.println("Enter Quantity:");

            while (Serial.available()) Serial.read();
            while (!Serial.available());

            y = Serial.parseInt();

            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Entered: ");
            lcd.print(y);
            lcd.print(" g");

            delay(1500);
            lcd.clear();
        }

     
        if (y > 0)
        {
            lcd.setCursor(0, 1);
            lcd.print("Target:");
            lcd.print(y);
            lcd.print("g ");

            lcd.setCursor(0, 0);
            lcd.print("Current:");
            lcd.print(w);
            lcd.print("g ");

            if (w >= y)
            {
                myservo.write(160);
                lcd.setCursor(0, 3);
                lcd.print("Collect Rice");

                for (int i = 0; i < 2; i++)
                {
                    digitalWrite(2, HIGH); delay(500);
                    digitalWrite(2, LOW);  delay(500);
                }

                delay(2000);
                y = 0;
                break;
            }
            else
            {
                myservo.write(90);
            }
        }

        prevPotPlaced = potPlaced;
        delay(200);
    }
}


void calibrate()
{
    lcd.setCursor(0, 1);
    lcd.print("Calibrating...");
    sample = 0;

    for (int i = 0; i < 100; i++)
    {
        count = readCount();
        sample += count;
    }

    sample /= 100;
    val = 400.0;

    lcd.setCursor(0, 0);
    lcd.print("Calibration OK");

    delay(1000);

    digitalWrite(2, HIGH); delay(500);
    digitalWrite(2, LOW);  delay(500);
}
