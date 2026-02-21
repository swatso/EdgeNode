// Debug class which allows creation of logging streams to an MQTT topic
// You can use .print() and .println() calls to log information
// Data is published in complete lines, so multiple .print() calls will be buffered until a .println() is made
// There is an optional .setNodeID() method which allows a unique decimal ID to be set (0-255d)
// If a nodeID has been set then (nn) is prepended to each line published which identifies the node sourcing the message,
// this can be helpful if several nodes are publishing to the same topic.
// You can also use .appendNodeID() to cause the /nn to be appended to the topic string, allowing each node to
// publish to a different topic

// This class has a dependancy on the MQTTComms module which provides the underlying MQTT interface
// It users the MQTTPublishMessage function to publish messages to MQTT, which uses the FreeRTOS queue system

// Four 'standard' debug streams are created at the end of the (.cpp) file which can be used for general debug and operations logging, both locally (per node) and globally (all nodes)

#ifndef debugStream_h
#define debugStream_h

#include <Arduino.h>

class debugStream : public Stream {
private:
    static const size_t BUFFER_SIZE = 64;
    static const size_t TOPIC_SIZE = 64;
    char topic[TOPIC_SIZE];
    char buffer[BUFFER_SIZE];
    char payload[BUFFER_SIZE + 5];
    int  lnodeID;
    int  ID0, ID1;
    bool appendID = false;
    size_t head = 0; // write position
    size_t tail = 0; // read position

public:
    // Constructor
    debugStream(const char* MQTTtopic);

    // Required: Return number of bytes available to read
    int available() override;

    // Required: Read one byte (-1 if none)
    int read() override;

    // Required: Peek next byte without removing it
    int peek() override;

    // Required: Write one byte
    size_t write(uint8_t c) override;

    // Write a string or buffer
    size_t write(const uint8_t *data, size_t len) override;

    // Optional: Flush (not needed for memory buffer)
    void flush() override;

    // set optional nodeID (decimal)
    void setNodeID(int nodeID);

    // request to append nodeID to topic before publishing
    void appendNodeID(bool append);
};

// pre definition of function in Communications.ino
void MQTTPublish(char* topic, char* message);

extern debugStream localDebug;
extern debugStream globalDebug;
extern debugStream localOperations;
extern debugStream globalOperations;

#endif
