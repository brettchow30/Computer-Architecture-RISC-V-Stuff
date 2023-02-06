#include "Core.h"
#include <math.h>
//#include <imagination.h>

/*Project3 TODO:
1) Implement the new instructions in Parser.c and Core.c
*/
Core *initCore(Instruction_Memory *i_mem)
{
    Core *core = (Core *)malloc(sizeof(Core));

    core->clk = 0;
    core->PC = 0;
    core->instr_mem = i_mem;
    core->tick = tickFunc;

    //Initializing Data memory to zero
    // core->data_mem[0] = 16; 
    // core->data_mem[8] = 128;
    // core->data_mem[16] = 8;
    // core->data_mem[32] = 4;

    core->data_mem[0] = 5; 
    core->data_mem[8] = 1;
    core->data_mem[16] = 2;
    core->data_mem[32] = 4;
    //Initializing Register File

    core->reg_file[0] = 0; //x0 
    // core->reg_file[10] = 0; //base address
    core->reg_file[22] = 0; //loop condition
    core->reg_file[23] = 0; //loop condition
    // core->reg_file[1] = 16; //loop condition

    //core->reg_file[2] = 2; //x2 shift 2

    return core;
}

//FIXME, implement this function
bool tickFunc(Core *core)
{
    unsigned instruction = core->instr_mem->instructions[core->PC / 4].instruction;
    ControlSignals *signals = (ControlSignals *)malloc(sizeof(ControlSignals));
    ALUio *alu = (ALUio *)malloc(sizeof(ALUio));
    InstructionElements *inst = (InstructionElements *)malloc(sizeof(InstructionElements));

    //Reading instruction from instruction memory
    printf("\nInstruction at PC: %lu\n", core->PC);
    int current_instruct[31];
    unsigned mask = (1 << 31);
    for (int i = 31; i >= 0; i--){ 
        if (instruction & mask){
            //printf("1 ");
            current_instruct[i] = 1;
        }else{ 
            //printf("0 ");
            current_instruct[i] = 0;
        }
        mask >>= 1;
    }
    //printf("\n");
    //--------------------------------------------- Decoding the instruction binary format --------------------------------------------------------
    inst->opcode = 0;
    for(int i = 0; i <= 6; i++){
        inst->opcode += current_instruct[i]*pow(2, i);
    }
    char *type;
    type = identifyType(inst);

    if(!strcmp(type, "R")){
        printf("Instruction is R type \n");
        genDecode(inst,  current_instruct);
        printf("Decoding results = %ld %ld %ld %ld \n", inst->func7, inst->rs_2, inst->rs_1, inst->rd);
    }
    else if(!strcmp(type, "I")){
        printf("Instruction is I type \n");
        genDecode(inst,  current_instruct);
        ImmeGen(inst, current_instruct, type);
        inst->func7 = 0;
        printf("Decoding results =  %ld %ld %ld %ld \n", inst->imm, inst->rs_1, inst->func3, inst->rd);
    }
    else if(!strcmp(type, "SB")){
        printf("Instruction is SB type \n");
        genDecode(inst,  current_instruct);
        ImmeGen(inst, current_instruct, type);
        inst->func7 = 0; //SB-type has no func7
        inst->imm = ShiftLeft1(inst->imm); //Shift immediate value to the left
        printf("Decoding results =  %ld %ld %ld %ld \n", inst->imm, inst->rs_2, inst->rs_1, inst->func3);
    }
    else if(!strcmp(type, "UJ")){
        printf("Instruction is UJ type \n");
        genDecode(inst,  current_instruct);
        ImmeGen(inst, current_instruct, type);
        inst->func7 = 0; 
        inst->rs_1 = 0;
        inst->rs_2 = 0;
        inst->imm = ShiftLeft1(inst->imm); //Shift immediate value to the left
    }
    else if(!strcmp(type, "S")){
        printf("Instruction is S type \n");
        genDecode(inst,  current_instruct);
        ImmeGen(inst, current_instruct, type); 
        inst->func7 = 0; 
        printf("Decoding results =  %ld %ld %ld %ld \n", inst->imm, inst->rs_2, inst->rs_1, inst->func3);
    }
    

    // //-------------------------------------------------- Connect DataPath Components ---------------------------------------------------------------

    //Generate the Control Unit
    ControlUnit(inst->opcode, signals);

    //Generate CTRL signal
    alu->ALUctrl_signal = ALUControlUnit(signals->ALUOp, inst->func7, inst->func3);

    //Initialize ALU
    alu->ALU_result = 0;
    ALU(core->reg_file[inst->rs_1], MUX(signals->ALUSrc, core->reg_file[inst->rs_2], inst->imm), alu->ALUctrl_signal, &alu->ALU_result, &alu->zero);

    
    //write output to register/memory if required
    if(signals->RegWrite){
        core->reg_file[inst->rd] = MUX(signals->MemtoReg, alu->ALU_result, core->data_mem[alu->ALU_result]);
    }
    if(signals->MemWrite){
        core->data_mem[alu->ALU_result] = core->reg_file[inst->rs_2]; //MEM[immediate + rs_1] = Reg[rs_2]
    }
    

    //Incrementing PC --> change PC based on bne output
    if(inst->opcode == 111){//JAL store PC+4
       core->reg_file[inst->rd] = core->PC + 4;
       alu->ALU_result = inst->imm;
    }
    if(inst->opcode == 103){//jalr - set the PC to the one stored in x1
        core->PC = core->reg_file[inst->rs_1];
    }
    else if(signals->Branch && alu->zero){ //zero holds the result for the branch instruction
        printf("Branch to: %ld \n", inst->imm);
        core->PC = Add(core->PC, inst->imm);
    }else{
        core->PC += 4;
    }


    // Are we reaching the final instruction?
    if (core->PC > core->instr_mem->last->addr)
    {   
        //print results
        printf("x22= %ld \n",  core->reg_file[22]);
        printf("x11 = %ld \n",  core->reg_file[11]);
        printf("x1 = %ld \n", core->reg_file[1]);
        //printf("x4 = %ld \n",  core->reg_file[4]);
        // printf("data memory [24] = %d \n", core->data_mem[24]);
        free(signals);
        free(alu);
        free(inst);
        return false;
    }
    return true;
}

