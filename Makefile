audio:
	mkdir deps
	# Install aubio
	wget -O deps/aubio-0.4.7.tar.bz2 https://aubio.org/pub/aubio-0.4.7.tar.bz2
	cd deps && tar xf aubio-0.4.7.tar.bz2
	cd deps/aubio-0.4.7 && ./waf configure build
	cd deps/aubio-0.4.7 && sudo ./waf install
	sudo cp /usr/local/lib/libaubio.so.5 /usr/lib/

build:
	# Create build folder and install conan-dependencies.
	rm -rf build
	mkdir build
	conan profile update settings.compiler.libcxx=libstdc++11 default
	cd build && conan install ..
	# Create deps folder and install other dependencies.
	# Build projekt
	cd build && cmake ..
	cd build && make

install:
	mkdir -p ~/.dissonance
	cp -r dissonance/* ~/.dissonance/
	chmod +x build/bin/dissonance
	cp build/bin/dissonance /usr/bin/

uninstall: 
	cd deps/aubio-0.4.7 && ./waf uninstall
	rm -rf deps/
	sudo rm /usr/lib/libaubio.so.5
	sudo rm /usr/bin/dissonance
	rm -rf build/
	rm -rf ~/.dissonance

