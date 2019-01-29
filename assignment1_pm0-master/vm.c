#include <stdio.h>
#include "vm.h"
#include "data.h"
#include <stdlib.h>

/* ************************************************************************************ */
/* Declarations                                                                         */
/* ************************************************************************************ */

/**
 * Recommended design includes the following functions implemented.
 * However, you are free to change them as you wish inside the vm.c file.
 * */
void initVM(VirtualMachine*);

int readInstructions(FILE*, Instruction*);

void dumpInstructions(FILE*, Instruction*, int numOfIns);

int getBasePointer(int *stack, int currentBP, int L);

void dumpStack(FILE*, int* stack, int sp, int bp);

int executeInstruction(VirtualMachine* vm, Instruction ins, FILE* vmIn, FILE* vmOut);

/* ************************************************************************************ */
/* Global Data and misc structs & enums                                                 */
/* ************************************************************************************ */

/**
 * allows conversion from opcode to opcode string
 * */
const char *opcodes[] = 
{
    "illegal", // opcode 0 is illegal
    "lit", "rtn", "lod", "sto", "cal", // 1, 2, 3 ..
    "inc", "jmp", "jpc", "sio", "sio",
    "sio", "neg", "add", "sub", "mul",
    "div", "odd", "mod", "eql", "neq",
    "lss", "leq", "gtr", "geq"
};

enum { CONT, HALT };

/* ************************************************************************************ */
/* Definitions                                                                          */
/* ************************************************************************************ */

/**
 * Initialize Virtual Machine
 * */
void initVM(VirtualMachine* vm)
{
	int i = 0;
	
    if(vm)
    {
		// set initial stack and register file values to 0
		for (i = 0; i < 16; i++)
		{
			vm->RF[i] = 0;
		}
		for (i = 0; i < MAX_STACK_HEIGHT; i++)
		{
			vm->stack[i] = 0;
		}
		
		// sp = 0, bp = 1, pc = 0,
		// set pointers and program counters to appropriate values
		vm->BP = 1;
		vm->SP = 0;
		vm->PC = 0;
		vm->IR = 0;
    }
}

/**
 * Fill the (ins)tructions array by reading instructions from (in)put file
 * Return the number of instructions read
 * */
int readInstructions(FILE* in, Instruction* ins)
{
    // Instruction index
    int i = 0;
    
    while(fscanf(in, "%d %d %d %d", &ins[i].op, &ins[i].r, &ins[i].l, &ins[i].m) != EOF)
    {
        i++;
    }

    // Return the number of instructions read
    return i;
}

/**
 * Dump instructions to the output file
 * */
void dumpInstructions(FILE* out, Instruction* ins, int numOfIns)
{
    // Header
    fprintf(out,
        "***Code Memory***\n%3s %3s %3s %3s %3s \n",
        "#", "OP", "R", "L", "M"
        );

    // Instructions
    int i;
    for(i = 0; i < numOfIns; i++)
    {
        fprintf(
            out,
            "%3d %3s %3d %3d %3d \n", // formatting
            i, opcodes[ins[i].op], ins[i].r, ins[i].l, ins[i].m
        );
    }
}

/**
 * Returns the base pointer for the lexiographic level L
 * */
int getBasePointer(int *stack, int currentBP, int L)
{
    int b1; //find base L levels down
    b1 = currentBP;
    while (L > 0)
    {
        b1 = stack[b1 + 1];
        L--;
    }
    return b1;
}

// Function that dumps the whole stack into output file
// Do not forget to use '|' character between stack frames
void dumpStack(FILE* out, int* stack, int sp, int bp)
{
    if(bp == 0)
        return;

    // bottom-most level, where a single zero value lies
    if(bp == 1)
    {
        fprintf(out, "%3d ", 0);
    }

    // former levels - if exists
    if(bp != 1)
    {
        dumpStack(out, stack, bp - 1, stack[bp + 2]);            
    }

    // top level: current activation record
    if(bp <= sp)
    {
        // indicate a new activation record
        fprintf(out, "| ");

        // print the activation record
        int i;
        for(i = bp; i <= sp; i++)
        {
            fprintf(out, "%3d ", stack[i]);
        }
    }
}

/**
 * Executes the (ins)truction on the (v)irtual (m)achine.
 * This changes the state of the virtual machine.
 * Returns HALT if the executed instruction was meant to halt the VM.
 * .. Otherwise, returns CONT
 * */
