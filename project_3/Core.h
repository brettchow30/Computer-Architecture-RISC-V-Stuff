#ifndef __CORE_H__
#define __CORE_H__

#include "Instruction_Memory.h"

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define BOOL bool

typedef uint8_t Byte;
typedef int64_t Signal;
typedef int64_t Register;

struct Core;
typedef struct Core Core;
typedef struct Core
{
    Tick clk; // Keep track of core clock
    Addr PC; // Keep track of program counter

    // What else you need? Data memory? Register file?
    Instruction_Memory *instr_mem;
   
    Byte data_mem[1024]; // data memory

    Register reg_file[32]; // register file.

    bool (*tick)(Core *core);
}Core;

Core *initCore(Instruction_Memory *i_mem);
bool tickFunc(Core *core);

// FIXME. Implement the following functions in Core.c
// FIXME (1). Control Unit.
typedef struct ControlSignals
{
    Signal Branch;
    Signal MemRead;
    Signal MemtoReg;
    Signal ALUOp;
    Signal MemWrite;
    Signal ALUSrc;
    Signal RegWrite;
}ControlSignals;

//----------------------------------------------------My Structures-------------------------------------------------
typedef struct ALUio //ad a structure to store the output of the ALU
{
    Signal zero;
    Signal ALU_result;
    Signal ALUctrl_signal;
}ALUio;

typedef struct InstructionElements //ad a structure to store the output of the ALU
{
    Signal func3;
    Signal func7;
    Signal imm;
    Signal rd;
    Signal rs_1;
    Signal rs_2;
    Signal opcode;
}InstructionElements;

void genDecode(InstructionElements *inst, int current_instruct[]);
char* identifyType(InstructionElements *inst);
void reverseTwosComplement(int immediate_array[], int size);
void converttoDecimal(int immediate_array[], InstructionElements *inst, int size);
//----------------------------------------------------My Structures - END-------------------------------------------------

void ControlUnit(Signal input, ControlSignals *signals); 

// FIXME (2). ALU Control Unit.
Signal ALUControlUnit(Signal ALUOp,
                      Signal Funct7,
                      Signal Funct3);

// FIXME (3). Imme. Generator
void ImmeGen(InstructionElements *inst, int current_instruct[], char *type); //changed return type of ImmeGen

// FIXME (4). ALU
void ALU(Signal input_0,
         Signal input_1,
         Signal ALU_ctrl_signal,
         Signal *ALU_result,
         Signal *zero);

// (4). MUX
Signal MUX(Signal sel,
           Signal input_0,
           Signal input_1);

// (5). Add
Signal Add(Signal input_0,
           Signal input_1);

// (6). ShiftLeft1
Signal ShiftLeft1(Signal input);

#endif
