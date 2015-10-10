/*
  Assignment 1.2

  Improved encrypted chat program between two arduinos.
*/

#include <Arduino.h>

/* Generates private key for the user */

uint32_t generate_private(){
    uint32_t private_key = 0;
    uint32_t read_random = 0;
    for(int i = 0; i < 31; ++i){
        
        //takes least significant bit of fluctuating analog signal
        read_random = analogRead(1);
        read_random = read_random % 2;
        
        private_key = private_key + read_random;
        private_key = private_key << 1;
   
        delay(50);
    }
    return private_key;
}

//~ 
//~ uint32_t mul_mod(uint32_t a, uint32_t b, uint32_t m) {
    //~ uint32_t product = 0;
    //~ int a_bit;
    //~ //mod b in case it is too large
    //~ b = b % m;
    //~ for (int i = 0; i <= 31; i++) {
        //~ a_bit = 1 & (a >> i);
        //~ if (a_bit == 1) {
            //~ product = (product + (a_bit % m) * b )% m;
        //~ }
        //~ b = (b << 1) % m;
    //~ }
    //~ return product;
//~ }

uint32_t mul_mod(uint32_t a, uint32_t b, uint32_t m) {
  uint32_t result = 0;
  uint32_t x = b % m;
   
  for (int i = 0; i < 32; ++i) {
    uint32_t myBit = 31 - i;
    //~ Serial.print(i);
    //~ Serial.print(": ");
    //~ Serial.println(((a << myBit) >> 31));
    if (((a << myBit) >> 31) == 1) {
      result = (result + x) % m;
    }
    x = (x << 1) % m;
  }

  return result;
}

uint32_t pow_mod(uint32_t base, uint32_t power, uint32_t m) {
  uint32_t result = 1;
  uint32_t x = base % m;
  for (int i = 0; i < 32; ++i ) {
    if ((power & (1UL << i)) != 0) {
      result = mul_mod(result, x, m);
    }
    x = mul_mod(x, x, m);
  }
  return result;
}




/** Waits for a certain number of bytes on Serial3 or timeout 
 * @param nbytes: the number of bytes we want
 * @param timeout: timeout period (ms); specifying a negative number
 *                turns off timeouts (the function waits indefinitely
 *                if timeouts are turned off).
 * @return True if the required number of bytes have arrived.
 */
bool wait_on_serial3( uint8_t nbytes, long timeout ) {
  unsigned long deadline = millis() + timeout;//wraparound not a problem
  while (Serial3.available()<nbytes && (timeout<0 || millis()<deadline)) 
  {
    delay(1); // be nice, no busy loop
  }
  return Serial3.available()>=nbytes;
}

/** Writes an uint32_t to Serial3, starting from the least-significant
 * and finishing with the most significant byte. 
 */
void uint32_to_serial3(uint32_t num) {
  Serial3.write((char) (num >> 0));
  Serial3.write((char) (num >> 8));
  Serial3.write((char) (num >> 16));
  Serial3.write((char) (num >> 24));
}

/** Reads an uint32_t from Serial3, starting from the least-significant
 * and finishing with the most significant byte. 
 */
uint32_t uint32_from_serial3() {
  uint32_t num = 0;
  num = num | ((uint32_t) Serial3.read()) << 0;
  num = num | ((uint32_t) Serial3.read()) << 8;
  num = num | ((uint32_t) Serial3.read()) << 16;
  num = num | ((uint32_t) Serial3.read()) << 24;
  return num;
}


uint32_t skey = 0;
uint32_t ckey = 0;


