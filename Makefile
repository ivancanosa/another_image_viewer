BUILDDIR=builddir
MAIN=aiv
RELEASE=release

TARGET=$(BUILDDIR)/$(MAIN)

.PHONY: build debug run clean profile release

export CXXFLAGS=-stdlib=libc++

all: build

build: $(BUILDDIR)
	ninja -C $(BUILDDIR) && cp $(BUILDDIR)/compile_commands.json compile_commands.json

debug: $(BUILDDIR)
	ninja -C $(BUILDDIR) && cp $(BUILDDIR)/compile_commands.json compile_commands.json
	gdb -batch -ex "run" -ex "bt" $(TARGET) 2>&1 | grep -v ^"No stack."$

run: $(BUILDDIR)
	ninja -C $(BUILDDIR) && cp $(BUILDDIR)/compile_commands.json compile_commands.json
	./$(TARGET) someImages

clean:
	rm -rf $(BUILDDIR)
	rm -rf $(RELEASE)
	rm -f compile_commands.json 
	 
profile: $(BUILDDIR)
	CPUPROFILE=prof.out ./$(TARGET)
	pprof --gif $(TARGET) prof.out > output.gif && xdg-open output.gif

record: $(BUILDDIR)
	perf record ./$(TARGET) 
	
report: $(BUILDDIR)
	perf report

test: $(BUILDDIR)
	meson test -C $(BUILDDIR) && cp $(BUILDDIR)/compile_commands.json compile_commands.json

install: $(BUILDDIR)
	cp ./$(TARGET) $(HOME)/.local/bin/$(MAIN)

$(BUILDDIR): meson.build
	meson $(BUILDDIR)
