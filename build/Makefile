
all: build

build:
	spectool -C ~/rpmbuild/SOURCES --get-files --sources --patches launchy.spec
	rpmbuild -bb launchy.spec