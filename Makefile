.PHONY	: all FsLib JKSV clean

all:	FsLib JKSV

# Only want to build the lib. Not bothering with the testing app
FsLib:
	$(MAKE) -C FsLib/Switch/FsLib

JKSV:
	$(MAKE) -C JKSV

clean:
	$(MAKE) -C FsLib/Switch/FsLib clean
	$(MAKE) -C JKSV clean
