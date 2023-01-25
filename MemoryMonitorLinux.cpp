/*
 * MemoryMonitor.cpp
 *
 *  Created on: Nov 22, 2022
 *      Author: Llanyro
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <csignal>
#include <limits>
#include <unistd.h>

#include <iostream>

#define PROC_DEF "/proc/"
#define MEM_DEF "/mem"

char* getProcess(const char* str) {
	char* proc_mem = new char[sizeof(PROC_DEF) + sizeof(MEM_DEF) + strlen(str)];
	strcat(proc_mem, PROC_DEF);
	strcat(proc_mem, str);
	strcat(proc_mem, MEM_DEF);
	return proc_mem;
}
#pragma endregion
#pragma region Internal

// Nodes for perpetual loop
#pragma region List
template<class T>
class Node {
	public:
		T a;
		Node<T>* next;
		Node<T>* prev;
		Node(T a, Node<T>* next, Node<T>* prev) {
			this->a = a;
			this->next = next;
			this->prev = prev;
		}
		Node(T a) : Node<T>(a, this, this) {}
		~Node() {
			if (this->a != nullptr)
				delete this->a;
			this->a = nullptr;
		}
		void linkLeft(Node<T>* node) {
			node->prev = this->prev;
			node->next = this;

			this->prev->next = node;
			this->prev = node;
		}
};
#pragma endregion
#pragma region Memory Management
/*
	Base class for memory management
*/
class MemoryDataBase {
	protected:
		unsigned long addr;
		const char* caddr;
		const char* nameID;
		unsigned int nameIDLength;
	public:
		MemoryDataBase(const char* caddr, const char* nameID, unsigned int nameIDLength) {
			this->addr = strtoul(caddr, NULL, 16);
			this->caddr = caddr;
			this->nameID = nameID;
			this->nameIDLength = nameIDLength;
		}
		MemoryDataBase(const char* caddr, const char* nameID) : MemoryDataBase(caddr, nameID, strlen(nameID)) {}
		virtual ~MemoryDataBase() {}

		unsigned long getAddr() const { return this->addr; }
		const char* getCaddr()  const { return this->caddr; }
		const char* getNameID()  const { return this->nameID; }


		virtual unsigned int getMemSize() const = 0;
		virtual void printMem() = 0;
		virtual void* getMem() = 0;
};
/*
	Template class for memory management
	Auto prints any c/c++ basic type
*/
template<class T>
class MemoryData : public MemoryDataBase {
	protected:
		T lastMem;
		T lastMemPrint;
	public:
		MemoryData(const char* caddr, const char* nameID) : MemoryDataBase(caddr, nameID) {}
		MemoryData(const char* caddr, const char* nameID, unsigned int nameIDLength) : MemoryDataBase(caddr, nameID, nameIDLength) {}
		~MemoryData() {}

		virtual unsigned int getMemSize() const override { return sizeof(T); }
		virtual void* getMem() override { return &this->lastMem; }
		bool updateMem() {
			bool result = false;
			if (lastMem != lastMemPrint) {
				result = true;
				lastMemPrint = lastMem;
			}
			return result;
		}
		virtual void printMem() {
			if (this->updateMem()) {
				printf("[%s] (%.*s): ", this->caddr, this->nameIDLength, this->nameID);
				std::cout << this->lastMemPrint << std::endl;
			}
		}
};
#pragma endregion


/*
	Basic static singleton to store the data in a linked list to monitor
	The program auto deletes this strucure on exit, and cleans the list
*/
template<class T>
class Singleton {
#pragma region Singleton
public:
	static Singleton* getInstance() {
		static Singleton<T> instance;
		return &instance;
	}
#pragma endregion


public:
	Node<T>* root;
	unsigned int length;

protected:
	Singleton() {
		this->root = nullptr;
		this->length = 0;
	}
	~Singleton() {
		Node<T>* tmp = this->root;
		while (this->length > 0) {
			tmp = tmp->next;
			delete tmp->prev;
			this->length--;
		}
	}
public:
	void add(T val) {
		Node<T>* node = new Node<T>(val);
		if (this->length == 0)
			this->root = node;
		else
			this->root->linkLeft(node);
		this->length++;
	}
	Node<T>* getRoot() const { return this->root; }


};

//#define CONTROLLER Singleton<MemoryDataBase*>
using Controller = Singleton<MemoryDataBase*>;


/*
	Struct to store data of the operation find first char
*/
#pragma region Substring
typedef struct substring_t {
	const char* start = nullptr;
	const char* end = nullptr;	// Character included
	unsigned int pos = 0;		//
} substring;

