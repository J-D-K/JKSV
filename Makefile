.PHONY	:= all FsLib JKSV

all	:= FsLib JKSV

# Only want to build the lib. Not bothering with the testing app
FsLib:
	$(MAKE) -C FsLib/Switch/FsLib

JKSV:
	$(MAKE) -C JKSV
