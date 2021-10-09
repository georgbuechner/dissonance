aubio:
	mkdir -p deps
	# Install aubio
	wget -O deps/aubio-0.4.7.tar.bz2 https://aubio.org/pub/aubio-0.4.7.tar.bz2
	cd deps && tar xf aubio-0.4.7.tar.bz2
	cd deps/aubio-0.4.7 && ./waf configure build
	cd deps/aubio-0.4.7 && sudo ./waf install
	# sudo cp /usr/local/lib/libaubio.so.5 /usr/lib/

build:
	# Create build folder and install conan-dependencies.
	rm -rf build; mkdir build
	cd build && conan install .. --build=missing -s compiler.libcxx=libstdc++11
	# export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
	# Build projekt
	cd build && cmake ..
	cd build && make -j

setup:
	mkdir -p ~/.dissonance
	cp -r dissonance/. ~/.dissonance/
	chmod +x build/bin/dissonance
	sudo cp build/bin/dissonance /usr/bin/

install:
	make build -B
	make setup

uninstall_aubio:
	cd deps/aubio-0.4.7 && ./waf uninstall
	rm -rf deps/
	sudo rm /usr/lib/libaubio.so.5

uninstall:
	sudo rm /usr/bin/dissonance
	rm -rf build/
	rm -rf ~/.dissonance

