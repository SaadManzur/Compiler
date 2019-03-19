#pragma once
#include<string>
#include<vector>
#define MemSize 10000

class DLXProcessor
{
	
public:
	// processor state variables
	static int R[32];
	static int PC, op, a, b, c, format;

	// emulated memory
	 // bytes in memory (divisible by 4)
	static unsigned int M[MemSize / 4];

	static const char **mnemo;
	 static void load(std::vector<unsigned int> &program);

	 static void execute();

	// Mnemonic-to-Opcode mapping
	
	static const int ADD = 0;
	static const int SUB = 1;
	static const int MUL = 2;
	static const int DIV = 3;
	static const int MOD = 4;
	static const int CMP = 5;
	static const int OR = 8;
	static const int AND = 9;
	static const int BIC = 10;
	static const int XOR = 11;
	static const int LSH = 12;
	static const int ASH = 13;
	static const int CHK = 14;

	static const int ADDI = 16;
	static const int SUBI = 17;
	static const int MULI = 18;
	static const int DIVI = 19;
	static const int MODI = 20;
	static const int CMPI = 21;
	static const int ORI = 24;
	static const int ANDI = 25;
	static const int BICI = 26;
	static const int XORI = 27;
	static const int LSHI = 28;
	static const int ASHI = 29;
	static const int CHKI = 30;

	static const int LDW = 32;
	static const int LDX = 33;
	static const int POP = 34;
	static const int STW = 36;
	static const int STX = 37;
	static const int PSH = 38;

	static const int BEQ = 40;
	static const int BNE = 41;
	static const int BLT = 42;
	static const int BGE = 43;
	static const int BLE = 44;
	static const int BGT = 45;
	static const int BSR = 46;
	static const int JSR = 48;
	static const int RET = 49;

	static const int RDI = 50;
	static const int WRD = 51;
	static const int WRH = 52;
	static const int WRL = 53;

	static const int ERR = 63; // error opcode which is insertered by loader 
							   // after end of program code

	static void disassem(int instructionWord);

	static std::string disassemble(int instructionWord);

	static int assemble(int op);

	static int assemble(int op, int arg1);

	static int assemble(int op, int arg1, int arg2);

	static int assemble(int op, int arg1, int arg2, int arg3);

	static int F1(int op, int a, int b, int c);

	static int F2(int op, int a, int b, int c);

	static int F3(int op, int c);

	static void bug(int n);

	DLXProcessor();
	~DLXProcessor();
};

