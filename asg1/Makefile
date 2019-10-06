# $Id: Makefile,v 1.1 2019-04-03 17:34:13-07 - - $
# Wai Chun Leung
# wleung11

WARNING     = -Wall -Wextra -Wpedantic -Wshadow -Wold-style-cast
COMPILECPP = g++ -std=gnu++17 -g -O0 ${WARNING}
MAKEDEPCPP = g++ -std=gnu++17 -MM
VALGRIND   = valgrind --leak-check=full --show-reachable=yes

MKFILE     = Makefile
DEPFILE    = Makefile.dep
SOURCES    = cppstrtok.cpp auxlib.cpp string_set.cpp
OBJECTS    = ${SOURCES:.cpp=.o}
EXECBIN    = oc
HEADERS    = auxlib.h string_set.h
CHECKINS   = ${SOURCES} ${MKFILE} ${DEPFILE} ${HEADERS}

all : ${EXECBIN}

${EXECBIN} : ${OBJECTS}
	${COMPILECPP} -o${EXECBIN} ${OBJECTS}

%.o : %.cpp
	${COMPILECPP} -c $<

ci :
	cid + ${CHECKINS}
	checksource ${CHECKINS}

clean :
	- rm ${OBJECTS}

spotless : clean
	- rm ${EXECBIN} ${DEPFILE} \

${DEPFILE} :
	${MAKEDEPCPP} ${SOURCES} >${DEPFILE}

deps :
	- rm ${DEPFILE}
	${MAKE} --no-print-directory ${DEPFILE}

include Makefile.dep


