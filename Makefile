all: ping traceroute discovery

ping: ping.c
	gcc -Wall -Wextra -g ping.c -o ping  # Tab here, not spaces!

traceroute: traceroute.c
	gcc -Wall -Wextra -g traceroute.c -o traceroute  # Tab here

discovery: discovery.c
	gcc -Wall -Wextra -g discovery.c -o discovery  # Tab here

new: clean all

clean:
	rm -f *.o ping traceroute discovery
