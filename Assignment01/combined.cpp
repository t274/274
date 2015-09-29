/*
 *  Thomas Tetz
 *  Brett Baker
 *  EA2
 *
 *  Assignment 1.1
 *
 *  Basic encryped chat program between two Arduinos.
 *
 */

#include <Arduino.h>

/* Generates private key for the user */
uint16_t generate_private(){
    uint16_t private_key = 0;
    uint16_t read_random = 0;
    for(int i = 0; i < 16; ++i){
        
        //takes least significant bit of fluctuating analog signal
        read_random = analogRead(1);
        read_random = read_random % 2;
        
        private_key = private_key + read_random;
        private_key = private_key << 1;
   
        delay(50);
    }
    return private_key;
}

/* Generates public and shared keys */
uint32_t pow_mod(uint32_t base, uint32_t power, uint32_t m){
    uint32_t result = 1;
    uint32_t x = base % m;
    //simplified exponent function using modulus rules
    for(uint32_t i = 0; i <=32; ++i){
        if((power&(1<<i)) != 0){
            result = (result * x) % m;
        }
        x = (x * x) % m;
    }
    return result;
}

/* Converts string to integer */
int str2int(char* buf){
    return atoi(buf); 
}

/* User enters their partner's public key */
int enter_public(){
    int my_PC_byte = 0;
    Serial.print("Enter your partner's public key: ");
    const int LINE_LENGTH = 128;
    int result = 0;
    char line[LINE_LENGTH + 1];
    int i = 0;
    while(i < LINE_LENGTH){
        int my_PC_byte = Serial.read();
        if(my_PC_byte != -1){
            if(my_PC_byte == 10 || my_PC_byte == 13){
                break;
            }
            line[i] = my_PC_byte;
            ++i;
            Serial.print((char)my_PC_byte);
        }
    }
    line[i] = 0;
    result = str2int(line);
    Serial.println();
    return result;
}

/* Encrypts outgoing bytes */
char encrypt(int my_PC_byte, int shared_key){
    char encrypted_byte = ((my_PC_byte) ^ ((shared_key) % 256));
    return encrypted_byte;
}

/* Decrypts incoming bytes */
char decrypt(int incoming_byte, int shared_key){
    char decrypted_byte = ((incoming_byte) ^ ((shared_key) % 256));
    return decrypted_byte;
}

/* Main */
int main(void) {
    // Initialise Arduino functionality
    init();
  
    // Attach USB for applicable processors
    #ifdef USBCON
         USBDevice.attach();
    #endif
    
    /* SETUP */
    int prime = 19211;
    int generator = 6;
    uint16_t private_key = prime + 1;
    
    /* Generates private key, sometimes takes a while */
    while(private_key >= prime){
        private_key = generate_private();
    }
    
    Serial.begin(9600);
    int public_key_A = pow_mod(generator, private_key, prime);
    Serial.print("Public key A: "); Serial.println(public_key_A);
    int public_key_B = enter_public();
    Serial.print("Public key B: "); Serial.println(public_key_B);
    int shared_key = pow_mod(public_key_B, private_key, prime);
    Serial.print("The shared key is: "); Serial.println(shared_key);

    
    /* LOOP */
    Serial3.begin(9600); // Serial3: communication with other board
    int incoming_byte = 0;
    int my_PC_byte = 0;
    while(true){
        
        //grab byte from other Arduino
        incoming_byte = Serial3.read();
        if(incoming_byte != -1){
            //decrypt byte
            int decrypted_byte = decrypt(incoming_byte, shared_key);
            if(decrypted_byte == 10 || decrypted_byte == 13){
                Serial.write('\n'); //character code line feed
                Serial.write('\r'); //carriage return
            }
            else {
                Serial.write(decrypted_byte);
            }
        }
        
        //grab byte from PC
        my_PC_byte = Serial.read();
        if(my_PC_byte != -1){
            //encrypt byte
            char byte_to_send = encrypt(my_PC_byte, shared_key);
            Serial.write((char)my_PC_byte);
            Serial3.write((char)byte_to_send);
        }
    }
    Serial.end();
    Serial3.end();

    return 0;
}
