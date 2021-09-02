.PHONY:clean
Exes	=ts
Objects	=ts.o 
Sources =ts.cpp
CommonSources=../common/my_encrpty.cpp
CommonObjects=my_encrpty.o

CC=g++
CFlags=-Wall 
CP=\cp -f
DirPath=../ts

Main:$(Exes)
	$(CP) $(Exes) $(DirPath)

$(Exes):$(Objects) $(CommonObjects)
	$(CC) $(CFlags) -o $(Exes) $(Objects) $(CommonObjects)

$(Objects):
	$(CC) -c -o $(Objects) $(Sources)
$(CommonObjects):
	$(CC) -c -o $(CommonObjects) $(CommonSources)
clean:
	rm *.o $(Exes) -f
