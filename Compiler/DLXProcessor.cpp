#include "DLXProcessor.h"
#include<iostream>
#include<cstdio>

using namespace std;

 int DLXProcessor::R[32];
 int DLXProcessor::PC, DLXProcessor::op, DLXProcessor::a, DLXProcessor::b, DLXProcessor::c, DLXProcessor::format;

// emulated memory
 // bytes in memory (divisible by 4)
 int DLXProcessor::M[MemSize / 4];
 const char **DLXProcessor::mnemo;

DLXProcessor::DLXProcessor()
{
	
	 mnemo = new const char*[100]{
	"ADD","SUB","MUL","DIV","MOD","CMP","ERR","ERR","OR","AND","BIC","XOR","LSH","ASH","CHK","ERR",
	"ADDI","SUBI","MULI","DIVI","MODI","CMPI","ERRI","ERRI","ORI","ANDI","BICI","XORI","LSHI","ASHI","CHKI","ERR",
	"LDW","LDX","POP","ERR","STW","STX","PSH","ERR","BEQ","BNE","BLT","BGE","BLE","BGT","BSR","ERR",
	"JSR","RET","RDI","WRD","WRH","WRL","ERR","ERR","ERR","ERR","ERR","ERR","ERR","ERR","ERR","ERR",
	"ERR","ERR","ERR","ERR","ERR","ERR","ERR","ERR","ERR","ERR","ERR","ERR","ERR","ERR","ERR","ERR" };
}



DLXProcessor::~DLXProcessor()
{
}

void DLXProcessor::load( vector<int> &program)
{

	int i;
	for (i = 0; i < program.size(); i++) {
		M[i] = program[i];
	}
	M[i] = -1; // set first opcode of first instruction after program
				// to ERR in order to detect 'fall off the edge' errors
	
}

