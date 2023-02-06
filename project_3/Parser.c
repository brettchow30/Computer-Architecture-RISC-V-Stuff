#include "Parser.h"
#include <math.h>
// FIXME, implement this function.
// Here shows an example on how to translate "add x10, x10, x25"

void setInstructionInf(int *opcode, int *funct3, int *funct7, char **type, char *opr);
void decimalToBinary(int a[], int decimal, int arr_len);
void twosComplement(int a[], int arr_len);
void UJimmediate(int *im_20, int *im_10_1, int *im_11, int *im_19_12, int a[]); 
void SBimmediate(int *im_11, int *im_4_1, int *im_10_5, int *im_12, int a[]);
void Simmediate(int *im_11_5, int *im_4_0, int a[]);
void loadInstructions(Instruction_Memory *i_mem, const char *trace)
{
    printf("Loading trace file: %s\n", trace);

    FILE *fd = fopen(trace, "r");
    if (fd == NULL)
    {
        perror("Cannot open trace file. \n");
        exit(EXIT_FAILURE);
    }

    // Iterate all the assembly instructions
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    Addr PC = 0; // program counter points to the zeroth location initially.
    int IMEM_index = 0;
    while ((read = getline(&line, &len, fd)) != -1)
    {
        // Assign program counter
        i_mem->instructions[IMEM_index].addr = PC;

        // Extract operation
        char *raw_instr = strtok(line, " ");
        if (strcmp(raw_instr, "add") == 0 || 
            strcmp(raw_instr, "sub") == 0 ||
            strcmp(raw_instr, "srl") == 0 ||
            strcmp(raw_instr, "xor") == 0 ||
            strcmp(raw_instr, "or")  == 0 ||
            strcmp(raw_instr, "and") == 0 ||
            strcmp(raw_instr, "ld") == 0 || 
            strcmp(raw_instr, "addi") == 0 || 
            strcmp(raw_instr, "slli") == 0 ||
            strcmp(raw_instr, "bne") == 0 ||
            strcmp(raw_instr, "bge") == 0 ||
            strcmp(raw_instr, "beq") == 0 ||
            strcmp(raw_instr, "jal") == 0 ||
            strcmp(raw_instr, "jalr") == 0 ||
            strcmp(raw_instr, "sd") == 0 ||
            strcmp(raw_instr, "sll") == 0
            )
        {
            parseRType(raw_instr, &(i_mem->instructions[IMEM_index]));
            i_mem->last = &(i_mem->instructions[IMEM_index]);
	}

        IMEM_index++;
        PC += 4;
    }

    fclose(fd);
}