int executeInstruction(VirtualMachine* vm, Instruction ins, FILE* vmIn, FILE* vmOut)
{
    switch(ins.op)
    {
		// LIT
		case 1 :
			vm->RF[ins.r] = ins.m;
			break;
			
		// RTN
		case 2 :
			vm->SP = vm->BP - 1;
			vm->BP = vm->stack[vm->SP + 3];
			vm->PC = vm->stack[vm->SP + 4];
			break;
			
		// LOD
		case 3 :
			vm->RF[ins.r] = vm->stack[getBasePointer(vm->stack, vm->BP, ins.l) + ins.m];
			break;
			
		// STO
		case 4 :
			vm->stack[getBasePointer(vm->stack, vm->BP, ins.l) + ins.m] = vm->RF[ins.r];
			break;
			
		// CAL
		case 5 :
			vm->stack[vm->SP + 1] = 0;
			vm->stack[vm->SP + 2] = getBasePointer(vm->stack, vm->BP, ins.l);
			vm->stack[vm->SP + 3] = vm->BP;
			vm->stack[vm->SP + 4] = vm->PC;
			vm->BP = vm->SP + 1;
			vm->PC = ins.m;
			break;
			
		// INC
		case 6 :
			vm->SP = vm->SP + ins.m;
			break;
			
		// JMP
		case 7 :
			vm->PC = ins.m;
			break;
			
		// JPC
		case 8 :
			if (vm->RF[ins.r] == 0)
			{
				vm->PC = ins.m;
			}
			break;
			
		// SIO 1
		case 9 :
			fprintf(vmOut, "%d", vm->RF[ins.r]);
			break;
		
		// SIO 2
		case 10 :
			fscanf(vmIn, "%d", &vm->RF[ins.r]);
			break;
			
		// SIO 3
		case 11 :
			return HALT;
			break;
		
		// NEG
		case 12 :
			vm->RF[ins.r] = 0 - vm->RF[ins.l];
			break;
			
		// ADD
		case 13 :
			vm->RF[ins.r] = vm->RF[ins.l] + vm->RF[ins.m];
			break;
			
		// SUB
		case 14 :
			vm->RF[ins.r] = vm->RF[ins.l] - vm->RF[ins.m];
			break;
			
		// MUL
		case 15 :
			vm->RF[ins.r] = vm->RF[ins.l] * vm->RF[ins.m];
			break;
			
		// DIV
		case 16 :
			vm->RF[ins.r] = vm->RF[ins.l] / vm->RF[ins.m];
			break;
			
		// ODD
		case 17 :
			vm->RF[ins.r] = vm->RF[ins.r] % 2;
			break;
			
		// MOD
		case 18 :
			vm->RF[ins.r] = vm->RF[ins.l] % vm->RF[ins.m];
			break;
			
		// EQL
		case 19 :
			vm->RF[ins.r] = (vm->RF[ins.l] == vm->RF[ins.m]);
			break;
			
		// NEQ
		case 20 :
			vm->RF[ins.r] = (vm->RF[ins.l] != vm->RF[ins.m]);
			break;
			
		// LSS
		case 21 :
			vm->RF[ins.r] = (vm->RF[ins.l] < vm->RF[ins.m]);
			break;
			
		// LEQ
		case 22 :
			vm->RF[ins.r] = (vm->RF[ins.l] <= vm->RF[ins.m]);
			break;
			
		// GTR
		case 23 :
			vm->RF[ins.r] = (vm->RF[ins.l] > vm->RF[ins.m]);
			break;
			
		// GEQ
		case 24 :
			vm->RF[ins.r] = (vm->RF[ins.l] >= vm->RF[ins.m]);
			break;
        default:
            fprintf(stderr, "Illegal instruction?");
            return HALT;
    }

    return CONT;
}

/**
 * inp: The FILE pointer containing the list of instructions to
 *         be loaded to code memory of the virtual machine.
 * 
 * outp: The FILE pointer to write the simulation output, which
 *       contains both code memory and execution history.
 * 
 * vm_inp: The FILE pointer that is going to be attached as the input
 *         stream to the virtual machine. Useful to feed input for SIO
 *         instructions.
 * 
 * vm_outp: The FILE pointer that is going to be attached as the output
 *          stream to the virtual machine. Useful to save the output printed
 *          by SIO instructions.
 * */
void simulateVM(
    FILE* inp,
    FILE* outp,
    FILE* vm_inp,
    FILE* vm_outp
    )
{
	int numOfIns = 0, status = 0, instrNum = 0, instrBeingExecuted = 0;
	Instruction ins;
	Instruction ins_array[MAX_CODE_LENGTH];
	
    // Read instructions from file
	numOfIns = readInstructions(inp, ins_array);

    // Dump instructions to the output file
	dumpInstructions(outp, ins_array, numOfIns);

    // Before starting the code execution on the virtual machine,
    // .. write the header for the simulation part (***Execution***)
    fprintf(outp, "\n***Execution***\n");
    fprintf(
        outp,
        "%3s %3s %3s %3s %3s %3s %3s %3s %3s \n",         // formatting
        "#", "OP", "R", "L", "M", "PC", "BP", "SP", "STK" // titles
    );

    // Create a virtual machine
    VirtualMachine *vm = malloc(sizeof(VirtualMachine));
	
    // Initialize the virtual machine
    initVM(vm);

    // Fetch&Execute the instructions on the virtual machine until halting
    while (status == CONT)
    {
        // Fetch
        ins.op = ins_array[vm->PC].op;
		ins.r = ins_array[vm->PC].r;
		ins.l = ins_array[vm->PC].l;
		ins.m = ins_array[vm->PC].m;

        // Advance PC - before execution!
        instrBeingExecuted = vm->PC++;

        // Execute the instruction
        status = executeInstruction(vm, ins, vm_inp, vm_outp);

        // Print current state
        // Following is a possible way of printing the current state
        // .. where instrBeingExecuted is the address of the instruction at vm
        // ..  memory and instr is the instruction being executed.
        fprintf(
            outp,
            "%3d %3s %3d %3d %3d %3d %3d %3d ",
            instrBeingExecuted, // place of instruction at memory
             opcodes[ins.op], ins.r, ins.l, ins.m, // instruction info
             vm->PC, vm->BP, vm->SP // vm info
        );

        // Print stack info
		dumpStack(outp, vm->stack, vm->SP, vm->BP);

        fprintf(outp, "\n");
    }

    // Above loop ends when machine halts. Therefore, dump halt message.
    fprintf(outp, "HLT\n");
    return;
}