void ControlUnit(Signal input, ControlSignals *signals){
    
    if (input == 51){// For R-type
        signals->ALUSrc = 0;
        signals->MemtoReg = 0;
        signals->RegWrite = 1;
        signals->MemRead = 0;
        signals->MemWrite = 0;
        signals->Branch = 0;
        signals->ALUOp = 2;
    }
    else if (input == 3){ //ld instruction
        signals->ALUSrc = 1;
        signals->MemtoReg = 1;
        signals->RegWrite = 1;
        signals->MemRead = 1;
        signals->MemWrite = 0;
        signals->Branch = 0;
        signals->ALUOp = 0;
    }
    else if (input == 19){ //addi and slli instructions
        signals->ALUSrc = 1;
        signals->MemtoReg = 0;
        signals->RegWrite = 1;
        signals->MemRead = 0;
        signals->MemWrite = 0;
        signals->Branch = 0;
        signals->ALUOp = 2;
    }
    else if (input == 99){//bne/bge/beq instruction - use subtract
        signals->ALUSrc = 0;
        signals->MemtoReg = 0;
        signals->RegWrite = 0;
        signals->MemRead = 0;
        signals->MemWrite = 0;
        signals->Branch = 1;
        signals->ALUOp = 1;
    }
    else if (input == 35){//sd 
        signals->ALUSrc = 1;
        signals->MemtoReg = 0;
        signals->RegWrite = 0;
        signals->MemRead = 0;
        signals->MemWrite = 1;
        signals->Branch = 0;
        signals->ALUOp = 0;
    }
    else if (input == 103){//jalr branch to immediate + rs1
        signals->ALUSrc = 1;
        signals->MemtoReg = 0;
        signals->RegWrite = 0;
        signals->MemRead = 0;
        signals->MemWrite = 0;
        signals->Branch = 0; //we have a custom "branch"
        signals->ALUOp = 3;
    }
    else if (input == 111){//jal branch to immediate + store PC+4
        signals->ALUSrc = 1;
        signals->MemtoReg = 0;
        signals->RegWrite = 1;
        signals->MemRead = 0;
        signals->MemWrite = 0;
        signals->Branch = 1;
        signals->ALUOp = 3;
    }
}