void parseRType(char *opr, Instruction *instr)
{
    instr->instruction = 0;
    int opcode = 0;
    int funct3 = 0;
    int funct7 = 0;
    char *type;

    setInstructionInf(&opcode, &funct3, &funct7, &type, opr);

    int rd, rs_1, rs_2, immediate;

    //Obtain the register values from the instruction
    char *reg_1 = strtok(NULL, ", ");

    char *reg_2 = strtok(NULL, ", ");

    char *reg_3 = strtok(NULL, ", ");
    
    // printf("%s \n", reg_1);//rd
    // printf("%s \n", reg_2);//rs1
    // printf("%s \n", reg_3);//rs2
    
    rd = regIndex(reg_1); //rd always matches the format x#

    if(reg_3 == NULL && (strcmp(type, "I") == 0)){//ld, jalr, sd commands lack third register therefore reg_3 == NULL

        char *arr_indx = strtok(reg_2, "("); //get the index of the array -> #
        char *temp = strtok(NULL, "(");
        char *arr_addr = strtok(temp, ")"); //gets the address of the array -> x#
        rs_1 = regIndex(arr_addr);
        immediate = atoi(arr_indx); 

    }else if((strcmp(type, "S") == 0)){
        char *arr_indx = strtok(reg_2, "("); //get the index of the array -> #
        char *temp = strtok(NULL, "(");
        char *arr_addr = strtok(temp, ")"); //gets the address of the array -> x#
        immediate = atoi(arr_indx);
        rs_1 = regIndex(arr_addr);
        rs_2 = regIndex(reg_1);
    }
    else if(reg_3 == NULL && (strcmp(type, "UJ") == 0)){ // jal command lacks third register
        immediate = atoi(reg_2);
        
    }else{//format is x#, x#, x# or x#, x#, # (# is an integer)

        if( (strcmp(type, "I") == 0) || (strcmp(type, "SB") == 0)){ //instruction is in format x#, x#, # (# is int; I-type/SB-type);
            rs_1 = regIndex(reg_2);
            immediate = atoi(reg_3); 
        }
        else if(strcmp(type, "R") == 0){//instruction is in format x#, x#, x#; R-type
             //reg_3[strlen(reg_3)-1] = '\0';
             if(reg_3[strlen(reg_3)-1] == '\n'){
                reg_3[strlen(reg_3)-1] = '\0';
             }
             rs_1 = regIndex(reg_2);
             rs_2 = regIndex(reg_3);
        } 
    }

    //SB type uses a different template to form binary -> the following code splits the binary representation of the 
    //location (-4 in this case) into the necessary pieces for the binary representation

    int im_11 = 0, im_4_1 = 0, im_10_5 = 0, im_12 = 0;
    int im_20 = 0, im_10_1 = 0, im_19_12 = 0; //used for UJ type immediate parsing
    int im_11_5 = 0, im_4_0 = 0; //used for S immediate

    if(strcmp(type, "SB") == 0){
        int arr_len = 12;
        int a[arr_len];
        decimalToBinary(a, immediate, arr_len);

        if(immediate > 0){//immediate value is positive
            SBimmediate(&im_11, &im_4_1, &im_10_5, &im_12, a);
        }
        else{
            twosComplement(a, arr_len); 
            SBimmediate(&im_11, &im_4_1, &im_10_5, &im_12, a);
        }

    }else if(strcmp(type, "UJ") == 0){
        int arr_len = 20;
        int a[arr_len];

        decimalToBinary(a, immediate, arr_len);

        if(immediate > 0){//immediate value is positive
            UJimmediate(&im_20, &im_10_1, &im_11, &im_19_12, a);
        }
        else{
            twosComplement(a, arr_len);
            UJimmediate(&im_20, &im_10_1, &im_11, &im_19_12, a);
        }
    }else if(strcmp(type, "S") == 0){
        int arr_len = 11;
        int a[arr_len];
        decimalToBinary(a, immediate, arr_len);
        Simmediate(&im_11_5, &im_4_0, a);
    }

    // Contruct instruction
    if(strcmp(type, "R") == 0){ //instruction is of type R
        instr->instruction |= opcode;
        instr->instruction |= (rd << 7);
        instr->instruction |= (funct3 << (7 + 5));
        instr->instruction |= (rs_1 << (7 + 5 + 3));
        instr->instruction |= (rs_2 << (7 + 5 + 3 + 5));
        instr->instruction |= (funct7 << (7 + 5 + 3 + 5 + 5));
    }
    else if(strcmp(type, "I") == 0){ //instruction is of type I
        instr->instruction |= opcode;
        instr->instruction |= (rd << 7);
        instr->instruction |= (funct3 << (7 + 5));
        instr->instruction |= (rs_1 << (7 + 5 + 3));
        instr->instruction |= (immediate << (7 + 5 + 3 + 5)); //rs_2 == immediate
    }
    else if(strcmp(type, "SB") == 0){//instruction is of type SB
        instr->instruction |= opcode;
        instr->instruction |= (im_11 << 7);
        instr->instruction |= (im_4_1 << (7 + 1));
        instr->instruction |= (funct3 << (7 + 1 + 4));
        instr->instruction |= (rd << (7 + 1 + 4 + 3));
        instr->instruction |= (rs_1 << (7 + 1 + 4 + 3 + 5));
        instr->instruction |= (im_10_5 << (7 + 1 + 4 + 3 + 5 + 5));
        instr->instruction |= (im_12 << (7 + 1 + 4 + 3 + 5 + 5 + 6));
    }
    else if(strcmp(type, "UJ") == 0){
        instr->instruction |= opcode;
        instr->instruction |= (rd << 7);
        instr->instruction |= (im_19_12 << (7 + 5));
        instr->instruction |= (im_11 << (7 + 5 + 8));
        instr->instruction |= (im_10_1 << (7 + 5 + 8 + 1));
        instr->instruction |= (im_20 << (7 + 5 + 8 + 1 + 10));
    }
    else if(strcmp(type, "S") == 0){
        instr->instruction |= opcode;
        instr->instruction |= (im_4_0 << 7);
        instr->instruction |= (funct3 << (7 + 5));
        instr->instruction |= (rs_1 << (7 + 5 + 3));
        instr->instruction |= (rs_2 << (7 + 5 + 3 + 5));
        instr->instruction |= (im_11_5 << (7 + 5 + 3 + 5 + 5));
    }
}

