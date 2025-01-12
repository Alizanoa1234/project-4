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
<<<<<<< HEAD
=======

	

# all: ping traceroute discovery
# ping: ping.c
# 	gcc ping.c -o parta
# traceroute: traceroute.c
# 	gcc traceroute.c -o traceroute
# discovery: discovery.c
# 	gcc discovery.c -o partb

# new: clean all

# clean:
# 	rm -f *. ping traceroute parta partb


#chat code
# # Compiler and flags
# CC = gcc  # The compiler
# CFLAGS = -Wall -Wextra -g  # Compilation flags for better error reporting and debugging

# # Executables
# TARGETS = ping traceroute discovery  # The programs to be built

# # Default target (build all programs)
# all: $(TARGETS)

# # Build the ping program
# ping: ping.c
# 	$(CC) $(CFLAGS) -o ping ping.c

# # Build the traceroute program
# traceroute: traceroute.c
# 	$(CC) $(CFLAGS) -o traceroute traceroute.c

# # Build the discovery program
# discovery: discovery.c
# 	$(CC) $(CFLAGS) -o discovery discovery.c

# # Clean up all object files and executables
# clean:
# 	rm -f *.o ping traceroute discovery

# # Rebuild everything from scratch
# new: clean all
>>>>>>> origin/master
