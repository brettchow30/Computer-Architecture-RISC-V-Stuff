#include "Parser.h"
#include <math.h>

//Students: 
//Aleksandar Aleksandrov
//Brett Chow

//To add:
//ld x9, 0(x10)
//addi x22, x22, 1
//slli x11, x22, 3
//bne x8, x24, -4

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
        if (strcmp(raw_instr, "add") == 0 || //CHANGE: Add ld, addi, skki, bne to the if statement
            strcmp(raw_instr, "sub") == 0 ||
            strcmp(raw_instr, "sll") == 0 ||
            strcmp(raw_instr, "srl") == 0 ||
            strcmp(raw_instr, "xor") == 0 ||
            strcmp(raw_instr, "or")  == 0 ||
            strcmp(raw_instr, "and") == 0 ||
            strcmp(raw_instr, "ld") == 0 || 
            strcmp(raw_instr, "addi") == 0 || 
            strcmp(raw_instr, "slli") == 0 ||
            strcmp(raw_instr, "bne") == 0)
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
    unsigned opcode = 0;
    unsigned funct3 = 0;
    unsigned funct7 = 0;
    char *type;

    if (strcmp(opr, "add") == 0) //add instruction
    {
        type = "R";
        opcode = 51;
        funct3 = 0;
        funct7 = 0;
    }

    if (strcmp(opr, "ld") == 0) //ld instruction
    {
        type = "I";
        opcode = 3;
        funct3 = 3;
        funct7 = 0;
    }

    if (strcmp(opr, "addi") == 0) //addi instruction
    {
        type = "I";
        opcode = 19;
        funct3 = 0;
        funct7 = 0;
    }

    if (strcmp(opr, "slli") == 0) //slli instruction
    {
        type = "I";
        opcode = 19;
        funct3 = 1;
        funct7 = 0;
    }

    if (strcmp(opr, "bne") == 0) //bne instruction
    {   
        type = "SB";
        opcode = 103;
        funct3 = 1;
        funct7 = 0;
    }

    int rd, rs_1, rs_2;

    //Obtain the register values from the instruction
    char *reg_1 = strtok(NULL, ", ");

    char *reg_2 = strtok(NULL, ", ");

    char *reg_3 = strtok(NULL, ", ");
    

    rd = regIndex(reg_1); //rd always matches the format x#

    if(reg_3 == NULL){//ld command lacks third register therefore reg_3 == NULL

        char *arr_indx = strtok(reg_2, "("); //get the index of the array -> #
        char *temp = strtok(NULL, "(");
        char *arr_addr = strtok(temp, ")"); //gets the address of the array -> x#
        rs_1 = regIndex(arr_addr);
        rs_2 = atoi(arr_indx); //set res_2 == index of array (goes in immediate)

    }else{//test if format is x#, x#, x# or x#, x#, # (# is an integer)
        char test_ch = 'x';
        char *test_result;
        test_result =  strchr(reg_3, test_ch); //tests if the third argument has an integer or a register

        if(test_result == NULL ){ //instruction is in format x#, x#, # (# is int; I-type);
            rs_1 = regIndex(reg_2);
            rs_2 = atoi(reg_3); 
        }else{//instruction is in format x#, x#, x#; R-type
             reg_3[strlen(reg_3)-1] = '\0';
             rs_1 = regIndex(reg_2);
             rs_2 = regIndex(reg_3);
        } 
    }

    //SB type uses a different template to form binary -> the following code splits the binary representation of the 
    //location (-4 in this case) into the necessary pieces for the binary representation
    unsigned int im_11 = 0, im_4_1 = 0, im_10_5 = 0, im_12 = 0;
    if(strcmp(type, "SB") == 0){
        int a[12],i;
        if(rs_2 > 0){//the location is positive -> just find the binary value and split
            for(i=0;i<=12;i++){    
                a[i]=rs_2%2;    
                rs_2=rs_2/2;
            }
            im_12 = a[12];
            im_11 = a[11];
            for(i=5;i<=10;i++){    
                im_10_5 += a[i]*pow(2, i-5);   
            }
            for(i=1;i<=4;i++){    
                im_4_1 += a[i]*pow(2, i-1);   
            }
        }else{//the location is negative -> do two's complement
            rs_2 = fabs(rs_2);
            for(i=0;i<=12;i++){    
                a[i]=rs_2%2;    
                rs_2=rs_2/2;
            }
            int flag = 0;
            //calculate the twos complement
            for(i=0;i<=12;i++){
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
            im_12 = a[12];
            im_11 = a[11]; 
            for(i=5;i<=10;i++){ 
                im_10_5 += a[i]*pow(2, i-5);   
            }
            for(i=1;i<=4;i++){    
                im_4_1 += a[i]*pow(2, i-1);   
            }
        }  
    }
    // Contruct instruction
    if(strcmp(type, "R") == 0){ //instruction is of type R
        instr->instruction |= opcode;
        instr->instruction |= (rd << 7);
        instr->instruction |= (funct3 << (7 + 5));
        instr->instruction |= (rs_1 << (7 + 5 + 3));
        instr->instruction |= (rs_2 << (7 + 5 + 3 + 5));
        instr->instruction |= (funct7 << (7 + 5 + 3 + 5 + 5));

    }else if(strcmp(type, "I") == 0){ //instruction is of type I
        instr->instruction |= opcode;
        instr->instruction |= (rd << 7);
        instr->instruction |= (funct3 << (7 + 5));
        instr->instruction |= (rs_1 << (7 + 5 + 3));
        instr->instruction |= (rs_2 << (7 + 5 + 3 + 5)); //rs_2 == immediate

    }else if(strcmp(type, "SB") == 0){//instruction is of type SB
        instr->instruction |= opcode;
        instr->instruction |= (im_11 << 7);
        instr->instruction |= (im_4_1 << (7 + 1));
        instr->instruction |= (funct3 << (7 + 1 + 4));
        instr->instruction |= (rd << (7 + 1 + 4 + 3));
        instr->instruction |= (rs_1 << (7 + 1 + 4 + 3 + 5));
        instr->instruction |= (im_10_5 << (7 + 1 + 4 + 3 + 5 + 5));
        instr->instruction |= (im_12 << (7 + 1 + 4 + 3 + 5 + 5 + 6));
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
