/** Emulate NAND flash memory.
 *  
 * Theory of operation:  We have a shared memory page, with two addresses.  
 * One address has read/write access, and the other has read only access.
 * 
 * The "r/w" memory pointer is considered "private".  Use the utility functions
 * for correctness sake.
 * 
 * We have utility methods for writing to the flash, but reading can be done
 * directly.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include "nand_flash.h"

int mock_nand_flash_erase(struct mock_nand_flash_st *mock, uint32_t mock_offset, uint32_t length){
  if(length%NV_PAGE_SIZE != 0){
    printf("length must be divible by page size (%d): is: %d\n",NV_PAGE_SIZE,length);
    return EINVAL;
  }
  if(mock_offset%4 != 0){
    printf("mock_offset must be page aligned (%d) is: %d\n",NV_PAGE_SIZE,mock_offset);
    return EINVAL;
  }

  //Flash can only change from 1->0, but not 0->1 except through a clear operation (using an AND operation).
  memset(&mock->rw_data_ptr[mock_offset],0xFF,length);

  return 0;
}


struct mock_nand_flash_st *mock_nand_flash_new(int fd,size_t length, off_t offset){
   struct mock_nand_flash_st  *retval = malloc(sizeof(struct mock_nand_flash_st));   
   if(retval == 0){
      printf("Malloc failed %d\n",errno);
   }

   retval->rw_data_ptr = (uint8_t *)mmap(0, length, PROT_WRITE | PROT_READ , MAP_SHARED, fd, offset);
   if(retval->rw_data_ptr == MAP_FAILED){
      printf("RW mmap failed: %s\n",strerror(errno));
   }

   retval->ro_data_ptr = (uint8_t *)mmap((void *) BLOCK_STORE_ADDRESS, length, PROT_READ | PROT_EXEC, MAP_SHARED, fd, offset);
   if(retval->ro_data_ptr == MAP_FAILED){
      printf("RO mmap failed: %s\n",strerror(errno));
   }

   retval->length = length;

   return retval;
}


/** Updates the "flash" memory with new data, must be 32 bit word aligned for length and offset 
 *  
 **/
int mock_nand_flash_write(struct mock_nand_flash_st *mock, uint32_t mock_offset, const uint8_t *data, uint32_t length,uint32_t data_offset){
  if(length%4 != 0){
    printf("length must be word-aligned (divisible by 4)");
    return EINVAL;
  }
  if(mock_offset%4 != 0){
    printf("mock_offset must be word-aligned (divisible by 4)");
    return EINVAL;
  }
  if(data_offset%4 != 0){
    printf("data_offset must be word-aligned (divisible by 4)");
    return EINVAL;
  }

  //Flash can only change from 1->0, but not 0->1 except through a clear operation (using an AND operation).
  for(uint32_t i=0; i < length;i+=4){
    mock->rw_data_ptr[mock_offset+i+0]&=data[data_offset+i+0];
    mock->rw_data_ptr[mock_offset+i+1]&=data[data_offset+i+1];
    mock->rw_data_ptr[mock_offset+i+2]&=data[data_offset+i+2];
    mock->rw_data_ptr[mock_offset+i+3]&=data[data_offset+i+3];
  }
  

  return 0;
}


void mock_nand_flash_destroy(struct mock_nand_flash_st *mock){
  munmap(mock->rw_data_ptr, mock->length);
  munmap(mock->ro_data_ptr, mock->length);

  free(mock);
}



#ifdef MOCK_TEST
#include <string.h>
#include <fcntl.h>

void handler(int nSignum, siginfo_t* si, void* vcontext) {
  printf("Segmentation fault\n");
  
//  ucontext_t* context = (ucontext_t*)vcontext;
//  context->uc_mcontext.gregs[REG_RIP]++;
}


