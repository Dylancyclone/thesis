# Datalogger

The purpose of this folder is to record latency between arbitrary remote desktop applications. Since these listen at the OS level instead of the application level, it is application independent.

The indended structure of this is to run the linux datalogger on the client, the windows datalogger on the server, then open the web page on the server. This way the initial input from the client is recorded, the recieved input is recorded by the server, the web interface changes color, and the client records when the color has changed.

This will record not only one way latency, but an accurate roundtrip of all the data.

This isn't as accurate as the NVIDIA Latency Display Analysis Tool (LDAT), but that is still an internal tool of NVIDIA, but it doesn't need to be, since this is intended to test the software, not the hardware in question.
