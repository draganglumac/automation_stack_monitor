ROOT=$(PWD)
OBJDIR=$(PWD)/bin
SRCDIR=$(PWD)/src
C_FILES=`find $(SRCDIR)  -type f -iname *.c -print`

all: clean build

gdb: clean build_gdb

build:
	cd $(SRCDIR)
	gcc	$(C_FILES) -o $(OBJDIR)/monitor -lmysqlclient -ljnxc -lpthread
	cd $(ROOT)

build_gdb:
	cd $(SRCDIR)
	gcc	$(C_FILES) -o $(OBJDIR)/monitor -lmysqlclient -ljnxc -lpthread -g
	cd $(ROOT)

clean:
	if [ -d $(OBJDIR) ]; then rm -rf $(OBJDIR); fi
	mkdir $(OBJDIR)

install:
	sudo mv bin/monitor /usr/bin/monitor
	rm -rf bin
