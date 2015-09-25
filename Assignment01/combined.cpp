/*
 *
 *  Basic encryped chat program between two Arduinos.
 * 
 *  First draft combination of various parts.
 *
 */

#include <Arduino.h>

/* Possibly change some method and variable types */ 

uint16_t generate_private(){
    //@TODO: I think it's done
    uint16_t private_key = 0;
    uint16_t read_random = 0;
    for(int i = 0; i < 16; ++i){
        
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
    //@TODO: I think it's done
    uint32_t result = 1;
    uint32_t x = base % m;
    for(uint32_t i = 0; i <=32; ++i){
        if((power&(1<<i)) != 0){
            result = (result * x) % m;
        }
        x = (x * x) % m;
    }
    return result;
}

int str2int(char* buf){
    return atoi(buf); 
}

/* User enters their partner's public key */
int enter_public(){
    //@TODO: finish this, essentially the chat example
    int my_PC_byte = 0;
    Serial.print("Enter your partner's public key: ");
     //~ while(true){
        //~ 
        //~ //grab byte from PC
        //~ my_PC_byte = Serial.read();
        //~ if(my_PC_byte != -1){
            //~ if(my_PC_byte == 10 || my_PC_byte == 13){
                //~ Serial.write('\n'); //character code line feed
                //~ Serial.write('\r'); //carriage return
                //~ break;
                //~ //Serial.write("\n\r"); //double quotes for 2+ chars
            //~ }
            //~ else {
                //~ //write to uint16 or 32
                //~ Serial.write((char)my_PC_byte);
                //~ 
            //~ }    
        //~ }
    //~ }
    //~ Serial.println("Entered number: ");
    
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
    
    
    
    
    return 0;
}

/* Encrypts outgoing bytes */
char encrypt(int my_PC_byte, int shared_key){
    //@TODO: finish this
    //~ Serial.print("Before encryption: "); Serial.write((char)my_PC_byte); Serial.println();
    char encrypted_byte = ((my_PC_byte) ^ ((shared_key) % 256));
    //~ Serial.print("After encryption: "); Serial.write((char)encrypted_byte); Serial.println();
    return encrypted_byte;
}

/* Decrypts incoming bytes */
char decrypt(int incoming_byte, int shared_key){
    //@TODO: finish this
    //~ Serial.print("Before decryption: "); Serial.write((char)incoming_byte); Serial.println();
    char decrypted_byte = ((incoming_byte) ^ ((shared_key) % 256));
    //~ Serial.print("After decryption: "); Serial.write(decrypted_byte); Serial.println();
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
    
    //checklist/basic outline:
    
    /* SETUP */
    //create 16 bit random key
    //compute public key A = (g^a) % p , p = 19211, g = 6
    //display public key A on serial monitor
    //enter public key B on keyboard (look at other monitor)
        //read as ascii keys then convert to decimal number
    //computer shared key k = B^a
    
    /* LOOP */
    //read character from serial(keyboard) if available
        //encrypt it
        //send it
    //receive an encrypted byte if available
        //decrypt the byte using shared key
        //display on serial (my) monitor
        
    /* SETUP */
    int prime = 19211;
    int generator = 6;
    uint16_t private_key = prime + 1;
    
    while(private_key >= prime){
        private_key = generate_private();
    }
    
    Serial.begin(9600);
    //~ Serial.print("private_key: "); Serial.println(private_key);
    int public_key_A = pow_mod(generator, private_key, prime); //pow_mod()
    Serial.print("Shared key A: "); Serial.println(public_key_A);
    int public_key_B = enter_public(); //while true, essential the same as loop
    Serial.print("Shared key B: "); Serial.println(public_key_B);
    int shared_key = pow_mod(public_key_B, private_key, prime);
    //~ Serial.print("The shared key is: "); Serial.println(shared_key);

    
    /* LOOP */
    //copy loop from chat, essentially complete
    Serial3.begin(9600); // Serial3: communication with other board
    int incoming_byte = 0;
    int my_PC_byte = 0;
    //int secretkey = 128;
    while(true){
        
        //grab byte from other Arduino
        incoming_byte = Serial3.read();
        if(incoming_byte != -1){
            
            //~ Serial.print(incoming_byte);
            //~ Serial.print(" ");
            //Serial.write((char)incomingByte);
          
            //decrypt here
            //int decryptedByte = incomingByte ^ secretkey;
            int decrypted_byte = decrypt(incoming_byte, shared_key);
            
            if(decrypted_byte == 10 || decrypted_byte == 13){
                Serial.write('\n'); //character code line feed
                Serial.write('\r'); //carriage return
                //Serial.write("\n\r"); //double quotes for 2+ chars
            }
            else {
                //~ Serial.print("Message sent: ");
                Serial.write(decrypted_byte);
                //~ Serial.println();
            } // write to PC
        }
        
        //grab byte from PC
        
        my_PC_byte = Serial.read();
        if(my_PC_byte != -1){
            //int byte_to_send = incoming_byte ^ secretkey;
            char byte_to_send = encrypt(my_PC_byte, shared_key);
            //~ Serial.print("Sending this byte: "); Serial.write(byte_to_send); Serial.println();
            int decrypt_check = decrypt(byte_to_send, private_key);
            Serial.write((char)my_PC_byte);
           // Serial.write((char)decrypt_check);
            Serial3.write((char)byte_to_send);

        }
    }
    Serial.end();
    Serial3.end();

    return 0;
}