bool handshake(int mode){
    typedef enum { Start=0, WaitForAck_1, Listen, WaitingForKey_1,
        WaitForAck_2, WaitingForKey_2, WaitForAck_3,
         DataExchange } State;
    char* StateNames[] = {"Start","WaitForAck_1","Listen",
        "WaitingForKey_1","WaitForAck_2","WaitingForKey_2",
        "WaitForAck_3","DataExchange" };
    State state = State(mode);
    unsigned long time_check = millis();
    bool logic_check = 0;
    
    while( (state != DataExchange) ){

        logic_check = 0;
        
        /* client starting point */
        if (state == Start){
            Serial3.write('C');
            uint32_to_serial3(ckey);
            state = WaitForAck_1;
            Serial.println("Start");
        }
        else if (state == WaitForAck_1){
            logic_check = wait_on_serial3(5,1000);
            Serial.println("WaitForAck_1");
                if(logic_check &&  ((char)(Serial3.read()) == 'A') ){
                skey = uint32_from_serial3();
                //Serial.print();
                Serial3.write('A');
                state = DataExchange;
            }
            else { state = Start; }
        }

        /* server starting point */
        else if (state == Listen){
            logic_check = wait_on_serial3(1,1000);
            Serial.println("Listen");
            if(logic_check && (((char)Serial3.read()) == 'C')){
                state = WaitingForKey_1;
            }
            else {state = Listen; }
        }
        else if (state == WaitingForKey_1){
            logic_check = wait_on_serial3(4,1000);
            Serial.println("WaitingForKey_1");
            if(logic_check){
                ckey = uint32_from_serial3();
                Serial3.write('A');
                uint32_to_serial3(skey);
                state = WaitForAck_2; 
            }
            else {state = Listen;}
        }
        else if (state == WaitForAck_2){
            logic_check = wait_on_serial3(1,1000);
            Serial.println("WaitForAck_2");
            
            char WaitForAck_2_char = ((char)Serial3.read());

            if(logic_check){

                if(WaitForAck_2_char == 'A'){
                    state = DataExchange;
                }
                else if(WaitForAck_2_char == 'C'){
                    state = WaitingForKey_2;
                }
                else { Serial.println("please help");}
            }
            else {state = Listen;}
        }
        else if (state == WaitingForKey_2){
            Serial.println("WaitingForKey_2");
            logic_check = wait_on_serial3(4,1000);
            Serial.print("  logic_check: "); Serial.println(logic_check);
            
            if(logic_check){
                state = WaitForAck_3;
            }
            else { state = Listen; }
        }
        else if (state == WaitForAck_3){
            logic_check = wait_on_serial3(1,1000);
            
            Serial.println("WaitForAck_3");
            char WaitForAck_3_char = ((char)Serial3.read());
            if(logic_check){
                if(WaitForAck_3_char == 'C'){
                    state = WaitingForKey_2;
                }
                else if(WaitForAck_3_char == 'A'){
                    state = DataExchange;
                }
            }
            else {state = Listen; }
        }
        
    }
    //Serial.print("End state: "); Serial.println(StateNames[state]);
    Serial.println("DataExchange");
    return (state == DataExchange); //useless
}


/** Implements the Park-Miller algorithm with 32 bit integer arithmetic 
 * @return ((current_key * 48271)) mod (2^31 - 1);
 * This is linear congruential generator, based on the multiplicative
 * group of integers modulo m = 2^31 - 1.
 * The generator has a long period and it is relatively efficient.
 * Most importantly, the generator's modulus is not a power of two
 * (as is for the built-in rng),
 * hence the keys mod 2^{s} cannot be obtained
 * by using a key with s bits.
 * Based on:
 * http://www.firstpr.com.au/dsp/rand31/rand31-park-miller-carta.cc.txt
 */
uint32_t next_key(uint32_t current_key) {
  const uint32_t modulus = 0x7FFFFFFF; // 2^31-1
  const uint32_t consta = 48271; // we use that this is <=15 bits
  uint32_t lo = consta*(current_key & 0xFFFF);  
  uint32_t hi = consta*(current_key >> 16); 
  lo += (hi & 0x7FFF)<<16;
  lo += hi>>15;
  if (lo > modulus) lo -= modulus;
  return lo;
}

/* Encrypts outgoing bytes */
uint32_t encrypt(uint32_t my_PC_byte, uint32_t shared_key){
    //@TODO: incorporate stream cipher
    uint32_t encrypted_byte = ((my_PC_byte) ^ ((shared_key) % 256));
    Serial.write((char)my_PC_byte);
    Serial3.write((char)encrypted_byte);
    shared_key = next_key(shared_key);
    
    return shared_key;
}

