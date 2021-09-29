build:
	rm -rf build
	mkdir build
	cd build && conan install ..
	cd build && cmake ..
	cd build && make

install:
	if [ -d "~/.disonance" ]; then mkdir ~/.disonance; fi
	cp -r data/ ~/.disonance/
	chmod +x build/bin/disonance
	cp build/bin/disonance /usr/share/

