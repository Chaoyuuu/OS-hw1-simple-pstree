# HW1-simple-pstree
input pid to find entire process tree/all sibling/all anscestors of a process

# Compile
		cd module
		make
		make ins
		cd ..
		make

# Run
		./simple_pstree [-c|-s|-p][pid]

* -c: Display the entire process tree
* -s: Display all sibling of a process  
* -p: Display all ancestors of a process

# Clean
		make clean
		cd module
		make rm
		make clean
