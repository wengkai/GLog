CC = g++
CFLAG = -std=c++17 -Wall -O2
LIBS = -lsqlite3
SRC = adif
INC = -I$(SRC)
OBJS = main.o $(SRC)/logdb.o $(SRC)/record.o $(SRC)/file.o 

ap: $(OBJS)
	$(CC) $(CFLAG) $(LIBS) $(OBJS) -o $@

.cpp.o:
	$(CC) $(CFLAG) $(INC) -c $< -o $@

clean:
	rm -f $(OBJS) ap

file: adif/record.cpp adif/file.cpp
	g++ -std=c++17 -liconv -Wall -O2 -D_FILE_TEST_ -I$(SRC) $(SRC)/file.cpp $(SRC)/record.cpp -o file
