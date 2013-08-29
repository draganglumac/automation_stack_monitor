ROOT=$(PWD)
OBJDIR=$(PWD)/bin
SRCDIR=$(PWD)/src
C_FILES=`find . -type f -iname *.c -print`

all: clean build

build:
	cd $(SRCDIR)
	gcc	$(C_FILES) -o $(OBJDIR)/monitor
	cd $(ROOT)

clean:
	if [ -d $(OBJDIR) ]; then rm -rf $(OBJDIR); fi
	mkdir $(OBJDIR)

install:
	sudo mv bin/monitor /usr/bin/monitor