Signal ALUControlUnit(Signal ALUOp, Signal Funct7, Signal Funct3){

    // For add;addi;jalr
    if (ALUOp == 2 && Funct7 == 0 && Funct3 == 0)
    {
        return 2;//add
    }
    if (ALUOp == 3 && Funct7 == 0 && Funct3 == 0)//FOR JAL ALU does nothing
    {
        return 3;
    }
    //for ld and sd - add immediate to reg
    if (ALUOp == 0 && Funct7 == 0 && Funct3 == 3)
    {
        return 2;//add
    }
    //for slli/sll
    if (ALUOp == 2 && Funct7 == 0 && Funct3 == 1)
    {
        return 9;//shift
    }
    //-------SB--------
    //bne
    if (ALUOp == 1 && Funct7 == 0 && Funct3 == 1)
    {
        return 6;//subtract if a-b != 0 --> zero = 1
    }
    //beq
    if (ALUOp == 1 && Funct7 == 0 && Funct3 == 0)
    {
        return 7;//subtract if a-b == 0 --> zero = 1
    }
    //bge
    if (ALUOp == 1 && Funct7 == 0 && Funct3 == 5)
    {
        return 8;//subtract if a-b > 0 --> zero = 1
    }
}

void ALU(Signal input_0, Signal input_1, Signal ALU_ctrl_signal, Signal *ALU_result, Signal *zero)
{   
    // For addition
    if (ALU_ctrl_signal == 2)
    {
        *ALU_result = (input_0 + input_1);
        *zero = 1; //used for unconditional branching
    }
    if (ALU_ctrl_signal == 3)
    {
        *zero = 1; //JAL - use to branch
    }
    //Different Subtract Operations
    if (ALU_ctrl_signal == 6)//bne operation
    {
        *ALU_result = (input_0 - input_1);
        if (*ALU_result != 0){ 
            *zero = 1; 
        }else{ 
            *zero = 0; 
        }
    }
    if (ALU_ctrl_signal == 7)//beq operation
    {
        *ALU_result = (input_0 - input_1);
        if (*ALU_result == 0){ 
            *zero = 1; 
        }else{ 
            *zero = 0; 
        }
    }
    if (ALU_ctrl_signal == 8)//bge operation
    {
        *ALU_result = (input_0 - input_1);
        if (*ALU_result >= 0){ 
            *zero = 1; 
        }else{ 
            *zero = 0; 
        }
    }
    //SHIFT LEFT FUNCTION --> ALU control returns 1000
    if (ALU_ctrl_signal == 9)
    {
        printf("Shifting by this many units = %ld \n", input_1);
        *ALU_result = (input_0*pow(2,input_1));
    }
}

