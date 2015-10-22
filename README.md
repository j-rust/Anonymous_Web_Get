# Anonymous_Web_Get

It is assumed the file containing all possible stepping stones (chainlist.txt) has one host and port per line,
with the the host and port delimited by one tab character.  
i.e.:  
  
>4  
>129.82.45.59	20000  
>129.82.47.209	25000  
>129.82.47.223	30000  
>129.82.47.243	35000  
  
Packet Format: 406 bytes total.  4 bytes for file length, 2 bytes for packet length, up to 400 bytes of data  