void DLXProcessor::execute()
{
	int origc = 0; // used for F2 instruction RET
	for (int i = 0; i < 32; i++) { R[i] = 0; };
	PC = 0; R[30] = MemSize - 1;

	try {

	execloop:
		while (true) {
			R[0] = 0;
			disassem(M[PC]); // initializes op, a, b, c

			int nextPC = PC + 1;
			if (format == 2) {
				origc = c; // used for RET
				c = R[c];  // dirty trick
			}
			string line;
			switch (op) {
			case ADD:
			case ADDI:
				R[a] = R[b] + c;
				break;
			case SUB:
			case SUBI:
				R[a] = R[b] - c;
				break;
			case CMP:
			case CMPI:
				R[a] = R[b] - c; // can not create overflow
				if (R[a] < 0) R[a] = -1;
				else if (R[a] > 0) R[a] = 1;
				// we don't have to do anything if R[a]==0
				break;
			case MUL:
			case MULI:
				R[a] = R[b] * c;
				break;
			case DIV:
			case DIVI:
				R[a] = R[b] / c;
				break;
			case MOD:
			case MODI:
				R[a] = R[b] % c;
				break;
			case OR:
			case ORI:
				R[a] = R[b] | c;
				break;
			case AND:
			case ANDI:
				R[a] = R[b] & c;
				break;
			case BIC:
			case BICI:
				R[a] = R[b] & ~c;
				break;
			case XOR:
			case XORI:
				R[a] = R[b] ^ c;
				break;
				// Shifts: - a shift by a positive number means a left shift
				//         - if c > 31 or c < -31 an error is generated
			case LSH:
			case LSHI:
				if ((c < -31) || (c > 31)) {
					cout << "Illegal value " + c;
					cout << " of operand c or register c!"<<endl;
					bug(1);
				}
				if (c < 0)  R[a] = (unsigned)R[b] >> -c;
				else        R[a] = R[b] << c;
				break;
			case ASH:
			case ASHI:
				if ((c < -31) || (c > 31)) {
					cout<<"DLX.execute: Illegal value "<< c<<
						" of operand c or register c!";
					bug(1);
				}
				if (c < 0)  R[a] = R[b] >> -c;
				else        R[a] = R[b] << c;
				break;
			case CHKI:
			case CHK:
				if (R[a] < 0) {
					cout<<"DLX.execute: "<< PC * 4<< ": R[" <<a <<"] == "<<
						R[a] << " < 0";
					bug(14);
				}
				else if (R[a] >= c) {
					cout<<"DLX.execute: " << PC * 4 << ": R[" << a << "] == " <<
						R[a] << " >= " << c;
					bug(14);
				}
				break;
			case LDW:
			case LDX: // remember: c == R[origc] because of F2 format
				R[a] = M[(R[b] + c) / 4];
				break;
			case STW:
			case STX: // remember: c == R[origc] because of F2 format
				M[(R[b] + c) / 4] = R[a];
				break;
			case POP:
				R[a] = M[R[b] / 4];
				R[b] = R[b] + c;
				break;
			case PSH:
				R[b] = R[b] + c;
				M[R[b] / 4] = R[a];
				break;
			case BEQ:
				if (R[a] == 0) nextPC = PC + c;
				if ((nextPC < 0) || (nextPC > MemSize / 4)) {
					cout<<4 * nextPC<< " is no address in memory (0.."
						<< MemSize << ").";
					bug(40);
				}
				break;
			case BNE:
				if (R[a] != 0) nextPC = PC + c;
				if ((nextPC < 0) || (nextPC > MemSize / 4)) {
					cout<<4 * nextPC<< " is no address in memory (0.."
						<< MemSize << ").";
					bug(41);
				}
				break;
			case BLT:
				if (R[a] < 0) nextPC = PC + c;
				if ((nextPC < 0) || (nextPC > MemSize / 4)) {
					cout<<4 * nextPC << " is no address in memory (0.."
						<< MemSize << ").";
					bug(42);
				}
				break;
			case BGE:
				if (R[a] >= 0) nextPC = PC + c;
				if ((nextPC < 0) || (nextPC > MemSize / 4)) {
					cout<<4 * nextPC << " is no address in memory (0.."
						<< MemSize << ").";
					bug(43);
				}
				break;
			case BLE:
				if (R[a] <= 0) nextPC = PC + c;
				if ((nextPC < 0) || (nextPC > MemSize / 4)) {
					cout<<4 * nextPC << " is no address in memory (0.."
						<< MemSize << ").";
					bug(44);
				}
				break;
			case BGT:
				if (R[a] > 0) nextPC = PC + c;
				if ((nextPC < 0) || (nextPC > MemSize / 4)) {
					cout<<4 * nextPC<< " is no address in memory (0.."
						<< MemSize << ").";
					bug(45);
				}
				break;
			case BSR:
				R[31] = (PC + 1) * 4;
				nextPC = PC + c;
				break;
			case JSR:
				R[31] = (PC + 1) * 4;
				nextPC = c / 4;
				break;
			case RET:
				if (origc == 0) return;//break execloop; // remember: c==R[origc]
				if ((c < 0) || (c > MemSize)) {
					cout<<c << " is no address in memory (0.."
						<< MemSize << ").";
					bug(49);
				}
				nextPC = c / 4;
				break;
			case RDI:
				cout<<"?: ";
				
				cin >> line;
				R[a] = atoi(line.c_str());
				break;
			case WRD:
				cout<<R[b] << "  ";
				break;
			case WRH:
				
				printf("%x ", R[b]);
				break;
			case WRL:
				cout << endl;
				break;
			case ERR:
				cout<<"Program dropped off the end!";
				bug(1);
				break;
			default:
				cout<<"DLX.execute: Unknown opcode encountered!";
				bug(1);
			}
			PC = nextPC;
		}

	}
	catch (exception e) {
		cout<<"failed at "<< PC * 4 << ",   " << disassemble(M[PC]);
	}

}

void DLXProcessor::disassem(int instructionWord)
{
	op = (unsigned)instructionWord >> 26; // without sign extension
	switch (op) {

		// F1 Format
	case BSR:
	case RDI:
	case WRD:
	case WRH:
	case WRL:
	case CHKI:
	case BEQ:
	case BNE:
	case BLT:
	case BGE:
	case BLE:
	case BGT:
	case ADDI:
	case SUBI:
	case MULI:
	case DIVI:
	case MODI:
	case CMPI:
	case ORI:
	case ANDI:
	case BICI:
	case XORI:
	case LSHI:
	case ASHI:
	case LDW:
	case POP:
	case STW:
	case PSH:
		format = 1;
		a = ((unsigned)instructionWord >> 21) & 0x1F;
		b = ((unsigned)instructionWord >> 16) & 0x1F;
		c = (short)instructionWord; // another dirty trick
		break;

		// F2 Format
	case RET:
	case CHK:
	case ADD:
	case SUB:
	case MUL:
	case DIV:
	case MOD:
	case CMP:
	case OR:
	case AND:
	case BIC:
	case XOR:
	case LSH:
	case ASH:
	case LDX:
	case STX:
		format = 2;
		a = ((unsigned)instructionWord >> 21) & 0x1F;
		b = ((unsigned)instructionWord >> 16) & 0x1F;
		c = instructionWord & 0x1F;
		break;

		// F3 Format
	case JSR:
		format = 3;
		a = -1; // invalid, for error detection
		b = -1;
		c = instructionWord & 0x3FFFFFF;
		break;

		// unknown instruction code
	default:
		cout<<"Illegal instruction! (" << PC << ")";
	}
}

