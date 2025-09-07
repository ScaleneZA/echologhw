void setup() {
  Serial.begin(115200);
  delay(5000);
  Serial.println("HELLO FROM XIAO ESP32S3!");
}

void loop() {
  Serial.println("Loop running...");
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);  
  delay(500);
}