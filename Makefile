build:
	# Create build folder and install conan-dependencies.
	rm -rf build
	mkdir build
	cd build && conan install ..
	# Create deps folder and install other dependencies.
	mkdir deps
	# Install aubio
	wget -O deps/aubio-0.4.7.tar.bz2 https://aubio.org/pub/aubio-0.4.7.tar.bz2
	cd deps && tar xf aubio-0.4.7.tar.bz2
	cd deps/aubio-0.4.7 && ./waf configure build
	cd deps/aubio-0.4.7 && sudo ./waf install
	sudo cp /usr/local/lib/libaubio.so.5 /usr/lib/
	# Build projekt
	cd build && cmake ..
	cd build && make

install:
	if [ -d "~/.disonance" ]; then mkdir ~/.disonance; fi
	cp -r data/ ~/.disonance/
	chmod +x build/bin/disonance
	cp build/bin/disonance /usr/bin/

uninstall: 
	cd deps/aubio-0.4.7 && ./waf uninstall
	rm -rf deps/
	sudo rm /usr/lib/libaubio.so.5
	rm -rf build/
	rm -rf ~/.disonance