/* Decrypts incoming bytes */
uint32_t decrypt(uint32_t incoming_byte, uint32_t shared_key){
    //@TODO: incorporate stream cipher
    uint32_t decrypted_byte = ((incoming_byte) ^ ((shared_key) % 256));
    if(decrypted_byte ==10 || decrypted_byte == 13){
        Serial.write('\n');
        Serial.write('\r');
    }
    else{Serial.write(decrypted_byte);}
    shared_key = next_key(shared_key);
    
    return shared_key;
}

int get_configuration(){
    int mode = digitalRead(13);
    return mode;
}

int main(void) {
    
    //THIS IS THE RIGHT MAIN
    
    // Initialise Arduino functionality
    init();
  
    // Attach USB for applicable processors
    #ifdef USBCON
        USBDevice.attach();
    #endif
    
    /* remember to change values and data types
     * 
     * modify pow_mod with mul_mod
     * 
     * reorder to have beginning stuff at top?
     * 
     * configure
     * generate private key (32 bits)
     *     ckey if client, skey if server
     *     still display public keys on monitor
     * handshake
     * generate shared
     * 
     * begin communication?
     *     use new encryption method
     *     must have synchronized next key on encrypt and decrypt
     * 
     */
    
     /* SETUP */
    uint32_t prime = 2147483647;
    uint32_t generator = 16807;
    uint32_t private_key = 0;
    
    pinMode(13, INPUT);
    
    bool diditwork = 0;
    // this should be done after private key made
    // gets configuration of the arduino
    // made a function in case more complication methods are used
    Serial.begin(9600);
    Serial3.begin(9600);
    int mode = get_configuration();
    Serial.print("mode: "); Serial.println(mode);
    //5V means server
    
    do{
        //@TODO: is there a better way?
        private_key = generate_private();
    }while(private_key >= prime);
    Serial.print("private_key: "); Serial.println(private_key);
    
    
    //handshake before this
    //depending on configuration do dif
    
    //~ if(digitalRead(13)==HIGH){
        skey = pow_mod(generator, private_key, prime);
        //~ Serial.print("skey: "); Serial.println(skey);
    //~ }
    //~ else{ 
        ckey = pow_mod(generator, private_key, prime); 
        //~ Serial.print("ckey: "); Serial.println(ckey);
    //~ }


    
    uint32_t shared_key = 0;
    

    if(mode){diditwork = handshake(2);}
    else {diditwork = handshake(0);}
    
    if(digitalRead(13)==HIGH){
        Serial.print("ckey: "); Serial.println(ckey);
        Serial.print("Server private_key: "); Serial.println(private_key);
        shared_key = pow_mod(ckey, private_key, prime);
    }
    else{
        Serial.print("skey: "); Serial.println(skey);
        Serial.print("Client private_key: "); Serial.println(private_key);
        shared_key = pow_mod(skey, private_key, prime);
    }
    

    Serial.print("The shared key is: "); Serial.println(shared_key);
    
    //~ if (mode == 0) { diditwork = handshake(0); } //check which it should be
    //~ else           { diditwork = handshake(9999); }
    //~ 
    
    
    //~ /* Generates private key, sometimes takes a while */
    //~ while(private_key >= prime){
        //~ //@TODO: is there a better way?
        //~ private_key = generate_private();
    //~ }
    //~ 
    
    /* Generates private key, sometimes takes a while */
    
    
    /* LOOP */
     // Serial3: communication with other board
    int incoming_byte = 0;
    int my_PC_byte = 0;
    while(true){
        
        //grab byte from other Arduino
        if(Serial3.available()){
            //decrypt byte
            incoming_byte = Serial3.read();
            shared_key = decrypt(incoming_byte, shared_key);

        }
        
        //grab byte from PC

        if(Serial.available()){
            //encrypt byte
            my_PC_byte = Serial.read();
            shared_key = encrypt(my_PC_byte, shared_key);

        }
    }
    Serial.end();
    Serial3.end();
    //Add your custom initialization here

    return 0;
}
