all : 
	g++ -o Debug/lemon_mincut -L. -lemon -ltinyxml lemonTest.cpp TimeManager.cpp SimpGraph.cpp libtinyxml.a