bool getSubString(const char* str, char c, substring* s) {
	bool result = true;
	s->start = str;
	s->end = strchr(str, c);
	if (result = (s->end != NULL))
		s->pos = s->end - s->start;
	return result;
}
#pragma endregion


void exit_monitor(int signl_no);


/*
	Given a string with the format:
		[TYPE]_[TAG]_[POINTER] ~> b_print_b435223
	Parses the string into Memory Class to manage the data
*/
MemoryDataBase* getMemoryReaderByData(const char* str) {
	MemoryDataBase* result = nullptr;

	// Getting type
	substring type;
	if (!getSubString(str, '_', &type)) {
		fprintf(stderr, "Error reading type of this argument: %s", str);
		exit_monitor(-2);
	}
	substring id;
	if (!getSubString(type.end + 1, '_', &id)) {
		fprintf(stderr, "Error reading type of this argument: %s", str);
		exit_monitor(-2);
	}

	const char* addr = id.end + 1;

	if (strncmp(type.start, "f", type.pos) == 0)
		result = new MemoryData<float>(addr, id.start, id.pos);
	else if (strncmp(type.start, "b", type.pos) == 0)
		result = new MemoryData<bool>(addr, id.start, id.pos);
	else if (strncmp(type.start, "i", type.pos) == 0)
		result = new MemoryData<int>(addr, id.start, id.pos);
	else if (strncmp(type.start, "d", type.pos) == 0)
		result = new MemoryData<double>(addr, id.start, id.pos);
	else if (strncmp(type.start, "c", type.pos) == 0)
		result = new MemoryData<char>(addr, id.start, id.pos);
	else if (strncmp(type.start, "p", type.pos) == 0)
		result = new MemoryData<void*>(addr, id.start, id.pos);
	
	return result;
}

// Function to exit program normaly
void exit_monitor(int signl_no) {
	printf("\nThe interrupt signal is (%i).\n", signl_no);
	if(signl_no != 0)
		exit(signl_no);
}
// Prints Help panel of this program
void printHelp() {
	printf("This monitor needs at least 3 parameters:\n");
	printf("Example:\n\t./monitor 246343 d_3ffb260\n");
	printf("First parameter needs to be the process id\n");
	printf("Parameters to monitor (3rd and previous parameters) needs to start with the type and continue with the address of the memory\n");
	printf("Type list:\n");
	printf("b: bool\n");
	printf("d: double\n");
	printf("f: float\n");
	printf("i: int\n");
	printf("c: char\n");
	printf("ui: unsigned int\n");
	printf("...\n");
}

#pragma endregion


int main(int argc, char** argv) {
	signal(SIGINT, exit_monitor);

	if (argc < 3)
		printHelp();
	else {
		// Parsing memory process to get process
		char* proc_mem = getProcess(argv[1]);

		fprintf(stdout, "Opening process: %s\n", proc_mem);
		int fd_proc_mem = open(proc_mem, O_RDWR);

		// If process doesnt exists or user do not have privileges
		if (fd_proc_mem == -1) {
			fprintf(stderr, "Could not open %s\n", proc_mem);
			delete proc_mem;
			exit_monitor(-1);
		}
		delete proc_mem;




		// Getting memory singleton to monitor...
		Controller* c = Controller::getInstance();

		// If we are going to read by a file
		if (strcmp(argv[2], "-f") == 0) {
			fprintf(stderr, "Sorry this function is in WIP\n");
		}
		// If memory is includeb by commandline
		else {
			MemoryDataBase* m = nullptr;
			/*
				For each argument to monitor, the string is parsed and stored to being monitored
			*/
			for (int i = 2; i < argc; ++i) {
				m = getMemoryReaderByData(argv[i]);
				if (m == nullptr)
					fprintf(stderr, "Ignoring value %s, not valid or defined type", argv[i]);
				else
					c->add(m);
			}
		}


		/*
			Monitor part
			Cause we want a permantent loop to monitor memory,
				we monitor all memoy provided while:
					* the main program closes
					* This program closes (Ctrl + C)
					* Any memory is unabled to been readed

		*/
		Node<MemoryDataBase*>* tmpn = c->getRoot();
		MemoryDataBase* tmp = nullptr;
		bool res = true;
		do {
			tmp = tmpn->a;
			lseek(fd_proc_mem, tmp->getAddr(), SEEK_SET);
			res = (read(fd_proc_mem, tmp->getMem(), tmp->getMemSize()) > 0);
			tmp->printMem();
			tmpn = tmpn->next;
		} while (res);

		printf("Last address that has been tried to read is: %s\n", tmp->getCaddr());
	}
	exit_monitor(0);
	return 0;
}
