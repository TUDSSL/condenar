#include "stdint.h"
#include "k_config.h"

//#define PREVENT_COMPILATION_INVALIDATION
#define PDR_MAGIC_NUMBER 0x68694206

extern uint32_t __saveStateStart;
extern uint32_t __saveStateEnd;

typedef struct pdr_header{
    uint32_t magicNumber; //Magic number    
    char compilationTimestamp[27]; //Hold the compilation timestamp of the firmware that created the checkpoint 
    bool checkpointValid; //Whether the checkpoint is currently valid. (This should be set to false while modifying it for example)
} pdr_header;

typedef struct pdr_fixedData{
    uint32_t registerData[14];
} pdr_fixedData;


//MRAM checkpoing structure (MRAM_SAVESTATE region)
__attribute__((__section__(".SaveState"))) struct MRAM_checkpoint_data{
    pdr_header header;
    pdr_fixedData fixedData;
    uint8_t heap[APP_HEAP_SIZE];
    uint8_t stack[APP_STACK_SIZE];
};

extern uint32_t g_pui32Stack[APP_STACK_SIZE/4];

extern uint32_t _mramCheckpointedBssStart; //Where the checkpointed BSS sections will start in the MRAM
extern uint32_t _mramCheckpointedDataStart; //Where the checkpointed DATA sections will start in the MRAM
extern uint32_t _mramCheckpointedBssStart2; //Where the checkpointed BSS sections will start in the MRAM
extern uint32_t _mramCheckpointedDataStart2; //Where the checkpointed DATA sections will start in the MRAM

extern uint32_t _scheckpointedBss; //Where the checkpointed BSS start in RAM
extern uint32_t _scheckpointedData; //Where the checkpointed DATA start in RAM

extern uint32_t _sizecheckpointedBss; //Size of the checkpointed BSS region
extern uint32_t _sizecheckpointedData; //Size of the checkpointer DATA region

bool k_isCheckpointValid( uint32_t id);

void k_markCheckpointAsValid( uint32_t id);
void k_markCheckpointAsInvalid( uint32_t id);

void k_ppr_restoreHeap( uint32_t id);
void k_ppr_checkpointHeap( uint32_t id);

void k_ppr_checkpointLinkerSections( uint32_t id);
void k_ppr_restoreLinkerSections( uint32_t id);

void k_getCheckpointHeader(pdr_header* headerOut, uint32_t id);
void k_setCheckpointHeader(pdr_header* header, uint32_t id);