std::string DLXProcessor::disassemble(int instructionWord)
{
	disassem(instructionWord);
	string line = string(mnemo[op]) + "  ";

	switch (op) {

	case WRL:
		return line += "\n";
	case BSR:
	case RET:
	case JSR:
		return line += to_string(c) + "\n";
	case RDI:
		return line += to_string(a) + "\n";
	case WRD:
	case WRH:
		return line += to_string(b) + "\n";
	case CHKI:
	case BEQ:
	case BNE:
	case BLT:
	case BGE:
	case BLE:
	case BGT:
	case CHK:
		return line += to_string(a) + " " + to_string(c) +string( "\n");
	case ADDI:
	case SUBI:
	case MULI:
	case DIVI:
	case MODI:
	case CMPI:
	case ORI:
	case ANDI:
	case BICI:
	case XORI:
	case LSHI:
	case ASHI:
	case LDW:
	case POP:
	case STW:
	case PSH:
	case ADD:
	case SUB:
	case MUL:
	case DIV:
	case MOD:
	case CMP:
	case OR:
	case AND:
	case BIC:
	case XOR:
	case LSH:
	case ASH:
	case LDX:
	case STX:
		line += to_string(a) + " " + to_string(b);
		return line += " " + to_string(c) + string("\n");;
	default:
		return line += "\n";
	}
}

int DLXProcessor::assemble(int op)
{
	if (op != WRL) {
		cout<<"DLX.assemble: the only instruction without arguments is WRL!"<<endl;
		bug(1);
	}
	return F1(op, 0, 0, 0);
}

int DLXProcessor::assemble(int op, int arg1)
{
	switch (op) {

		// F1 Format
	case BSR:
		return F1(op, 0, 0, arg1);
	case RDI:
		return F1(op, arg1, 0, 0);
	case WRD:
	case WRH:
		return F1(op, 0, arg1, 0);

		// F2 Format
	case RET:
		return F2(op, 0, 0, arg1);

		// F3 Format
	case JSR:
		return F3(op, arg1);
	default:
		cout<<"DLX.assemble: wrong opcode for one arg instruction!"<<endl;
		bug(1);
		return -1; // java forces this senseless return statement!
				   // I'm thankful for every sensible explanation.
	}
}

int DLXProcessor::assemble(int op, int arg1, int arg2)
{
	switch (op) {

		// F1 Format
	case CHKI:
	case BEQ:
	case BNE:
	case BLT:
	case BGE:
	case BLE:
	case BGT:
		return F1(op, arg1, 0, arg2);

		// F2 Format
	case CHK:
		return F2(op, arg1, 0, arg2);

	default:
		cout<<"DLX.assemble: wrong opcode for two arg instruction!"<<endl;
		bug(1);
		return -1;
	}
}

int DLXProcessor::assemble(int op, int arg1, int arg2, int arg3)
{
	switch (op) {

		// F1 Format
	case ADDI:
	case SUBI:
	case MULI:
	case DIVI:
	case MODI:
	case CMPI:
	case ORI:
	case ANDI:
	case BICI:
	case XORI:
	case LSHI:
	case ASHI:
	case LDW:
	case POP:
	case STW:
	case PSH:
		return F1(op, arg1, arg2, arg3);

		// F2 Format
	case ADD:
	case SUB:
	case MUL:
	case DIV:
	case MOD:
	case CMP:
	case OR:
	case AND:
	case BIC:
	case XOR:
	case LSH:
	case ASH:
	case LDX:
	case STX:
		return F2(op, arg1, arg2, arg3);

	default:
		cout<<"DLX.assemble: wrong opcode for three arg instruction!"<<endl;
		bug(1);
		return -1;
	}
}

int DLXProcessor::F1(int op, int a, int b, int c)
{
	if (c < 0) c ^= 0xFFFF0000;
	if ((a & ~0x1F | b & ~0x1F | c & ~0xFFFF) != 0) {
		cout<<"Illegal Operand(s) for F1 Format."<<endl;
		bug(1);
	}
	return op << 26 | a << 21 | b << 16 | c;
}

int DLXProcessor::F2(int op, int a, int b, int c)
{
	if ((a & ~0x1F | b & ~0x1F | c & ~0x1F) != 0) {
		cout<<"Illegal Operand(s) for F2 Format."<<endl;
		bug(1);
	}
	return op << 26 | a << 21 | b << 16 | c;
}

int DLXProcessor::F3(int op, int c)
{
	if ((c < 0) || (c > MemSize)) {
		cout<<"Operand for F3 Format is referencing " <<
			"non-existent memory location."<<endl;
		bug(1);
	}
	return op << 26 | c;
}

void DLXProcessor::bug(int n)
{
	int x;
	cout<<"bug number: " << n<<endl;
	try { cin >> x; }
	catch (exception ee) { ; }
	exit(n);
}
