// debug class which provides a streams interface for code debugging at sends out
// debug information via MQTT (Communications Module)
//
#include "debugStream.h"
#include "MQTTServices.h"
#include "gpio.h"
#include <cstring>

    // Constructor - accepts topic string
    debugStream ::debugStream(const char* MQTTtopic)
    {
        lnodeID = -1;                        // indicates global message as opposed to local (nodeID) message
        int i;
        for(i=0; i<strlen(MQTTtopic); i++)topic[i]=MQTTtopic[i];
    }

    // Required: Return number of bytes available to read
    int debugStream::available() {

        return (head >= tail) ? (head - tail) : (BUFFER_SIZE - tail + head);
    }

    // Required: Read one byte (-1 if none)
    int debugStream::read() {
        if (available() == 0) return -1;
        char c = buffer[tail];
        tail = (tail + 1) % BUFFER_SIZE;
        return c;
    }

    // Required: Peek next byte without removing it
    int debugStream::peek() {
        if (available() == 0) return -1;
        return buffer[tail];
    }

    // Required: Write one byte
    size_t debugStream::write(uint8_t c) {
        size_t nextHead = (head + 1) % BUFFER_SIZE;
        if (nextHead == tail) {
            // Buffer full — overwrite oldest data
            tail = (tail + 1) % BUFFER_SIZE;
        }
        buffer[head] = c;
        head = nextHead;
        return 1;
    }

    // Optional: Write a string or buffer
    size_t debugStream::write(const uint8_t *data, size_t len) 
    {
        size_t written = 0;
        for (size_t i=0; i < len; i++) 
        {
            written += write(data[i]);
        }

        if(data[len-1]==10)                             // keep buffering until we receive a linefeed char
        {
            // Build the string by adding the node ID at the front if nodeID has been set
            int i=0;
            if(lnodeID >= 0)
            {
                // If a node ID has been set then build this into the front of the supplied topic
                payload[i++]=40;        // open brackets
                payload[i++]=ID0;
                payload[i++]=ID1;
                payload[i++]=41;        // close brackets
                payload[i++]=32;        // space character
            }
             while (available()) 
            {
                payload[i++] = read();
                payload[i]=0;
            }
            MQTTMessagePayload messagePayload;
            strncpy(messagePayload.topic, topic, sizeof(messagePayload.topic) - 1);
            messagePayload.topic[sizeof(messagePayload.topic) - 1] = '\0';
            strncpy(messagePayload.message, payload, sizeof(messagePayload.message) - 1);
            messagePayload.message[sizeof(messagePayload.message) - 1] = '\0';
            if(GPIOState() == true)MQTTPublishMessage(messagePayload);  // publish the message to MQTT
        }
        return written;
    }

    // Optional: Flush (not needed for memory buffer)
    void debugStream::flush() {}

    void  debugStream::setNodeID(int nodeID)
    {
        // accepts an optional NodeID
        // If set then this is converted to HEXASCII and appended to the topic (/nn) and prepended to the payload (nn) 
        lnodeID = nodeID;
        if(nodeID > -1)
        {
            // convert and store nodeID as a two digit ACSIIHex 
            ID0= nodeID/16 + 0x30;
            if(ID0 > 57)ID0+=7;
            ID1= nodeID % 16 + 0x30;
            if(ID1 > 57)ID1+=7;
            if(appendID == true)
            {
                // update topic address to append node ID to the path
                int i;
                for(i=0; topic[i] != 0; i++);
                topic[i++]=47;        // slash character
                topic[i++]=ID0;
                topic[i++]=ID1;
            }
        }
    }

    void debugStream::appendNodeID(bool append)
    {
        // Set true to have the nodeID automatically appended to the topic before publication
        appendID = append;
        if(append == true)setNodeID(lnodeID);
    }

// Create the four standard debug streams
debugStream localDebug("iot/debug");
debugStream globalDebug("iot/debug");
debugStream localOperations("iot/operations");
debugStream globalOperations("iot/operations");