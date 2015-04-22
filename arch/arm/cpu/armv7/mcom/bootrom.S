#define BOOTROM_LOADED_ADDRESS 0x30000000 /* May be 0x30000000 or 0x00000000 */
#define BOOTROM_API_FUNCTION(name, o) \
	.equiv bootrom_##name, (BOOTROM_LOADED_ADDRESS + o); \
	.global bootrom_##name

BOOTROM_API_FUNCTION(set_cpu_stack,  0x20)
BOOTROM_API_FUNCTION(uart_config,    0x24)
BOOTROM_API_FUNCTION(uart_disable,   0x28)
BOOTROM_API_FUNCTION(uart_putstr,    0x2c)
BOOTROM_API_FUNCTION(PrintUint32,    0x30)

BOOTROM_API_FUNCTION(ConfigSPI0,     0x34)
BOOTROM_API_FUNCTION(DisableSPI0,    0x38)
BOOTROM_API_FUNCTION(ReadSPI0Flash,  0x3c)
BOOTROM_API_FUNCTION(WriteSPI0Flash, 0x40)
BOOTROM_API_FUNCTION(SendOpcodeSPI0, 0x44)
BOOTROM_API_FUNCTION(ReadStatusRegisterSPI0Flash, 0x48)

BOOTROM_API_FUNCTION(StrLen,         0x4c)
BOOTROM_API_FUNCTION(StrCpy,         0x50)
BOOTROM_API_FUNCTION(StrCompare,     0x54)
BOOTROM_API_FUNCTION(StrIndexOf,     0x58)
BOOTROM_API_FUNCTION(StringToUint32, 0x5c)

BOOTROM_API_FUNCTION(DDR_INIT,       0x60)
BOOTROM_API_FUNCTION(get_cpu_id,     0x64)
BOOTROM_API_FUNCTION(MemoryCopy,     0x68)

BOOTROM_API_FUNCTION(set_data_package, 0x6c)
BOOTROM_API_FUNCTION(compress_and_dump_register_fields, 0x70)

BOOTROM_API_FUNCTION(umctl2_load,     0x74)
BOOTROM_API_FUNCTION(lpddr2_pub_load, 0x78)
BOOTROM_API_FUNCTION(ddr3_pub_load,   0x7c)
BOOTROM_API_FUNCTION(init_start,      0x80)
BOOTROM_API_FUNCTION(pub_init_cmpl_wait, 0x84)
BOOTROM_API_FUNCTION(umctl2_norm_wait,   0x88)
BOOTROM_API_FUNCTION(ddr_self_refresh_enter, 0x8c)
BOOTROM_API_FUNCTION(ddr_self_refresh_exit,  0x90)
BOOTROM_API_FUNCTION(phy_train_load, 0x94)
BOOTROM_API_FUNCTION(phy_train_save, 0x98)

BOOTROM_API_FUNCTION(sd_get_nom,     0x9c)
BOOTROM_API_FUNCTION(sd_struct_load, 0xa0)
BOOTROM_API_FUNCTION(sd_read_block,  0xa4)

BOOTROM_API_FUNCTION(nand_ctr_init,  0xa8)
BOOTROM_API_FUNCTION(nand_reset,     0xac)
BOOTROM_API_FUNCTION(nand_read,      0xb0)

BOOTROM_API_FUNCTION(rtc_init,       0xb4);
BOOTROM_API_FUNCTION(ddr_ret,        0xb8);
BOOTROM_API_FUNCTION(set_lpddr2_cfg, 0xbc);
BOOTROM_API_FUNCTION(set_ddr3_cfg,   0xc0);