int regIndex(char *reg)
{
    unsigned i = 0;
    for (i; i < NUM_OF_REGS; i++)
    {
        if (strcmp(REGISTER_NAME[i], reg) == 0)
        {
            break;
        }
    }

    return i;
}


void Simmediate(int *im_11_5, int *im_4_0, int a[]){
    int i = 0;
    for(i=5;i<=11;i++){    
        *im_11_5 += a[i]*pow(2, i-5);   
    }
    for(i=0;i<=4;i++){    
        *im_4_0 += a[i]*pow(2, i-1);   
    }
}

void UJimmediate(int *im_20, int *im_10_1, int *im_11, int *im_19_12, int a[]){
    int i = 0;
    *im_20 = a[20];
    *im_11 = a[11];
    for(i=12;i<=19;i++){ 
        *im_19_12 += a[i]*pow(2, i-12);   
    }
    for(i=1;i<=10;i++){    
        *im_10_1 += a[i]*pow(2, i-1);   
    }
}
void SBimmediate(int *im_11, int *im_4_1, int *im_10_5, int *im_12, int a[]){
    int i = 0;
    *im_12 = a[12];
    *im_11 = a[11];
    for(i=5;i<=10;i++){    
        *im_10_5 += a[i]*pow(2, i-5);   
    }
    for(i=1;i<=4;i++){    
        *im_4_1 += a[i]*pow(2, i-1);   
    }
}

void decimalToBinary(int a[], int decimal, int arr_len){
    int i = 0;
    decimal = abs(decimal);
    for(i=0;i<=arr_len;i++){    
        a[i]=decimal%2;    
        decimal=decimal/2;
    }
}

void twosComplement(int a[], int arr_len){
    int flag = 0, i = 0;

    for(i=0;i<=arr_len;i++){
        if(flag){
            if(a[i] == 0){
                a[i] = 1;
            }else{
                a[i] = 0;
            }
        }else{
            if(a[i] == 1){
                flag = 1;
            }
        } 
    }
}
void setInstructionInf(int *opcode, int *funct3, int *funct7, char **type, char *opr){

    if (strcmp(opr, "add") == 0) //add instruction
    {
        *type = "R";
        *opcode = 51;
        *funct3 = 0;
        *funct7 = 0;
    }

    if (strcmp(opr, "ld") == 0) //ld instruction
    {
        *type = "I";
        *opcode = 3;
        *funct3 = 3;
        *funct7 = 0;
    }

    if (strcmp(opr, "addi") == 0) //addi instruction
    {
        *type = "I";
        *opcode = 19;
        *funct3 = 0;
        *funct7 = 0;
    }

    if (strcmp(opr, "slli") == 0) //slli instruction
    {
        *type = "I";
        *opcode = 19;
        *funct3 = 1;
        *funct7 = 0;
    }

    if (strcmp(opr, "bne") == 0) //bne instruction
    {   
        *type = "SB";
        *opcode = 99;
        *funct3 = 1;
        *funct7 = 0;
    }

    //bellow - implement instructions for project 3
    if (strcmp(opr, "bge") == 0) 
    {   
        *type = "SB";
        *opcode = 99;
        *funct3 = 5;
        *funct7 = 0;//none
    }
    if (strcmp(opr, "beq") == 0) 
    {   
        *type = "SB";
        *opcode = 99;
        *funct3 = 0;
        *funct7 = 0;//none
    }
    if (strcmp(opr, "jal") == 0) 
    {   
        *type = "UJ";
        *opcode = 111;
        *funct3 = 0;
        *funct7 = 0;
    }
    if (strcmp(opr, "jalr") == 0) 
    {   
        *type = "I";
        *opcode = 103;
        *funct3 = 0;
        *funct7 = 0;
    }
    if (strcmp(opr, "sd") == 0) 
    {   
        *type = "S";
        *opcode = 35;
        *funct3 = 3;
        *funct7 = 0;
    }
    if (strcmp(opr, "sll") == 0) 
    {   
        *type = "R";
        *opcode = 51;
        *funct3 = 1;
        *funct7 = 0;
    }
}
