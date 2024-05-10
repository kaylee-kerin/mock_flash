#include <stdint.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>

#define BLOCK_STORE_ADDRESS (uintptr_t)(0x08010000)

struct mock_nand_flash_st {
  uint8_t *ro_data_ptr;
  uint8_t *rw_data_ptr;
  size_t length; 
};

int mock_nand_flash_erase(struct mock_nand_flash_st *mock, uint32_t mock_offset, uint32_t length);

struct mock_nand_flash_st *mock_nand_flash_new(int fd,size_t length, off_t offset);

int mock_nand_flash_write(struct mock_nand_flash_st *mock, uint32_t mock_offset, const uint8_t *data, uint32_t length,uint32_t data_offset);

void mock_nand_flash_destroy(struct mock_nand_flash_st *mock);