int main(int argc, char *argv[]){
      
   if(argc != 2){
      printf("Usage:\n");
      printf("%s \"<nand_backing_file>\"\n",argv[0]);
      exit(1);
   }

   int fd = open(argv[1], O_RDWR|O_CREAT, 664);
   
   struct mock_nand_flash_st *mock  = mock_nand_flash_new(fd,40960,0);
   close(fd);
 
   //printf("DEBUG\nRW Page Addr: %x\nRO Page Addr: %x\nLength: %x\n",(uint64_t *)mock->rw_data_ptr,(uint64_t *) mock->ro_data_ptr,mock->length);
 
   const char *test_message_1 = "If you can read this, that is good.";
   snprintf(mock->rw_data_ptr,100,test_message_1);
   
   if(strncmp(test_message_1,mock->rw_data_ptr,strlen(test_message_1)) == 0){
      printf("RO Follows RW Page -> PASS\n");
   }else{
      printf("RO Follows RW Page -> FAIL\n");
      printf("Value on RW Page -> %s\n",mock->rw_data_ptr);
      printf("Value on RO Page -> %s\n",mock->ro_data_ptr);
   }
   

   //test that we can write data to the flash, and it shows up correctly.
   mock_nand_flash_erase(mock,0,40960);

   const char *test_message_2 = "Normal Write Test   "; //must be divisible by 4 in length
   int r = mock_nand_flash_write(mock, 0, test_message_2, strlen(test_message_2), 0);

   if(strncmp(test_message_2,mock->rw_data_ptr,strlen(test_message_2)) == 0){
      printf("mock_nand_flash_write -> PASS\n");
   }else{
      printf("mock_nand_flash_write -> FAIL\n");
      printf("Message: %s\n",test_message_2);
      printf("Value on RW Page -> %s\n",mock->rw_data_ptr);
      printf("Value on RO Page -> %s\n",mock->ro_data_ptr);
   }


   //test that we can write, but not change a 0 to a 1 (only a 1 to a 0)  
   //
   //We do this by writing a lower case set of letters, then changing them to upper case.  This should work.
   //This is a single bit flip on bit 5 1 for lower case, 0 for upper case.
   //Once we've done that, we try and change it back to lower case, which should remain upper case after.
   mock_nand_flash_erase(mock,0,40960);
   
   const char *test_message_3_lc = "abcdefghijkl"; //must be divisible by 4 in length
   const char *test_message_3_uc = "ABCDEFGHIJKL"; //must be divisible by 4 in length

   r = mock_nand_flash_write(mock, 0, test_message_3_lc, strlen(test_message_3_lc), 0);

   if(strncmp(test_message_3_lc,mock->rw_data_ptr,strlen(test_message_3_lc)) == 0){
      printf("mock_nand_flash_write[nand_emulation] -> SETUP\n");
   }else{
      printf("mock_nand_flash_write[nand_emulation] -> SETUP FAIL\n");
      printf("Message: %s\n",test_message_3_lc);
      printf("Value on RW Page -> %s\n",mock->rw_data_ptr);
      printf("Value on RO Page -> %s\n",mock->ro_data_ptr);
   }

   r = mock_nand_flash_write(mock, 0, test_message_3_uc, strlen(test_message_3_uc), 0);

   if(strncmp(test_message_3_uc,mock->rw_data_ptr,strlen(test_message_3_uc)) == 0){
      printf("mock_nand_flash_write[nand_emulation] -> Stage 1 Pass\n");
   }else{
      printf("mock_nand_flash_write[nand_emulation] -> Stage 1 Fail\n");
      printf("Message: %s\n",test_message_3_uc);
      printf("Value on RW Page -> %s\n",mock->rw_data_ptr);
      printf("Value on RO Page -> %s\n",mock->ro_data_ptr);
   }

   //now let's try to write the lower case letters back again.  It should still be upper case after we're done (because we can't change a 0 to a 1)
   r = mock_nand_flash_write(mock, 0, test_message_3_lc, strlen(test_message_3_lc), 0);
   
   if(strncmp(test_message_3_uc,mock->rw_data_ptr,strlen(test_message_3_uc)) == 0){
      printf("mock_nand_flash_write[nand_emulation] -> PASS\n");
   }else{
      printf("mock_nand_flash_write[nand_emulation] -> FAIL\n");
      printf("Message: %s\n",test_message_3_uc);
      printf("Value on RW Page -> %s\n",mock->rw_data_ptr);
      printf("Value on RO Page -> %s\n",mock->ro_data_ptr);
   }
   


/*
   struct sigaction action;
   memset(&action, 0, sizeof(struct sigaction));
   action.sa_flags = SA_SIGINFO;
   action.sa_sigaction = handler;
   sigaction(SIGSEGV, &action, NULL);
   
   printf("About to test writing to the RO page.  It SHOULD segfault here.\n");

   snprintf(mock->ro_data_ptr,100,"If you can read this, that is BAD!!!.");
   printf("Value on RW Page -> %s\n",mock->rw_data_ptr);
   printf("Value on RO Page -> %s\n",mock->ro_data_ptr);
*/
   mock_nand_flash_destroy(mock);

   return 0;
}
#endif
