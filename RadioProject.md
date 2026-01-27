# GPS Transmitter Protocol
> Credit goes to Patrick McGuire. The original document can be found 
> [here](https://docs.google.com/document/d/1Z5hUQjqS5lblr6D8eGP7msMxBeI7Vp8RdR-bDFRKnfU/edit?tab=t.0#heading=h.jqz3i8slg90p)

## Background
When sending data between computers in various ways, such as over radio, USB or
Serial, you have to define standards for how they will talk to each other. With 
the GPS we saw a simple text based protocol, that looked something like this: 

`$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A`

Where every message started with a “$”, ended with a “*XX” and had data fields 
separated by commas in the middle. Transmitting as text works, but is 
inefficient. We will define a protocol for the radio that uses binary data. 
Instead of starting with “$” we will start with 1 byte of data with a set 
numerical value. Everything in the message will be represented numerically 
instead of textually.

## Protocol
The following describes a basic protocol for transmitting data between two 
radios. 

`[Start Flag] [Type] [Payload Length] [Payload] [CRC] [End Flag]`

| Byte Name      | Size (bytes) | Value                                                                                          |
|----------------|--------------|------------------------------------------------------------------------------------------------|
| Start Flag     | 1            | Always `0xB5`                                                                                  |
| Type           | 1            | This describes what type of data is in the message. Types: GPS Position = `0x1` Status = `0x2` |
| Payload Length | 4            | The value of this field is the number of bytes in the payload (0 - 255)                        |
| Payload        | L            | This is the actual data. See below for details for each message type.                          |
| CRC            | 1            | This value is used to validate the rest of the message. See below for details on calculating.  |
| End Flag       | 1            | Always `0x62`                                                                                  |

The start flag and end flag allows us to identify which messages are ours and
which messages are random noise or someone else's. Ideally, the start and end 
flag are chosen such that it cannot appear at all at any point within the 
message payload, until the end. 

In this protocol, we will assume that the values chosen are rare enough that 
they do not appear within the payload itself. However, this is not a reliable 
method. A valid solution for this could be byte stuffing. This requires finding 
every time the start or end flag appears, then adding an escape byte (sequence)
to modify that byte to avoid having a false start/end. Your decoder will then
have to undo the escape sequence operation on its end. This is the method used 
in Avionics' Protobuf-based messaging format, however, there are definitely 
other valid strategies.

The Payload's max size should be `4` bytes, as specified by the protocol 
description.

## Endianness
Data is transmitted in little endian format (meaning that it stores the least 
significant bit at the smallest memory address). Your arduino already does this 
with variables, so you don’t have to worry about this.

## Payloads
### GPS Position
`[Latitude] [Longitude] [Satellites]`

| Field           | Size (bytes) | Value                                                                                                                            |
|-----------------|--------------|----------------------------------------------------------------------------------------------------------------------------------|
| Latitude 1 - 8  | 8            | Latitude in degrees (decimal). Use type double in your code, which should be 8 bytes long.                                       |
| Longitude 1 - 8 | 8            | Longitude in degrees (decimal). Use type double in your code, which should be 8 bytes long.                                      |
| Sats 1 - 4      | 4            | The number of visible GPS satellites. Use type int32_t in your code, which is 4 bytes long (or 32 bits, hence the name int32_t). |

### Status
Status messages should be text based, and varied in length. Start by randomly 
sending “Status good” and “Hello”. Text is stored as ASCII in C++/Arduino, a 
standard for associating numbers to characters. Each character will take up one 
byte. See here: https://www.asciitable.com/ for more on ASCII.

## CRC
CRCs can help check for the integrity of a message, e.g. if it is what the
sender wanted to send. *However*, it does not provide any guarantees against
malicious tampering. Go take a security class if you're interested!

Below is a rough example of how to calculate it, however, it is not in any way
standard, or even a robust CRC.

```C++
/*
 * “data” should contain every byte from the `Type` byte to the last `Payload`
 * byte, in order. “Length” should be the number of bytes in data
 */
void calculateCrc(uint8_t *data, uint8_t length) {
   uint8_t CK_A = 0;		// MUST be type uint8_t
   
   for (// Iterate over beach byte in data) {
       CK_A = CK_A + byte;
   }

   return {CK_A, CK_B};		// Return or use the outputs somehow
}

```

## Implementation

