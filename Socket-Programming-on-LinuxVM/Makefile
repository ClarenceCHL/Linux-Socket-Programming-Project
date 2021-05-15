all: scheduler.cpp hospitalA.cpp hospitalB.cpp hospitalC.cpp client.cpp
	g++ -o scheduler scheduler.cpp

	g++ -o hospitalA hospitalA.cpp

	g++ -o hospitalB hospitalB.cpp

	g++ -o hospitalC hospitalC.cpp

	g++ -o client client.cpp

.PHONY: scheduler
scheduler:
	./scheduler

.PHONY: hospitalA
hospitalA:
	./hospitalA

.PHONY: hospitalB
hospitalB:
	./hospitalB

.PHONY: hospitalC
hospitalC:
	./hospitalC

.PHONY: client
client:
	./client