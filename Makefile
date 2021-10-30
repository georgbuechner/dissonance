#Variables 
AUBIO_VERSION=0.4.7
AUBIO_FRAMEWORK=aubio_frame_work_$(AUBIO_VERSION)
UNAME_S := $(shell uname -s)

aubio_apple: 
	# Download aubio frameworl for mac-os
	mkdir -p deps 
	wget -O deps/$(AUBIO_FRAMEWORK).zip https://aubio.org/bin/$(AUBIO_VERSION)/aubio-$(AUBIO_VERSION).darwin_framework.zip 
	# Unzip aubio-framework and delete zip
	unzip deps/$(AUBIO_FRAMEWORK).zip -d $(AUBIO_FRAMEWORK) && rm deps/$(AUBIO_FRAMEWORK).zip
	# Copy headers to `src/aubio`. 
	rm -rf src/aubio; mkdir src/aubio
	cp -r $(AUBIO_FRAMEWORK)/aubio-$(AUBIO_VERSION).darwin_framework/aubio.framework/Headers/* src/aubio/
	rm -rf $(AUBIO_FRAMEWORK)

aubio_general:
	mkdir -p deps
	# Install aubio
	wget -O deps/aubio-$(AUBIO_VERSION).tar.bz2 https://aubio.org/pub/aubio-$(AUBIO_VERSION).tar.bz2
	# sudo ln -sf /usr/bin/python3 /usr/bin/python
	cd deps && tar xf aubio-$(AUBIO_VERSION).tar.bz2 && rm aubio-$(AUBIO_VERSION).tar.bz2
	cd deps/aubio-$(AUBIO_VERSION) && ./waf configure build
	cd deps/aubio-$(AUBIO_VERSION) && sudo ./waf install

aubio:
	make aubio_general
ifeq ($(UNAME_S),Darwin)
	make aubio_apple
endif

build:
	# Create build folder and install conan-dependencies.
	rm -rf build; mkdir build
ifeq ($(UNAME_S),Linux)
	cd build && conan install .. --build=missing -s compiler.libcxx=libstdc++11
else
	cd build && conan install .. --build=missing
endif
	# Build projekt
	cd build && cmake ..
	cd build && make -j

setup:
	rm -rf ~/.dissonance; mkdir -p ~/.dissonance
	cp -r dissonance/. ~/.dissonance/
	chmod +x build/bin/dissonance
	sudo cp build/bin/dissonance /usr/local/bin/dissonance_bin
	chmod +x dissonance.sh
	sudo cp dissonance.sh /usr/local/bin/dissonance
	# For macos move terminfo
ifeq ($(UNAME_S),Darwin)
	mkdir -p ~/.terminfo/
	sudo cp -r /usr/share/terminfo/78 ~/.terminfo/
endif

install:
	make build -B
	make setup

uninstall_aubio:
	cd deps/aubio-$(AUBIO_VERSION) && ./waf uninstall
	rm -rf deps/

uninstall_dissonance: 
	sudo rm /usr/local/bin/dissonance
	sudo rm /usr/local/bin/dissonance
	rm -rf build/
	rm -rf ~/.dissonance

uninstall:
	make uninstall_dissonance
	make uninstall_aubio