// Imme. Generator - has two cases SB-type and I-type
void ImmeGen(InstructionElements *inst, int current_instruct[], char *type){ //Use this for I-type?
    int power = 0;

    if(!strcmp(type, "SB")){//generate imm for SB-type
        int size = 12;
        int immediate_array[size];

        //Extract immediate value:
        immediate_array[11] = current_instruct[7];
        immediate_array[0] = 0; //not used - later we shift

        int indx = 1;
        for(int i = 8; i <= 11; i++){
            immediate_array[indx] = current_instruct[i];
            indx++;
        }
        indx = 5;
        for(int i = 25; i <= 30; i++){
            immediate_array[indx] = current_instruct[i];
            indx++;
        }
        immediate_array[12] = current_instruct[31];
        
        if(immediate_array[12] == 1){//the immediate value is negative
            reverseTwosComplement(immediate_array, size); 
            converttoDecimal(immediate_array, inst, size);
            inst->imm = inst->imm - 2*(inst->imm);
            
        }else{//immediate value is positive
            converttoDecimal(immediate_array, inst, size);
        }
    }
    else if(!strcmp(type, "I")){//generate imm for I-type
        power = 0;
        for(int i = 20; i <= 31; i++){
            inst->imm += current_instruct[i]*pow(2, power);
            power ++;
        }
    }
    else if(!strcmp(type, "UJ")){
        int size = 20;
        int immediate_array[size];
        //Extract immediate value:
        immediate_array[11] = current_instruct[20];
        immediate_array[20] = current_instruct[31];
        immediate_array[0] = 0; //not used - later we shift

        int indx = 12;
        for(int i = 12; i <= 19; i++){
            immediate_array[indx] = current_instruct[i];
            indx++;
        }

        indx = 1;
        for(int i = 21; i <= 30; i++){
            immediate_array[indx] = current_instruct[i];
            indx++;
        }

        if(immediate_array[20]==1){
            reverseTwosComplement(immediate_array, size);
            converttoDecimal(immediate_array, inst, size);
            inst->imm = inst->imm - 2*(inst->imm); 
        }
        else{
            converttoDecimal(immediate_array, inst, size);
        }

    }else if(!strcmp(type, "S")){
        int size = 11;
        int immediate_array[size];
        //Extract immediate value:
        immediate_array[11] = current_instruct[20];
        immediate_array[20] = current_instruct[31];
        immediate_array[0] = 0; //not used - later we shift

        int indx = 5;
        for(int i = 24; i <= 31; i++){
            immediate_array[indx] = current_instruct[i];
            indx++;
        }

        indx = 0;
        for(int i = 6; i <= 10; i++){
            immediate_array[indx] = current_instruct[i];
            indx++;
        }
        converttoDecimal(immediate_array, inst, size);
    }
}

void converttoDecimal(int immediate_array[], InstructionElements *inst, int size){
    int power = 0;
    for(int i = 1; i<=size; i++){
        inst->imm += immediate_array[i]*pow(2, power);
        power++;
    }
}
void reverseTwosComplement(int immediate_array[], int size){
    int invert_flag = 0;
    for(int i = 1; i<= size; i++){//invert all bits
        if(immediate_array[i]==0){
            immediate_array[i]=1;
        }else{
             immediate_array[i]=0;
        }
    }

    if(immediate_array[1]==0){//adds one to the inverted array
        immediate_array[1] += 1;
    }else{
        int m = 1;
        while(1){
            if(immediate_array[m] == 1){
                immediate_array[m] = 0;
                m++;
            }else if(immediate_array[m] == 0){
                immediate_array[m] = 1;
                break;
            }
            if(m == size){
                break;
            }
        }
    }
}
//identify the instruction based on the opcode and return string
char*  identifyType(InstructionElements *inst){

    if(inst->opcode == 51){
        return "R";
    }else if(inst->opcode == 3 || inst->opcode == 19 ||inst->opcode == 103){
        return "I";
    }else if(inst->opcode == 99){
        return "SB";
    }else if(inst->opcode == 111){ //jal
        return "UJ";
    }else if(inst->opcode == 35){
        return "S";
    }
}
// (4). MUX
Signal MUX(Signal sel, Signal input_0, Signal input_1)
{
    if (sel == 0){ 
        return input_0; 
    }else{ 
        return input_1; 
    }
}

// (5). Add
Signal Add(Signal input_0, Signal input_1)
{
    return (input_0 + input_1);
}

// (6). ShiftLeft2
Signal ShiftLeft1(Signal input)
{
    return input << 1; //Changed to shift twice ---> immediate value of -4 --> -8
}

//Decodes func3,func7,rs_1,rs2
void genDecode(InstructionElements *inst, int current_instruct[]){

    int power = 0;
    for(int i = 7; i <= 11; i++){
        inst->rd += current_instruct[i]*pow(2, power);
        power ++;
    }

    power = 0;
    for(int i = 12; i <= 14; i++){
        inst->func3 += current_instruct[i]*pow(2, power);
        power ++;
    }

    power = 0;
    for(int i = 15; i <= 19; i++){
        inst->rs_1 += current_instruct[i]*pow(2, power);
        power ++;
    }

    power = 0;
    for(int i = 20; i <= 24; i++){
        inst->rs_2 += current_instruct[i]*pow(2, power);
        power ++;
    }

    power = 0;
    for(int i = 25; i <= 31; i++){
        inst->func7 += current_instruct[i]*pow(2, power);
        power ++;
    }
}