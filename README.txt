This project was my introduction to network programming with C. 
The architecture is a bit unconcentional because there are
two producers and two consumers. The server must run first because
it must bind to a socket and listen on a given port. The client connects
to the ip on the given port then the two processes may communictate. 
